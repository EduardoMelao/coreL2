/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacController.cpp
@Classification : MAC Controller
@
@Last alteration : January 16th, 2020
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module creates and starts TUN Interface reading, Control 
    SDUs creation, Sending timeout control, and decoding threads. Also, management of
    SDUs concatenation into PDU is made by this module.
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
}

MacController::~MacController(){
    delete [] rxMetrics;
    delete macConfigRequest;
    delete staticParameters;
    delete protocolControl;
    delete protocolData;
    delete mux;
    delete macHigh;
    delete receptionProtocol;
    delete transmissionProtocol;
    delete tunInterface;
    delete l1l2Interface;
    delete [] threads;
    delete [] queueConditionVariables;
    delete ipMacTable;
    delete staticParameters;
}

void
MacController::initialize(){
    currentMacMode = STANDBY_MODE;      //Initializes MAC in STANDBY_MODE 
    manager();
}

void
MacController::recordDynamicParameters(){
    //PROVISIONAL IMPLEMENTATION, considering only one UE
    //Copy values from Dynamic Parameters to Static Parameters
    macConfigRequest->dynamicParameters->getUlReservations(staticParameters->ulReservations);
    if(flagBS) staticParameters->mcsDownlink[0] = macConfigRequest->dynamicParameters->getMcsDownlink(staticParameters->ulReservations[0].target_ue_id);
    staticParameters->mcsUplink[0] = macConfigRequest->dynamicParameters->getMcsUplink(staticParameters->ulReservations[0].target_ue_id);
    staticParameters->mimoConf[0]=macConfigRequest->dynamicParameters->getMimoConf(staticParameters->ulReservations[0].target_ue_id);
    staticParameters->mimoDiversityMultiplexing[0]=macConfigRequest->dynamicParameters->getMimoDiversityMultiplexing(staticParameters->ulReservations[0].target_ue_id);
    staticParameters->mimoAntenna[0]=macConfigRequest->dynamicParameters->getMimoAntenna(staticParameters->ulReservations[0].target_ue_id);
    staticParameters->mimoOpenLoopClosedLoop[0]=macConfigRequest->dynamicParameters->getMimoOpenLoopClosedLoop(staticParameters->ulReservations[0].target_ue_id);
    staticParameters->mimoPrecoding[0]=macConfigRequest->dynamicParameters->getMimoPrecoding(staticParameters->ulReservations[0].target_ue_id);
    staticParameters->transmissionPowerControl[0] = macConfigRequest->dynamicParameters->getTPC(staticParameters->ulReservations[0].target_ue_id);
    staticParameters->rxMetricPeriodicity[0] = macConfigRequest->dynamicParameters->getRxMetricsPeriodicity(staticParameters->ulReservations[0].target_ue_id);
}

void
MacController::manager(){
    //Infinite loop
    while(1){
        switch(currentMacMode){
            case STANDBY_MODE:
            {
                //Provisional. Here, system waits for MacStartCommand
                cout<<"[MacController] ___________ System in STANDBY mode. ___________ \n Press any key to start functionalities (MacStartCommand)..."<<endl;
                cin.get();
                currentMacMode = CONFIG_MODE;
            }
            break;

            case CONFIG_MODE:
            {
                //All MAC Initial Configuration is made here
                cout<<"[MacController] ___________ System in CONFIG mode. ___________"<<endl; 

                //Read txt static and default parameters and initialize flagBS and currentMacAddress values
                staticParameters = new StaticDefaultParameters(verbose);
                staticParameters->readTxtStaticParameters();
                flagBS = staticParameters->flagBS;
                currentMacAddress = flagBS? 0:staticParameters->ulReservations[0].target_ue_id;
                
                //Define IP-MAC correlation table creating and initializing a MacAddressTable with static informations (HARDCODE)
                ipMacTable = new MacAddressTable(verbose);
                uint8_t addressEntry0[4] = {10,0,0,10};
                uint8_t addressEntry1[4] = {10,0,0,11};
                uint8_t addressEntry2[4] = {10,0,0,12};
                ipMacTable->addEntry(addressEntry0, 0, true);
                ipMacTable->addEntry(addressEntry1, 1, false);
                ipMacTable->addEntry(addressEntry2, 2, false);

                //Create condition variables
                queueConditionVariables = new condition_variable[staticParameters->numberUEs];
                
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

                //Create MACHigh queue to store IP packets received from TUN
                macHigh = new MacHighQueue(receptionProtocol, verbose);

                //Threads definition
                /** Threads order:
                 * 0 .. numberEquipments-1    ---> Timeout control threads
                 * numberEquipments           ---> ProtocolData MACD SDU enqueueing (From L3)
                 * numberEquipments+1         ---> Data SDU enqueueing from TUN interface in MacHighQueue
                 * numberEquipments+2         ---> Reading control messages from PHY
                 */
                threads = new thread[3+staticParameters->numberUEs];

                //Create Multiplexer and set its TransmissionQueues
                mux = new Multiplexer(staticParameters->mtu[0], currentMacAddress, ipMacTable, MAXSDUS, flagBS, verbose);     //PROVISIONAL UNIVERSAL MTU
                if(flagBS){
                    for(int i=0;i<staticParameters->numberUEs;i++)
                        mux->setTransmissionQueue(staticParameters->ulReservations[i].target_ue_id);
                }
                else mux->setTransmissionQueue(0);      //UE needs a single Transmission Queue to BS

                //Create ProtocolData to deal with MACD SDUs
                protocolData = new ProtocolData(this, macHigh, verbose);

                //Create ProtocolControl to deal with MACC SDUs
                protocolControl = new ProtocolControl(this, verbose);

                //Initialize Dynamic Parameters class
                macConfigRequest = new MacConfigRequest(verbose);

                //Fill dynamic Parameters with default parameters (stating system)
                staticParameters->loadDynamicParametersDefaultInformation(macConfigRequest->dynamicParameters);

                //Create a RxMetrics array
                rxMetrics = new RxMetrics[staticParameters->numberUEs];

                //Set subframe counter to zero
                subframeCounter = 0;

                //---------------PROVISIONAL: NEED TO SEND PHYConfig.Request here!--------------------------

                //Set MAC mode to start mode
                currentMacMode = START_MODE;
            }
            break;

            case START_MODE:
            {
                //Here, all system threads that don't execute only in IDLE_MODE are started.
                cout<<"[MacController] ___________ System in START mode. ___________"<<endl;
                startThreads();

                //Set MAC mode to start mode
                currentMacMode = IDLE_MODE;
            }
            break;

            case IDLE_MODE:
            {
                //System will join idle threads (receiving from L1 or L3) and wait for other commands e.g MacConfigRequestCommand or MacStopCommand
                cout<<"[MacController] ___________ System in IDLE mode. ___________"<<endl;
                
                //Join idle threads
                threads[staticParameters->numberUEs].join();    //ProtocolData MACD SDU enqueueing (From L3) thread
                threads[staticParameters->numberUEs+2].join();  //Reading control messages from PHY thread (From L1)

                //PROVISIONAL: query user to change state to RECONFIG_MODE or STOP_MODE
                cout<<"[MacController] Enter '%' for ConfigRequest or '*' for Stop."<<endl;
                char caracter;  //Caracter received from user
                cin>>caracter;
                while(caracter!='%'&&caracter!='*'){
                    cin>>caracter;
                }

                if(caracter=='%')
                    currentMacMode = RECONFIG_MODE;
                else
                    currentMacMode = STOP_MODE;
            }
            break;

            case RECONFIG_MODE:
            {
                cout<<"[MacController] ___________ System in RECONFIG mode. ___________"<<endl;

                //System will update static parameters
                recordDynamicParameters();

                //Then, if it is BS, it will send Dynamic Parameters to UE via MACC SDU for reconfiguration
                if(flagBS){
                    vector<uint8_t> dynamicParametersBytes;

                    //Send a MACC SDU to each UE attached
                    for(int i=0;i<staticParameters->numberUEs;i++){
                        dynamicParametersBytes.clear();
                        macConfigRequest->dynamicParameters->serialize(staticParameters->ulReservations[i].target_ue_id, dynamicParametersBytes);
                        protocolControl->enqueueControlSdus(&(dynamicParametersBytes[0]), dynamicParametersBytes.size(), staticParameters->ulReservations[i].target_ue_id);
			        }

                    currentMacMode = IDLE_MODE;
                }

                //Set MAC mode back to idle mode
                currentMacMode = IDLE_MODE;
            }
            break;

            case STOP_MODE:
            {
                cout<<"[MacController] ___________ System in STOP mode. ___________"<<endl;

                //Destroy all System environment variables
                this->~MacController();

                //System will stand in STANDBY mode until it is started again
                currentMacMode = STANDBY_MODE;
            }
            break;

            default:
            {
                if(verbose) cout<<"[MacController] ___________Unknown mode ___________"<<endl;
            }
            break;
        }
    }
}

void 
MacController::startThreads(){
    int i;   	//Auxiliary variable for loops

    //For each equipment
    for(i=0;i<staticParameters->numberUEs;i++){
        //timeoutController threads - threads[0 .. attachedEquipments-1]
        threads[i] = thread(&MacController::timeoutController, this, i);
    }

    //TUN queue control thread (only IDLE mode)
    threads[i] = thread(&ProtocolData::enqueueDataSdus, protocolData, ref(currentMacMode));

    //TUN reading and enqueueing thread
    threads[i+1] = thread(&MacHighQueue::reading, macHigh);

    //Control messages from PHY reading (only IDLE mode)
    threads[i+2] = thread(&ProtocolControl::receiveInterlayerMessages, protocolControl, ref(currentMacMode));

    //Join all threads
    int numberThreads = 3+staticParameters->numberUEs;
    for(i=0;i<numberThreads;i++){
        //Join all threads that don't execute only in IDLE mode
        if(i!=staticParameters->numberUEs&&i!=staticParameters->numberUEs+2)    
            threads[i].join();
    }
}

void 
MacController::sendPdu(
    uint8_t macAddress)     //Destination MAC Address of TransmissionQueue in the Multiplexer
{
    //Declaration of PDU buffers: data and control
    char bufferPdu[MAXIMUM_BUFFER_LENGTH];
    char bufferControl[MAXIMUM_BUFFER_LENGTH];
    bzero(bufferPdu, MAXIMUM_BUFFER_LENGTH);
    bzero(bufferControl, MAXIMUM_BUFFER_LENGTH);

    //Gets PDU from multiplexer
    ssize_t numberDataBytesRead = mux->getPdu(bufferPdu, macAddress);

    //Creates a Control Header to this PDU and inserts it
    MacCtHeader macControlHeader(flagBS, verbose);
    ssize_t numberControlBytesRead = macControlHeader.getControlData(bufferControl);

    //Fill MAC PDU with information 
    setMacPduStaticInformation(numberDataBytesRead, macAddress);
    macPDU.mac_data_.assign(bufferPdu, bufferPdu+numberDataBytesRead);
    macPDU.control_data_.assign(bufferControl, bufferControl+numberControlBytesRead);
    macPDU.allocation_.target_ue_id = macAddress;
    macPDU.mcs_.num_info_bytes = numberDataBytesRead;
    macPDU.allocation_.number_of_rb = get_num_required_rb(macPDU.numID_, macPDU.mimo_, macPDU.mcs_.modulation, 3/4 , numberDataBytesRead*8);

    //Create SubframeTx.Start message
    string messageParameters;		            //This string will contain the parameters of the message
	vector<uint8_t> messageParametersBytes;	    //Vector to receive serialized parameters structure

    if(flagBS){     //Create BSSubframeTx.Start message
    	BSSubframeTx_Start messageBS;	//Message parameters structure

        //Fill the structure with information
    	messageBS.numUEs = staticParameters->numberUEs;
    	messageBS.numPDUs = 1;
    	for(int i=0;i<17;i++)
    		messageBS.fLutDL[i] = staticParameters->fLutMatrix[i];
    	messageBS.ulReservations = staticParameters->ulReservations;
    	messageBS.numerology = staticParameters->numerology[0];
    	messageBS.ofdm_gfdm = staticParameters->ofdm_gfdm[0];
    	messageBS.rxMetricPeriodicity = staticParameters->rxMetricPeriodicity[0];

        //Serialize struct
    	messageBS.serialize(messageParametersBytes);

        //Copy structure bytes to message
    	for(uint i=0;i<messageParametersBytes.size();i++)
    		messageParameters+=messageParametersBytes[i];
    }
    else{       //Create UESubframeTx.Start message
        UESubframeTx_Start messageUE;	//Messages parameters structure

        //Fill the structure with information
        messageUE.ulReservation = staticParameters->ulReservations[0];
        messageUE.numerology = staticParameters->numerology[0];
        messageUE.ofdm_gfdm = staticParameters->ofdm_gfdm[0];
        messageUE.rxMetricPeriodicity = staticParameters->rxMetricPeriodicity[0];

        //Serialize struct
        messageUE.serialize(messageParametersBytes);

        //Copy struct bytes to message
        for(uint i=0;i<messageParametersBytes.size();i++)
            messageParameters+=messageParametersBytes[i];
    }

    //Downlink routine:
    string subFrameStartMessage = flagBS? "BS":"UE";
    subFrameStartMessage+="SubframeTx.Start";
    string subFrameEndMessage = flagBS? "BS":"UE";
    subFrameEndMessage += "SubframeTx.End";
    
    //Add parameters to original message
    subFrameStartMessage+=messageParameters;

    //Send interlayer messages and the PDU
    protocolControl->sendInterlayerMessages(&subFrameStartMessage[0], subFrameStartMessage.size());
    transmissionProtocol->sendPackageToL1(macPDU, macAddress);
    protocolControl->sendInterlayerMessages(&subFrameEndMessage[0], subFrameEndMessage.size());
}

void 
MacController::timeoutController(
    int index)      //Index that identifies the condition variable and destination MAC Address of a queue 
{
    //Timeout declaration: static
    chrono::nanoseconds timeout = chrono::milliseconds(staticParameters->ipTimeout[index]);    //ms

    //Communication infinite loop
    while(1){
        //Lock mutex one time to read
        unique_lock<mutex> lk(queueMutex);
        
        //If there's no timeout OR the PDU is empty, no transmission is necessary
        if(queueConditionVariables[index].wait_for(lk, timeout)==cv_status::no_timeout || mux->emptyPdu(flagBS? staticParameters->ulReservations[index].target_ue_id:0)) 
            continue; 
        //Else, perform PDU transmission
        if(verbose) cout<<"[MacController] Timeout!"<<endl;
        sendPdu(flagBS? staticParameters->ulReservations[index].target_ue_id : 0);
    }
}

uint8_t 
MacController::decoding()
{
    uint8_t macAddress;                     //Source MAC address
    char buffer[MAXIMUM_BUFFER_LENGTH];     //Buffer to store message incoming

    //Clear buffer
    bzero(buffer,sizeof(buffer));

    //Read packet from Socket
    ssize_t numberDecodingBytes = receptionProtocol->receivePackageFromL1(buffer, MAXIMUM_BUFFER_LENGTH, macAddress);

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

    //Create TransmissionQueue object to help unstacking SDUs contained in the PDU
    TransmissionQueue *transmissionQueue = pdu.getMultiplexedSDUs();
    while((numberDecodingBytes = transmissionQueue->getSDU(buffer))>0){
        //Test if it is Control SDU
        if(transmissionQueue->getCurrentDataControlFlag()==0)
            protocolControl->decodeControlSdus(buffer, numberDecodingBytes, macAddress);
        else    //Data SDU
            protocolData->decodeDataSdus(buffer, numberDecodingBytes);
    }
    delete transmissionQueue;

    //If it is UE, increase subframeCounter
    if(!flagBS){    
        subframeCounter++;
        if(subframeCounter==macConfigRequest->dynamicParameters->getRxMetricsPeriodicity(currentMacAddress)){
            rxMetricsReport();
            subframeCounter = 0;
        }
    }
    
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
    mimoConfiguration.scheme = staticParameters->mimoConf[0]==0? NONE:(staticParameters->mimoDiversityMultiplexing[0]==0? DIVERSITY:MULTIPLEXING);
    mimoConfiguration.num_tx_antenas = staticParameters->mimoAntenna[0]==0? 2:4;
    mimoConfiguration.precoding_mtx = staticParameters->mimoPrecoding[0];

    //MCS Configuration
    mcsConfiguration.num_info_bytes = staticParameters->mtu[0];
    mcsConfiguration.num_coded_bytes = staticParameters->mtu[0]/codeRate;
    mcsConfiguration.modulation = QAM64;
    mcsConfiguration.power_offset = staticParameters->transmissionPowerControl[0];

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

void 
MacController::managerDynamicParameters(
    uint8_t* bytesDynamicParameters,    //Serialized bytes from MacConfigRequest object
    size_t numberBytes)                 //Number of bytes of serialized information
{
    vector<uint8_t> serializedBytes;        //Vector to be used for deserialization

    for(int i=0;i<numberBytes;i++)
        serializedBytes.push_back(bytesDynamicParameters[i]);   //Copy information form array to vector

    //Deserialize bytes
    macConfigRequest->dynamicParameters->deserialize(serializedBytes);

    if(verbose) cout<<"[MacController] Dynamic Parameters were managed successfully."<<endl;

    //Now, turn MAC into RECONFIG_MODE to reconfigure static parameters
    currentMacMode = RECONFIG_MODE;
}

void 
MacController::rxMetricsReport(){  //This thread executes only on UE
    vector<uint8_t> rxMetricsBytes;     //Array of bytes where RX Metrics will be stored

    //Serialize Rx Metrics
    rxMetrics->serialize(rxMetricsBytes);
    
    if(verbose) cout<<"[MacController] RxMetrics report with size "<<rxMetricsBytes.size()<<" enqueued to BS."<<endl;

    //Enqueue MACC SDU
    protocolControl->enqueueControlSdus(&(rxMetricsBytes[0]), rxMetricsBytes.size(), 0);
	
}

int
MacController::getIndex(
    uint8_t macAddress)
{
    for(int i=0;i<staticParameters->numberUEs;i++)
        if(macAddress==staticParameters->ulReservations[i].target_ue_id)
            return i;
    return -1;
}
