/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacController.cpp
@Classification : MAC Controller
@
@Last alteration : April 17th, 2020
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module manages all System execution modes and their transitions. 
    It creates and starts TUN Interface reading, Control module, and decoding threads. 
    Also, management of SDUs concatenation into PDU is made by this module. (temporarily)
*/

#include "MacController.h"

MacController::MacController(
    const char* _deviceNameTun,     			//TUN device name
    bool _verbose)                  			//Verbosity flag
{    
    //Assign verbosity flag
    verbose = _verbose;

    //Assign TUN device name
    deviceNameTun = _deviceNameTun;

    //Read default information from file and record to "Current.txt"
    currentParameters = new CurrentParameters(verbose);
    currentParameters->readTxtSystemParameters("Default.txt");
    currentParameters->recordTxtCurrentParameters();

    //Initialize CLI-Interface class
	cliL2Interface = new CLIL2Interface(verbose);

	//Fill dynamic Parameters with default parameters (stating system)
	currentParameters->loadDynamicParametersDefaultInformation(cliL2Interface->dynamicParameters);
}

MacController::~MacController(){
    delete cosora;
    delete scheduler;
    delete protocolControl;
    delete sduBuffers;
    delete receptionProtocol;
    delete transmissionProtocol;
    delete tunInterface;
    delete l1l2Interface;
    delete [] threads;
    delete ipMacTable;
    delete timerSubframe;

    //Delete current system parameters only shutting down MAC
    //In STOP_MODE, there's no need to destroy system parameters and CLI interface
    if(currentParameters->getMacMode()!=STOP_MODE){
        delete cliL2Interface;
        delete currentParameters;
    }
}

void
MacController::initialize(){
    currentParameters->setMacMode(STANDBY_MODE);      //Initializes MAC in STANDBY_MODE 
    cout<<"\n\n[MacController] ___________ System entering STANDBY mode. ___________\n"<<endl;
    manager();
}

void
MacController::manager(){
    //Infinite loop
    while(1){
        switch(currentParameters->getMacMode()){
            case STANDBY_MODE:
            {
                //System waits for MacStartCommand
                if(cliL2Interface->getMacStartCommandSignal()){
                    cliL2Interface->setMacStartCommandSignal(false);
                    currentParameters->setMacMode(CONFIG_MODE);
                    cout<<"\n\n[MacController] ___________ System entering CONFIG mode. ___________\n"<<endl;
                }
            }
            break;

            case CONFIG_MODE:
            {
                //All MAC Initial Configuration is made here

                //Read txt current parameters and initialize flagBS and currentMacAddress values
                currentParameters->readTxtSystemParameters("Current.txt");
                flagBS = currentParameters->isBaseStation();
                currentMacAddress = currentParameters->getCurrentMacAddress();
                
            	//Fill dynamic Parameters with current parameters (updating system)
                currentParameters->loadDynamicParametersDefaultInformation(cliL2Interface->dynamicParameters);

                //Create Timer Subframe module for Subframe-time counting
                timerSubframe = new TimerSubframe();

                //Define IP-MAC correlation table creating and initializing a MacAddressTable with static informations (HARDCODE)
                ipMacTable = new MacAddressTable(verbose);
                uint8_t addressEntry0[4] = {10,0,0,10};
                uint8_t addressEntry1[4] = {10,0,0,11};
                uint8_t addressEntry2[4] = {10,0,0,12};
                ipMacTable->addEntry(addressEntry0, 0);
                ipMacTable->addEntry(addressEntry1, 1);
                ipMacTable->addEntry(addressEntry2, 2);
                
                //Create Tun Interface and allocate it
                tunInterface = new TunInterface(deviceNameTun, currentParameters->getMTU(), verbose);
                if(!(tunInterface->allocTunInterface())){
                    if(verbose) cout << "[MacController] Error allocating tun interface." << endl;
                    exit(1);
                }

                //Create L1L2Interface
                l1l2Interface = new L1L2Interface(verbose);

                //Create reception and transmission protocols
                receptionProtocol = new ReceptionProtocol(l1l2Interface, tunInterface, verbose);
                transmissionProtocol = new TransmissionProtocol(l1l2Interface,tunInterface, verbose);

                //Create SduBuffers to store MACD and MACC SDUs
                sduBuffers = new SduBuffers(receptionProtocol, currentParameters, ipMacTable, timerSubframe, verbose);

                //Create Scheduler to make Spectrum and SDU scheduling
                scheduler = new Scheduler(currentParameters, sduBuffers, verbose);

                //Create COSORA module for Fusion calculation
                cosora = new Cosora(cliL2Interface->dynamicParameters, currentParameters, verbose);

                //Threads definition
                /** Threads order:
                 * 0    ---> ProtocolData MACD SDU enqueueing (From L3)
                 * 1    ---> Reading control messages from PHY
                 * 2    ---> Counting Subframe time-intervals
                 * 3    ---> Checking for IP packets timeout
                 */
                threads = new thread[4];

                //Create ProtocolControl to deal with MACC SDUs
                protocolControl = new ProtocolControl(this, verbose);

                if(flagBS){
                    //Perform MACC SDU construction to send to all UEs with actual system information
                    vector<uint8_t> dynamicParametersBytes;

                    //Enqueue a MACC SDU to each UE attached
                    for(int i=0;i<currentParameters->getNumberUEs();i++){
                        dynamicParametersBytes.clear();
                        currentParameters->serialize(currentParameters->getMacAddress(i), dynamicParametersBytes);
                        sduBuffers->enqueueControlSdu(&(dynamicParametersBytes[0]), dynamicParametersBytes.size(), currentParameters->getMacAddress(i));
                    }
                }

                //Here, all system threads that don't execute only in IDLE_MODE are started.
                startThreads();

                //Set MAC mode to start mode
                currentParameters->setMacMode(START_MODE);

                cout<<"\n\n[MacController] ___________ System entering START mode. ___________\n"<<endl;
            }
            break;

            case START_MODE:
            {
                //Send PHYConfig.Request message to PHY (no parameters needed)
                char configRequestMessage = 'A';
                protocolControl->sendInterlayerMessages(&configRequestMessage, 1);

                //Wait for PHY to be ready
                this_thread::sleep_for(chrono::seconds(PHY_READY));
            }
            break;

            case IDLE_MODE:
            {
                //System will continue to execute idle threads (receiving from L1 or L3) and wait for other commands e.g MacConfigRequestCommand or MacStopCommand

                //In BS, check for ConfigRequest or Stop commands. In UE, check only for Stop commands
                if(flagBS){     //On BS
                    if(cliL2Interface->getMacConfigRequestCommandSignal()){           //MacConfigRequest
                        currentParameters->setMacMode(RECONFIG_MODE);                                 //Change mode

                        //Set flag to indicate that UEs are out-of-date
                        currentParameters->setFlagUesOutdated(true);

                        cout<<"\n\n[MacController] ___________ System entering RECONFIG mode by CLI command. ___________\n"<<endl;
                    }
                    else{ 
                        if(cliL2Interface->getMacStopCommandSignal()){                //Mac Stop
                            
                            //Send PHYStop.Request message to PHY (no parameters needed)
                            char stopRequestMessage = 'B';
                            protocolControl->sendInterlayerMessages(&stopRequestMessage, 1);

                            //Wait 1s for PHYConfig.Response Message
                            this_thread::sleep_for(chrono::seconds(PHY_READY));
                        }
                    }
                }
                else{       //On UE
                    if(cliL2Interface->getMacStopCommandSignal()){                    //Mac Stop  
                        
                        //Send PHYStop.Request message to PHY (no parameters needed)
                        char stopRequestMessage = 'B';
                        protocolControl->sendInterlayerMessages(&stopRequestMessage, 1);

                        //Wait 1s for PHYConfig.Response Message
                        this_thread::sleep_for(chrono::seconds(PHY_READY));
                    }
                }
            }
            break;

            case RECONFIG_MODE:
            {
                //To enter RECONFIG_MODE, TX and RX must be disabled
                if(currentParameters->getMacRxMode()==DISABLED_MODE_RX && currentParameters->getMacTxMode()==DISABLED_MODE_TX){

                    //System will update current parameters with dynamic parameters
                    if(!flagBS){    //If it is UE, parameters to update are CLI's and ULMCS
                        currentParameters->setUEParameters(cliL2Interface->dynamicParameters);
                    }
                    else            //If it is BS, it is needed to test if the parameters changed are CLI's or system's
                    {
                        if(cliL2Interface->getMacConfigRequestCommandSignal())  //CLI parameters changed
                            currentParameters->setCLIParameters(cliL2Interface->dynamicParameters);
                        else{   //System parameters changed           
                            if(cliL2Interface->dynamicParameters->getFLUTMatrix()!=currentParameters->getFLUTMatrix()){
                                if(verbose) cout<<"[MacController] Fusion Lookup Table values changed. Sending new value to PHY..."<<endl;
                                char buffer[2];
                                buffer[1] = 'F';
                                buffer[2] = cliL2Interface->dynamicParameters->getFLUTMatrix();
                                protocolControl->sendInterlayerMessages(buffer, 2);
                            }
                            currentParameters->setSystemParameters(cliL2Interface->dynamicParameters);
                        }
                        
                        if(currentParameters->areUesOutdated()){
                            //Perform MACC SDU construction to send to UE
                            vector<uint8_t> dynamicParametersBytes;

                            //Send a MACC SDU to each UE attached
                            for(int i=0;i<currentParameters->getNumberUEs();i++){
                                dynamicParametersBytes.clear();
                                currentParameters->serialize(currentParameters->getMacAddress(i), dynamicParametersBytes);
                                sduBuffers->enqueueControlSdu(&(dynamicParametersBytes[0]), dynamicParametersBytes.size(), currentParameters->getMacAddress(i));
                            }

                            //Set Flag of UEs Out of Date to false
                            currentParameters->setFlagUesOutdated(false);
                        }
                    }

                    //Record updated parameters
                    currentParameters->recordTxtCurrentParameters();

                    //Set MacConfigRequestSignal to false
                    cliL2Interface->setMacConfigRequestCommandSignal(false);

                    //Set MAC mode back to idle mode
                    currentParameters->setMacMode(IDLE_MODE);

                    if(verbose) cout<<"[MacController] Current Parameters updated correctly."<<endl;

                    cout<<"\n\n[MacController] ___________ System entering IDLE mode. ___________\n"<<endl;
                }
            }
            break;

            case STOP_MODE:
            {
                //Stop counting Subframes
                timerSubframe->stopCounting();

                //To enter RECONFIG_MODE, TX, RX and Tun modes must be disabled and COSORA must not be waiting for SSR
                if(currentParameters->getMacRxMode()==DISABLED_MODE_RX && currentParameters->getMacTxMode()==DISABLED_MODE_TX && currentParameters->getMacTunMode()==TUN_DISABLED && !cosora->isBusy()){
                    //Reset flag
                    cliL2Interface->setMacStopCommandSignal(false);
                    
                    //Destroy all System environment variables
                    this->~MacController();

                    //Set MAC mode back to idle mode
                    currentParameters->setMacMode(STANDBY_MODE);

                    cout<<"\n\n[MacController] ___________ System entering STANDBY mode. ___________\n"<<endl;
                
                }
            }
            break;

            default:
            {
                if(verbose) cout<<"\n\n[MacController] ___________Unknown mode ___________\n"<<endl;
            }
            break;
        }
    }
}

void 
MacController::startThreads(){

    //TUN queue control thread (only IDLE mode)
    threads[0] = thread(&SduBuffers::enqueueingDataSdus, sduBuffers);

    //Control messages from PHY reading (only IDLE mode)
    threads[1] = thread(&ProtocolControl::receiveInterlayerMessages, protocolControl);

    //Counting Subframe time-intervals
    threads[2] = thread(&TimerSubframe::countingThread, timerSubframe);

    //Checking for IP packets timeout
    threads[3] = thread(&SduBuffers::dataSduTimeoutChecking, sduBuffers);

    //Join all threads
    for(int i=0;i<4;i++){
        //Join all threads IDLE mode 
        threads[i].detach();
    }

    if(verbose) cout<<"[MacController] Threads started successfully."<<endl;
}

void
MacController::scheduling(){
    if(currentParameters->getMacMode()==IDLE_MODE){
        currentParameters->setMacTxMode(ACTIVE_MODE_TX);
        //Create vectors to store UEIDs and number of packets for next transmission
        vector<uint8_t> ueIds;                  //UEIDs of each UE selected for next transmission
        vector<int> bufferSize;                 //Current buffer status for each UE
        vector<allocation_cfg_t> allocations;   //Vector containing allocation for next transmission

        //Before any procedure, check if there are channels available for transmission
        if(flagBS && currentParameters->getFLUTMatrix()==0){
            if(verbose) cout<<"[MacController] All TV channels are busy"<<endl;
            return;
        }

        //Get buffer status information and store into ueIds and bufferSize vectors
        sduBuffers->bufferStatusInformation(ueIds, bufferSize);

        //Execute if there are UEs selected for transmission
        if(ueIds.size()>0){
            //If it is BS, perform spectrum allocation calculation for next transmission
            if(flagBS)
                scheduler->scheduleRequest(ueIds, bufferSize , allocations);
            else{
                allocations.resize(1);
                allocations[0] = currentParameters->getUlReservation(currentParameters->getCurrentMacAddress());
            }

            //Create MacPDU structures and populate allocations
            vector<MacPDU> macPdus;
            macPdus.resize(allocations.size());
            for(int i=0;i<allocations.size();i++)
                macPdus[i].allocation_ = allocations[i];

            //Schedule SDUs into PDU(s)
            scheduler->fillMacPdus(macPdus);
            

            //Test if there is actualy information to send
            if(macPdus.size() > 0){
                //Get number of UEs for next transmission
                int numberUes = 1;
                uint8_t currentUeId = macPdus[0].allocation_.target_ue_id;
                for(int i=1;i<macPdus.size();i++){
                    if(macPdus[i].allocation_.target_ue_id!=currentUeId){
                        numberUes++;
                        currentUeId = macPdus[i].allocation_.target_ue_id;
                    }
                }

                //Create SubframeTx.Start message
                string messageParameters;		            //This string will contain the parameters of the message
                vector<uint8_t> messageParametersBytes;	    //Vector to receive serialized parameters structure

                if(flagBS){     //Create BSSubframeTx.Start message
                    BSSubframeTx_Start messageBS;	//Message parameters structure

                    //Fill the structure with information
                    messageBS.numUEs = numberUes;
                    messageBS.numPDUs = macPdus.size();
                    messageBS.fLutDL = currentParameters->getFLUTMatrix();

                    //Get Uplink reservations for each UE in this transmission
                    currentUeId = macPdus[0].allocation_.target_ue_id;
                    messageBS.ulReservations.push_back(currentParameters->getUlReservation(currentUeId));
                    for(int i=1;i<macPdus.size();i++){
                        if(currentUeId != macPdus[i].allocation_.target_ue_id){
                            currentUeId = macPdus[i].allocation_.target_ue_id;
                            messageBS.ulReservations.push_back(currentParameters->getUlReservation(currentUeId));
                        }
                    }
                    messageBS.numerology = currentParameters->getNumerology();
                    messageBS.ofdm_gfdm = currentParameters->isGFDM()? 1:0;
                    messageBS.rxMetricPeriodicity = currentParameters->getRxMetricsPeriodicity();

                    //Serialize struct
                    messageBS.serialize(messageParametersBytes);

                }
                else{       //Create UESubframeTx.Start message
                    UESubframeTx_Start messageUE;	//Messages parameters structure

                    //Fill the structure with information
                    messageUE.ulReservation = currentParameters->getUlReservation(currentParameters->getMacAddress(0));
                    messageUE.numerology = currentParameters->getNumerology();
                    messageUE.ofdm_gfdm = currentParameters->isGFDM()? 1:0;
                    messageUE.rxMetricPeriodicity = currentParameters->getRxMetricsPeriodicity();

                    //Serialize struct
                    messageUE.serialize(messageParametersBytes);
                    }

                //Copy structure bytes to message
                for(uint i=0;i<messageParametersBytes.size();i++)
                    messageParameters+=messageParametersBytes[i];

                //Downlink routine:
                string subFrameStartMessage = flagBS? "C":"D";
                string subFrameEndMessage = "E";
                
                //Add parameters to original message
                subFrameStartMessage+=messageParameters;

                //Send interlayer messages and the PDU
                protocolControl->sendInterlayerMessages(&subFrameStartMessage[0], subFrameStartMessage.size());
                transmissionProtocol->sendPackagesToL1(macPdus);
                protocolControl->sendInterlayerMessages(&subFrameEndMessage[0], subFrameEndMessage.size());
            }
        }
    }
    //Change MAC Tx Mode to DISABLED_MODE_TX
    currentParameters->setMacTxMode(DISABLED_MODE_TX);
}

uint8_t 
MacController::decoding()
{
    uint8_t macAddress;                         //Source MAC address
    ssize_t numberBytesSdu;                     //Number of bytes of SDU incoming
    char bufferSdu[MAXIMUM_BUFFER_LENGTH];      //Buffer to store SDU incoming
    vector<MacPDU*> bufferPdus;                 //Buffer to store PDUs incoming

    //Read packet from Socket
    receptionProtocol->receivePackageFromL1(bufferPdus, MAXIMUM_BUFFER_LENGTH);

    //Decode PDUs
    while(bufferPdus.size()>0){
        //Get MAC Address from MAC header
        macAddress = (bufferPdus[0]->mac_data_[0]>>4)&15;

        if(flagBS){ //If it is BS, analyze SNR and verify if it is needed to change MCSUL
            if(verbose) cout<<"[MacController] Decoding MAC Address "<<(int)macAddress<<": in progress..."<<endl;

            //Analyze Average SNR from MACPDU to see if it is necessary to update MCSDL
            //Calculates new UL MCS and sets it based on SNR received
            cliL2Interface->dynamicParameters->setMcsUplink(macAddress, LinkAdaptation::getSnrConvertToMcs(bufferPdus[0]->snr_avg_));
            
            //If new MCS is different from old, enter RECONFIG mode
            if(cliL2Interface->dynamicParameters->getMcsUplink(macAddress)!=currentParameters->getMcsUplink(macAddress)){
                //Changes current MAC mode to RECONFIG
                currentParameters->setMacMode(RECONFIG_MODE);

                //Set flag to indicate that UEs are out-of-date
                currentParameters->setFlagUesOutdated(true);

                if(verbose) cout<<"\n\n[MacController] ___________ System entering RECONFIG mode by System parameters alteration. ___________\n"<<endl;
            }
        }
        else{   //If it is BS, send report to BS

            protocolControl->rxMetrics->rankIndicator = bufferPdus[0]->rankIndicator_;
            protocolControl->rxMetrics->snr_avg = bufferPdus[0]->snr_avg_;
            protocolControl->rxMetricsReport(true);
        }

        //Create Multiplexer object to help unstacking SDUs contained in the PDU
        Multiplexer *multiplexer = new Multiplexer(&(bufferPdus[0]->mac_data_[0]), verbose);

        //Remove MAC Header
        multiplexer->removeMacHeader();

        while((numberBytesSdu = multiplexer->getSDU(bufferSdu))>0){
            //Test if it is Control SDU
            if(multiplexer->getCurrentDataControlFlag()==0)
                protocolControl->decodeControlSdus(bufferSdu, numberBytesSdu, macAddress);
            else{    //Data SDU
            if(verbose) cout<<"[MacController] Data SDU received. Forwarding to L3."<<endl; 
                transmissionProtocol->sendPackageToL3(bufferSdu, numberBytesSdu);
            }
            bzero(bufferSdu, MAXIMUM_BUFFER_LENGTH);
        }

        //Delete multiplexer and erase first position of vector
        delete multiplexer;
        delete bufferPdus[0];
        bufferPdus.erase(bufferPdus.begin());
    }

    return macAddress;
}
