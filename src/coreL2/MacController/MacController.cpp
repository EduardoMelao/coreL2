/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacController.cpp
@Classification : MAC Controller
@
@Last alteration : March 10th, 2020
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

                //Perform MACC SDU construction to send to all UEs with actual system information
                vector<uint8_t> dynamicParametersBytes;

                //Enqueue a MACC SDU to each UE attached
                for(int i=0;i<currentParameters->getNumberUEs();i++){
                    dynamicParametersBytes.clear();
                    currentParameters->serialize(currentParameters->getMacAddress(i), dynamicParametersBytes);
                    sduBuffers->enqueueControlSdu(&(dynamicParametersBytes[0]), dynamicParametersBytes.size(), currentParameters->getMacAddress(i));
                }

                //Define IP-MAC correlation table creating and initializing a MacAddressTable with static informations (HARDCODE)
                ipMacTable = new MacAddressTable(verbose);
                uint8_t addressEntry0[4] = {10,0,0,10};
                uint8_t addressEntry1[4] = {10,0,0,11};
                uint8_t addressEntry2[4] = {10,0,0,12};
                ipMacTable->addEntry(addressEntry0, 0);
                ipMacTable->addEntry(addressEntry1, 1);
                ipMacTable->addEntry(addressEntry2, 2);
                
                //Create Tun Interface and allocate it
                tunInterface = new TunInterface(deviceNameTun, verbose);
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

                //Threads definition
                /** Threads order:
                 * 0    ---> ProtocolData MACD SDU enqueueing (From L3)
                 * 1    ---> Reading control messages from PHY
                 * 2    ---> Scheduling SDUs
                 */
                threads = new thread[3];

                //Create ProtocolControl to deal with MACC SDUs
                protocolControl = new ProtocolControl(this, verbose);

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
    threads[2] = thread(&MacController::provisionalScheduling, this);

    //Join all threads
    for(int i=0;i<3;i++){
        //Join all threads IDLE mode 
        threads[i].detach();
    }

    if(verbose) cout<<"[MacController] Threads started successfully."<<endl;
}

void
MacController::provisionalScheduling(){
    char bufferSdu[MAXIMUM_BUFFER_LENGTH];  //Buffer to store aggregated SDUs
    ssize_t numberBytesRead = 0;            //Size of MAC SDU read in Bytes
    uint8_t macAddress;
    uint16_t maxNumberBytes = 1500;

    while(currentMacMode!=STOP_MODE){
        if(currentMacMode==IDLE_MODE){
            currentMacTxMode = ACTIVE_MODE_TX;
            for(int i=0;i<currentParameters->getNumberUEs();i++){
                macAddress = flagBS? currentParameters->getMacAddress(i) : 0;
                if(sduBuffers->bufferStatusInformation(macAddress)){
                    //Fulfill bufferData with zeros 
                    bzero(bufferSdu, MAXIMUM_BUFFER_LENGTH);

                    Multiplexer* mux = new Multiplexer(1, currentMacAddress, &macAddress, &maxNumberBytes, verbose);

                    //Gets next SDU from SduBuffers. Prority for MACC SDUs
                    if(sduBuffers->getNumberControlSdus(macAddress)){
                        numberBytesRead = sduBuffers->getNextControlSdu(macAddress, bufferSdu);
                        mux->addSdu(bufferSdu, numberBytesRead, 0, macAddress);
                    }
                    else{
                        numberBytesRead = sduBuffers->getNextDataSdu(macAddress, bufferSdu);
                        mux->addSdu(bufferSdu, numberBytesRead, 1, macAddress);
                    }

                    sendPdu(mux, macAddress);
                }
            }
        }
        else{
            //Change MAC Tx Mode to DISABLED_MODE_TX
            currentMacTxMode = DISABLED_MODE_TX;
        }
    }
    if(verbose) cout<<"[Scheduller] Entering STOP_MODE."<<endl;    
    //Change MAC Tx Mode to DISABLED_MODE_TX before stopping System
    currentMacTxMode = DISABLED_MODE_TX;
}

void 
MacController::sendPdu(
    Multiplexer* mux,       //Multiplexer object containing multiplexed SDUs
    uint8_t macAddress)     //Destination MAC Address of AggregationQueue in the Multiplexer
{
    //Declaration of PDU buffers: data and control
    char bufferPdu[MAXIMUM_BUFFER_LENGTH];
    bzero(bufferPdu, MAXIMUM_BUFFER_LENGTH);

    //Gets PDU from multiplexer
    ssize_t numberDataBytesRead = mux->getPdu(bufferPdu, macAddress);

    //Fill MAC PDU with information 
    setMacPduStaticInformation(numberDataBytesRead, macAddress);
    macPDU.mac_data_.assign(bufferPdu, bufferPdu+numberDataBytesRead);
    macPDU.allocation_.target_ue_id = macAddress;
    macPDU.mcs_.num_info_bytes = numberDataBytesRead;
    macPDU.allocation_.number_of_rb = get_num_required_rb(macPDU.numID_, macPDU.mimo_, macPDU.mcs_.modulation, 3/4 , numberDataBytesRead*8);

    //Create SubframeTx.Start message
    string messageParameters;		            //This string will contain the parameters of the message
	vector<uint8_t> messageParametersBytes;	    //Vector to receive serialized parameters structure

    if(flagBS){     //Create BSSubframeTx.Start message
    	BSSubframeTx_Start messageBS;	//Message parameters structure

        //Fill the structure with information
    	messageBS.numUEs = currentParameters->getNumberUEs();
    	messageBS.numPDUs = 1;
        messageBS.fLutDL = currentParameters->getFLUTMatrix();
        currentParameters->getUlReservations(messageBS.ulReservations);
    	messageBS.numerology = currentParameters->getNumerology();
    	messageBS.ofdm_gfdm = currentParameters->isGFDM()? 1:0;
    	messageBS.rxMetricPeriodicity = currentParameters->getRxMetricsPeriodicity();

        //Serialize struct
    	messageBS.serialize(messageParametersBytes);

        //Copy structure bytes to message
    	for(uint i=0;i<messageParametersBytes.size();i++)
    		messageParameters+=messageParametersBytes[i];
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

        //Copy struct bytes to message
        for(uint i=0;i<messageParametersBytes.size();i++)
            messageParameters+=messageParametersBytes[i];
    }

    //Downlink routine:
    string subFrameStartMessage = flagBS? "1":"2";
    string subFrameEndMessage = "3";
    
    //Add parameters to original message
    subFrameStartMessage+=messageParameters;

    //Send interlayer messages and the PDU
    protocolControl->sendInterlayerMessages(&subFrameStartMessage[0], subFrameStartMessage.size());
    transmissionProtocol->sendPackageToL1(macPDU, macAddress);
    protocolControl->sendInterlayerMessages(&subFrameEndMessage[0], subFrameEndMessage.size());
    
    //Deletes multiplexer object
    delete mux;
}

uint8_t 
MacController::decoding()
{
    uint8_t macAddress;                     //Source MAC address
    char buffer[MAXIMUM_BUFFER_LENGTH];     //Buffer to store message incoming

    //Clear buffer
    bzero(buffer,sizeof(buffer));

    //Read packet from Socket
    ssize_t numberDecodingBytes = receptionProtocol->receivePackageFromL1(buffer, MAXIMUM_BUFFER_LENGTH);

    //Error checking
    if(numberDecodingBytes==-1 && verbose){ 
        cout<<"[MacController] Error reading from socket."<<endl;
        return 0;
    }

    //CRC checking
    if(numberDecodingBytes==-2 && verbose){ 
        cout<<"[MacController] Drop packet due to CRC Error."<<endl;
        return 0;
    }

    //EOF checking
    if(numberDecodingBytes==0 && verbose){ 
        cout<<"[MacController] End of Transmission."<<endl;
        return 0;
    }

    //Get MAC Address from MAC header
    macAddress = (buffer[0]>>4)&15;
    
    if(verbose) cout<<"[MacController] Decoding MAC Address "<<(int)macAddress<<": in progress..."<<endl;

    //Create ProtocolPackage object to help removing Mac Header
    ProtocolPackage pdu(buffer, numberDecodingBytes , verbose);
    pdu.removeMacHeader();

    //Create AggregationQueue object to help unstacking SDUs contained in the PDU
    AggregationQueue *aggregationQueue = pdu.getMultiplexedSDUs();
    while((numberDecodingBytes = aggregationQueue->getSDU(buffer))>0){
        //Test if it is Control SDU
        if(aggregationQueue->getCurrentDataControlFlag()==0)
            protocolControl->decodeControlSdus(currentMacMode, buffer, numberDecodingBytes, macAddress);
        else{    //Data SDU
        if(verbose) cout<<"[MacController] Data SDU received. Forwarding to L3."<<endl; 
            transmissionProtocol->sendPackageToL3(buffer, numberDecodingBytes);
        }
    }
    delete aggregationQueue;
    
    return macAddress;
}

void
MacController::setMacPduStaticInformation(
    size_t numberBytes,         //Number of Data Bytes in the PDU
    uint8_t macAddress)         //Destination MAC Address
{
    //Static information:
    unsigned numerologyID = 2;                  //Numerology identification
    float codeRate = 3/4;                       //Core rate used in codification

    //Define Structures
    mimo_cfg_t mimoConfiguration;               //MIMO configuration structure
    mcs_cfg_t mcsConfiguration;                 //Modulation Coding Scheme configuration
    allocation_cfg_t allocationConfiguration;   //Resource allocation configuration
    macphyctl_t macPhyControl;                  //MAC-PHY control structure

    //MIMO Configuration
    mimoConfiguration.scheme = currentParameters->getMimoConf(macAddress)==0? NONE:(currentParameters->getMimoDiversityMultiplexing(macAddress)==0? DIVERSITY:MULTIPLEXING);
    mimoConfiguration.num_tx_antenas = currentParameters->getMimoAntenna(macAddress)==0? 2:4;
    mimoConfiguration.precoding_mtx = currentParameters->getMimoPrecoding(macAddress);

    //MCS Configuration
    mcsConfiguration.num_info_bytes = currentParameters->getMTU();
    mcsConfiguration.num_coded_bytes = currentParameters->getMTU()/codeRate;
    mcsConfiguration.modulation = QAM64;
    mcsConfiguration.power_offset = currentParameters->getTPC(macAddress);

    //Resource allocation configuration
    allocationConfiguration.first_rb = 0;
    allocationConfiguration.number_of_rb = get_num_required_rb(numerologyID, mimoConfiguration, mcsConfiguration.modulation, codeRate, numberBytes*8);
    allocationConfiguration.target_ue_id = macAddress;

    //MAC-PHY Control
    macPhyControl.first_tb_in_subframe = true;
    macPhyControl.last_tb_in_subframe = true;
    macPhyControl.sequence_number = 1;
    macPhyControl.subframe_number = 3;

    //MAC PDU object definition
    macPDU.allocation_ = allocationConfiguration;
    macPDU.mimo_ = mimoConfiguration;
    macPDU.mcs_ = mcsConfiguration;
    macPDU.macphy_ctl_ = macPhyControl;
}
