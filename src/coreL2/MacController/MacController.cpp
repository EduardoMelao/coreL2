/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacController.cpp
@Classification : MAC Controller
@
@Last alteration : March 13th, 2020
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
    delete scheduler;
    delete protocolControl;
    delete sduBuffers;
    delete receptionProtocol;
    delete transmissionProtocol;
    delete tunInterface;
    delete l1l2Interface;
    delete [] threads;
    delete ipMacTable;

    //Delete current system parameters only shutting down MAC
    //In STOP_MODE, there's no need to destroy system parameters and CLI interface
    if(currentMacMode!=STOP_MODE){
    	delete cliL2Interface;
    	delete currentParameters;
    }
}

void
MacController::initialize(){
    currentMacMode = STANDBY_MODE;      //Initializes MAC in STANDBY_MODE 
    cout<<"\n\n[MacController] ___________ System entering STANDBY mode. ___________\n"<<endl;
    manager();
}

void
MacController::manager(){
    //Infinite loop
    while(1){
        switch(currentMacMode){
            case STANDBY_MODE:
            {
                //System waits for MacStartCommand
                if(cliL2Interface->getMacStartCommandSignal()){
                    cliL2Interface->setMacStartCommandSignal(false);
                    currentMacMode = CONFIG_MODE;
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
                sduBuffers = new SduBuffers(receptionProtocol, currentParameters, ipMacTable, verbose);

                //Create Scheduler to make Spectrum and SDU scheduling
                scheduler = new Scheduler(currentParameters, sduBuffers, verbose);

                //Threads definition
                /** Threads order:
                 * 0    ---> ProtocolData MACD SDU enqueueing (From L3)
                 * 1    ---> Reading control messages from PHY
                 * 2    ---> Scheduling SDUs
                 */
                threads = new thread[3];

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

                //#TODO: Send PHYConfig.Request here!

                //Set MAC mode to start mode
                currentMacMode = START_MODE;

                cout<<"\n\n[MacController] ___________ System entering START mode. ___________\n"<<endl;
            }
            break;

            case START_MODE:
            {
                //Here, all system threads that don't execute only in IDLE_MODE are started.
                startThreads();

                //Set MAC mode to start mode
                currentMacMode = IDLE_MODE;

                cout<<"\n\n[MacController] ___________ System entering IDLE mode. ___________\n"<<endl;
            }
            break;

            case IDLE_MODE:
            {
                //System will continue to execute idle threads (receiving from L1 or L3) and wait for other commands e.g MacConfigRequestCommand or MacStopCommand

                //In BS, check for ConfigRequest or Stop commands. In UE, check onlu for Stop commands
                if(flagBS){     //On BS
                    if(cliL2Interface->getMacConfigRequestCommandSignal()){           //MacConfigRequest
                        currentMacMode = RECONFIG_MODE;                                 //Change mode

                        //Set flag to indicate that UEs are out-of-date
                        currentParameters->setFlagUesOutdated(true);

                        cout<<"\n\n[MacController] ___________ System entering RECONFIG mode by CLI command. ___________\n"<<endl;
                    }
                    else{ 
                        if(cliL2Interface->getMacStopCommandSignal()){                //Mac Stop
                            cliL2Interface->setMacStopCommandSignal(false);           //Reset flag
                            currentMacMode = STOP_MODE;                                 //Change mode

                            cout<<"\n\n[MacController] ___________ System entering STOP mode. ___________\n"<<endl;
                        }
                    }
                }
                else{       //On UE
                    if(cliL2Interface->getMacStopCommandSignal()){                    //Mac Stop  
                        cliL2Interface->setMacStopCommandSignal(false);               //Reset flag
                        currentMacMode = STOP_MODE;                                     //Change mode

                        cout<<"\n\n[MacController] ___________ System entering STOP mode. ___________\n"<<endl;
                    }
                }
            }
            break;

            case RECONFIG_MODE:
            {
                //To enter RECONFIG_MODE, TX and RX must be disabled
                if(currentMacRxMode==DISABLED_MODE_RX && currentMacTxMode==DISABLED_MODE_TX){

                    //System will update current parameters with dynamic parameters
                    if(!flagBS){    //If it is UE, parameters to update are CLI's and ULMCS
                        currentParameters->setUEParameters(cliL2Interface->dynamicParameters);
                    }
                    else            //If it is BS, it is needed to test if the parameters changed are CLI's or system's
                    {
                        if(cliL2Interface->getMacConfigRequestCommandSignal())  //CLI parameters changed
                            currentParameters->setCLIParameters(cliL2Interface->dynamicParameters);
                        else    //System parameters changed           
                           currentParameters->setSystemParameters(cliL2Interface->dynamicParameters);
                        
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
                    currentMacMode = IDLE_MODE;

                    if(verbose) cout<<"[MacController] Current Parameters updated correctly."<<endl;

                    cout<<"\n\n[MacController] ___________ System entering IDLE mode. ___________\n"<<endl;
                }
            }
            break;

            case STOP_MODE:
            {
                //To enter RECONFIG_MODE, TX, RX and Tun modes must be disabled
                if(currentMacRxMode==DISABLED_MODE_RX && currentMacTxMode==DISABLED_MODE_TX && currentMacTunMode==TUN_DISABLED){
                    //Destroy all System environment variables
                    this->~MacController();

                    //System will stand in STANDBY mode until it is started again
                    currentMacMode = STANDBY_MODE;

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
    threads[0] = thread(&SduBuffers::enqueueingDataSdus, sduBuffers,  ref(currentMacMode), ref(currentMacTunMode));

    //Control messages from PHY reading (only IDLE mode)
    threads[1] = thread(&ProtocolControl::receiveInterlayerMessages, protocolControl, ref(currentMacMode), ref(currentMacRxMode));

    //#TODO: SCHEDULER
    threads[2] = thread(&MacController::scheduling, this);

    //Join all threads
    for(int i=0;i<3;i++){
        //Join all threads IDLE mode 
        threads[i].detach();
    }

    if(verbose) cout<<"[MacController] Threads started successfully."<<endl;
}

void
MacController::scheduling(){

    while(currentMacMode!=STOP_MODE){
        if(currentMacMode==IDLE_MODE){
            currentMacTxMode = ACTIVE_MODE_TX;
            if(sduBuffers->bufferStatusInformation()){
                //Create array of 2 pointers to MacPDU objects
                MacPDU* macPdus[2];
                macPdus[0] = new MacPDU();
                macPdus[1] = new MacPDU();  //In case it is BS, 2 MAC PDUs are required by the scheduler

                //Schedule Spectrum and SDUs into PDU(s)
                if(flagBS){
                    scheduler->scheduleRequestBS(macPdus);
                }
                else
                    scheduler->scheduleRequestUE(macPdus[0]);
                
                //Get number of PDUs
                int numberPdus = macPdus[1]->mac_data_.size()==0 ? 1:2;

                //Create SubframeTx.Start message
                string messageParameters;		            //This string will contain the parameters of the message
                vector<uint8_t> messageParametersBytes;	    //Vector to receive serialized parameters structure

                if(flagBS){     //Create BSSubframeTx.Start message
                    BSSubframeTx_Start messageBS;	//Message parameters structure

                    //Fill the structure with information
                    messageBS.numUEs = currentParameters->getNumberUEs();
                    messageBS.numPDUs = numberPdus;      //If seconds MACPDU is empty, there's just one MAC PDU
                    messageBS.fLutDL = currentParameters->getFLUTMatrix();
                    currentParameters->getUlReservations(messageBS.ulReservations);
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
                transmissionProtocol->sendPackagesToL1(macPdus, numberPdus);
                protocolControl->sendInterlayerMessages(&subFrameEndMessage[0], subFrameEndMessage.size());
            }
        }
        else{
            //Change MAC Tx Mode to DISABLED_MODE_TX
            currentMacTxMode = DISABLED_MODE_TX;
        }
        this_thread::sleep_for(chrono::nanoseconds(1));
    }
    if(verbose) cout<<"[MacController - Scheduling] Entering STOP_MODE."<<endl;    
    //Change MAC Tx Mode to DISABLED_MODE_TX before stopping System
    currentMacTxMode = DISABLED_MODE_TX;
}

uint8_t 
MacController::decoding()
{
    uint8_t macAddress;                         //Source MAC address
    ssize_t numberBytesSdu;                     //Number of bytes of SDU incoming
    char bufferSdu[MAXIMUM_BUFFER_LENGTH];      //Buffer to store SDU incoming
    vector<vector<uint8_t>> bufferPdus;         //Buffer to store PDUs incoming

    //Read packet from Socket
    receptionProtocol->receivePackageFromL1(bufferPdus, MAXIMUM_BUFFER_LENGTH);

    //Decode PDUs
    while(bufferPdus.size()>0){
        //Get MAC Address from MAC header
        macAddress = (bufferPdus[0][1]>>4)&15;

        if(verbose) cout<<"[MacController] Decoding MAC Address "<<(int)macAddress<<": in progress..."<<endl;

        //Create Multiplexer object to help unstacking SDUs contained in the PDU
        Multiplexer *multiplexer = new Multiplexer(&(bufferPdus[0][0]), verbose);

        //Remove MAC Header
        multiplexer->removeMacHeader();

        while((numberBytesSdu = multiplexer->getSDU(bufferSdu))>0){
            //Test if it is Control SDU
            if(multiplexer->getCurrentDataControlFlag()==0)
                protocolControl->decodeControlSdus(currentMacMode, bufferSdu, numberBytesSdu, macAddress);
            else{    //Data SDU
            if(verbose) cout<<"[MacController] Data SDU received. Forwarding to L3."<<endl; 
                transmissionProtocol->sendPackageToL3(bufferSdu, numberBytesSdu);
            }
            bzero(bufferSdu, MAXIMUM_BUFFER_LENGTH);
        }

        //Delete multiplexer and erase first position of vector
        delete multiplexer;
        bufferPdus.erase(bufferPdus.begin());
    }

    return macAddress;
}
