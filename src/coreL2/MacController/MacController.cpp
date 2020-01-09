/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacController.cpp
@Classification : MAC Controller
@
@Last alteration : January 9th, 2020
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
    MacAddressTable* _ipMacTable,   			//MAC address - IP address table
    StaticDefaultParameters* _staticParameters,	//All parameters loaded from file
    bool _verbose)                  			//Verbosity flag
{    
    //Load static parameters from file
    staticParameters = _staticParameters;

    //Assign verbosity flag
    verbose = _verbose;

    //Assign IP <-> MAC table
    ipMacTable = _ipMacTable;

    //Gets BaseStation flag and current MacAddress from static parameters
    flagBS = staticParameters->flagBS;
    macAddress = flagBS? 0:_staticParameters->ulReservations[0].target_ue_id;
    
    //Create condition variables
    queueConditionVariables = new condition_variable[staticParameters->numberUEs];
    
    //Create Tun Interface and allocate it
    tunInterface = new TunInterface(_deviceNameTun, _verbose);
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
    macHigh = new MacHighQueue(receptionProtocol, _verbose);

    //Threads definition
    /** Threads order:
     * 0 .. attachedEquipments-1    ---> Timeout control threads
     * attachedEquipments           ---> ProtocolData MACD SDU enqueueing
     * attachedEquipments+1         ---> Data SDU enqueueing from TUN interface in MacHighQueue
     * attachedEquipments+2         ---> Reading control messages from PHY
     * attachedEquipments+3         ---> (BS Only) Send DynamicParameters to UE when changed
     */
    threads = new thread[4+staticParameters->numberUEs];

    //Create Multiplexer and set its TransmissionQueues
    mux = new Multiplexer(_staticParameters->mtu, macAddress, ipMacTable, MAXSDUS, flagBS, verbose);
    if(flagBS){
    	for(int i=0;i<staticParameters->numberUEs;i++)
    		mux->setTransmissionQueue(staticParameters->ulReservations[i].target_ue_id);
    }
    else mux->setTransmissionQueue(0);      //UE needs a single Transmission Queue to BS

    //Create ProtocolData to deal with MACD SDUs
    protocolData = new ProtocolData(this,macHigh, verbose);

    //Create ProtocolControl to deal with MACC SDUs
    protocolControl = new ProtocolControl(this, verbose);

    //Initialize Dynamic Parameters class
    dynamicParameters = new MacConfigRequest(verbose);

    //Fill dynamic Parameters with static parameters (stating system)
    staticParameters->loadDynamicParametersDefaultInformation(dynamicParameters);
}

MacController::~MacController(){
    delete dynamicParameters;
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
}

void 
MacController::startThreads(){
    int i;   	//Auxiliary variable for loops

    //For each equipment
    for(i=0;i<staticParameters->numberUEs;i++){
        //timeoutController threads - threads[0 .. attachedEquipments-1]
        threads[i] = thread(&MacController::timeoutController, this, i);
    }

    //TUN queue control thread
    threads[i] = thread(&ProtocolData::enqueueDataSdus, protocolData);

    //TUN reading and enqueueing thread
    threads[i+1] = thread(&MacHighQueue::reading, macHigh);

    //Control messages from PHY reading
    threads[i+2] = thread(&ProtocolControl::receiveInterlayerMessages, protocolControl);

    //(Only BS) Manager threads: Send MACC SDUs when Dynamic Parameters are changed
    if(flagBS)
    	threads[i+3] = thread(&MacController::manager, this);

    //Join all threads
    int numberThreads = flagBS? 4+staticParameters->numberUEs:3+staticParameters->numberUEs;
    for(i=0;i<numberThreads;i++)
        threads[i].join();
}

void 
MacController::sendPdu(
    uint8_t macAddress)     //Destination MAC Address of TransmissionQueue in the Multiplexer
{
    //Declaration of PDU buffer
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
    	messageBS.numUEs = staticParameters->numberUEs;
    	messageBS.numPDUs = 1;
    	for(int i=0;i<17;i++)
    		messageBS.fLutDL[i] = dynamicParameters->fLutMatrix[i];
    	messageBS.ulReservations = dynamicParameters->ulReservations;
    	messageBS.numerology = staticParameters->numerology;
    	messageBS.ofdm_gfdm = staticParameters->ofdm_gfdm;
    	messageBS.rxMetricPeriodicity = dynamicParameters->rxMetricPeriodicity;
    	messageBS.serialize(messageParametersBytes);
    	for(uint i=0;i<messageParametersBytes.size();i++)
    		messageParameters+=messageParametersBytes[i];
    }
    else{       //Create UESubframeTx.Start message
        UESubframeTx_Start messageUE;	//Messages parameters structure
        messageUE.ulReservation = dynamicParameters->ulReservations[0];
        messageUE.numerology = staticParameters->numerology;
        messageUE.ofdm_gfdm = staticParameters->ofdm_gfdm;
        messageUE.rxMetricPeriodicity = dynamicParameters->rxMetricPeriodicity;
        messageUE.serialize(messageParametersBytes);
        for(uint i=0;i<messageParametersBytes.size();i++)
            messageParameters+=messageParametersBytes[i];
    }

    //Downlink routine:
    string subFrameStartMessage = flagBS? "BS":"UE";
    subFrameStartMessage+="SubframeTx.Start";
    string subFrameEndMessage = flagBS? "BS":"UE";
    subFrameEndMessage += "SubframeTx.End";
    
    //Add parameters
    subFrameStartMessage+=messageParameters;

    protocolControl->sendInterlayerMessages(&subFrameStartMessage[0], subFrameStartMessage.size());
    transmissionProtocol->sendPackageToL1(macPDU, macAddress);
    protocolControl->sendInterlayerMessages(&subFrameEndMessage[0], subFrameEndMessage.size());
}

void 
MacController::timeoutController(
    int index)      //Index that identifies the condition variable and destination MAC Address of a queue 
{
    //Timeout declaration: static
    chrono::nanoseconds timeout = chrono::milliseconds(staticParameters->ipTimeout);    //ms

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

void 
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
        return;
    }

    //CRC checking
    if(numberDecodingBytes==-2 && verbose){ 
        cout<<"[MacController] Drop packet due to CRC Error."<<endl;
        return;
    }

    //EOF checking
    if(numberDecodingBytes==0 && verbose){ 
        cout<<"[MacController] End of Transmission."<<endl;
        return;
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
            protocolControl->decodeControlSdus(buffer, numberDecodingBytes);
        else    //Data SDU
            protocolData->decodeDataSdus(buffer, numberDecodingBytes);
    }
    delete transmissionQueue;
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
    mimoConfiguration.scheme = staticParameters->mimoConf==0? NONE:(staticParameters->mimoDiversityMultiplexing==0? DIVERSITY:MULTIPLEXING);
    mimoConfiguration.num_tx_antenas = staticParameters->mimoAntenna==0? 2:4;
    mimoConfiguration.precoding_mtx = staticParameters->mimoPrecoding;

    //MCS Configuration
    mcsConfiguration.num_info_bytes = staticParameters->mtu;
    mcsConfiguration.num_coded_bytes = staticParameters->mtu/codeRate;
    mcsConfiguration.modulation = QAM64;
    mcsConfiguration.power_offset = staticParameters->transmissionPowerControl;

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
    dynamicParameters->deserialize(serializedBytes);

    if(verbose) cout<<"[MacController] Dynamic Parameters were managed successfully."<<endl;
}

void
MacController::manager(){   //This thread executes only on BS
    vector<uint8_t> dynamicParametersBytes;
    //Infinite loop
    while(1){
        //Wait for TIMEOUT_DYNAMIC_PARAMETERS seconds
        this_thread::sleep_for(chrono::seconds(TIMEOUT_DYNAMIC_PARAMETERS));

        if(dynamicParameters->isModified()){
        	dynamicParameters->dynamicParametersMutex.lock();
        	{
        		//Send a MACC SDU to each UE attached
				for(int i=0;i<staticParameters->numberUEs;i++){
					dynamicParametersBytes.clear();
					dynamicParameters->serialize(staticParameters->ulReservations[i].target_ue_id, dynamicParametersBytes);
					protocolControl->enqueueControlSdus(&(dynamicParametersBytes[0]), dynamicParametersBytes.size(), staticParameters->ulReservations[i].target_ue_id);
				}
        	}
        	dynamicParameters->dynamicParametersMutex.unlock();
        }
    }
}
