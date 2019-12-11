/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacController.cpp
@Classification : MAC Controller
@
@Last alteration : December 10th, 2019
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
        int numberEquipments,           //Number of equipments attached
        uint8_t* _macAddressEquipments, //MAC Address of each attached equipment
        uint16_t _maxNumberBytes,       //Maximum number of Bytes in PDU
        const char* _deviceNameTun,     //TUN device name
        MacAddressTable* _ipMacTable,   //MAC address - IP address table
        uint8_t _macAddress,            //5GR MAC address
        bool _verbose)                  //Verbosity flag
{
    attachedEquipments = numberEquipments;
    macAddressEquipments = _macAddressEquipments;
    maxNumberBytes = _maxNumberBytes;
    verbose = _verbose;
    ipMacTable = _ipMacTable;
    macAddress = _macAddress;
    flagBS = ipMacTable->getFlagBS(macAddress);
    
    //Create condition variables
    queueConditionVariables = new condition_variable[numberEquipments];
    
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

    macHigh = new MacHighQueue(receptionProtocol, _verbose);

    /** Threads order:
     * 0 .. attachedEquipments-1    ---> Timeout control threads
     * attachedEquipments           ---> ProtocolData MACD SDU enqueueing
     * attachedEquipments+1         ---> ProtocolControl MACC SDU heneration and enqueueing
     * attachedEquipments+2         ---> Data SDU enqueueing from TUN interface in MacHighQueue
     * attachedEquipments+3         ---> Reading control messages from PHY
     */
    threads = new thread[4+attachedEquipments];

    //Create Multiplexer and set its TransmissionQueues
    mux = new Multiplexer(maxNumberBytes, macAddress, ipMacTable, MAXSDUS, flagBS, verbose);
    if(flagBS){
    	for(int i=0;i<numberEquipments;i++)
    		mux->setTransmissionQueue(macAddressEquipments[i]);
    }
    else mux->setTransmissionQueue(macAddressEquipments[0]);

    //Create ProtocolData to deal with MACD SDUs
    protocolData = new ProtocolData(this,macHigh, verbose);

    //Create ProtocolControl to deal with MACC SDUs
    protocolControl = new ProtocolControl(this, verbose);
}

MacController::~MacController(){
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
    int i, j;   //Auxiliary variables for loops

    //For each equipment
    for(i=0;i<attachedEquipments;i++){
        //timeoutController threads - threads[0 .. attachedEquipments-1]
        threads[i] = thread(&MacController::timeoutController, this, i);
    }

    //TUN queue control thread
    threads[i] = thread(&ProtocolData::enqueueDataSdus, protocolData);

    //Control SDUs controller thread
    threads[i+1] = thread(&ProtocolControl::enqueueControlSdus, protocolControl);

    //TUN reading and enqueueing thread
    threads[i+2] = thread(&MacHighQueue::reading, macHigh);

    //Control messages from PHY reading
    threads[i+3] = thread(&ProtocolControl::receiveInterlayerMessages, protocolControl);

    //Join all threads
    for(i=0;i<attachedEquipments+4;i++)
        threads[i].join();
}

void 
MacController::sendPdu(
    uint8_t macAddress)     //Destination MAC Address of TransmissionQueue
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
    setMacPduStaticInformation();
    macPDU.mac_data_.assign(bufferPdu, bufferPdu+numberDataBytesRead);
    macPDU.control_data_.assign(bufferControl, bufferControl+numberControlBytesRead);
    macPDU.allocation_.target_ue_id = macAddress;
    macPDU.mcs_.num_info_bytes = numberDataBytesRead;
    macPDU.allocation_.number_of_rb = get_num_required_rb(macPDU.numID_, macPDU.mimo_, macPDU.mcs_.modulation, 3/4 , numberDataBytesRead*8);

    //Downlink routine:
    string subFrameStartMessage = flagBS? "BS":"UE";
    subFrameStartMessage+="SubframeTx.Start";
    string subFrameEndMessage = flagBS? "BS":"UE";
    subFrameEndMessage += "BSSubframeTx.End";
    
    protocolControl->sendInterlayerMessages(&subFrameStartMessage[0], subFrameStartMessage.size());
    transmissionProtocol->sendPackageToL1(macPDU, macAddress);
    protocolControl->sendInterlayerMessages(&subFrameEndMessage[0], subFrameEndMessage.size());
}

void 
MacController::timeoutController(
    int index)      //Index that identifies the condition variable and destination MAC Address of a queue 
{
    //Timeout declaration: static
    chrono::nanoseconds timeout = chrono::nanoseconds(TIMEOUT);	//ns

    //Communication infinite loop
    while(1){
        //Lock mutex one time to read
        unique_lock<mutex> lk(queueMutex);
        
        //If there's no timeout OR the PDU is empty, no transmission is necessary
        if(queueConditionVariables[index].wait_for(lk, timeout)==cv_status::no_timeout || mux->emptyPdu(macAddressEquipments[index])) 
            continue; 
        //Else, perform PDU transmission
        if(verbose) cout<<"[MacController] Timeout!"<<endl;
        sendPdu(macAddressEquipments[index]);
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

    macAddress = (buffer[0]>>4)&15;
    
    if(verbose) cout<<"[MacController] Decoding MAC Address "<<macAddress<<": in progress..."<<endl;

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
MacController::setMacPduStaticInformation(){
    //Static information:
    uint8_t ueID = 0xFF;                        //Equipment identification (0xFF indicates all terminals)
    unsigned numerologyID = 2;                  //Numerology identification
    float codeRate = 3/4;                       //Core rate used in codification
    mimo_cfg_t mimoConfiguration;               //MIMO configuration structure
    mcs_cfg_t mcsConfiguration;                 //Modulation Coding Scheme configuration
    allocation_cfg_t allocationConfiguration;   //Resource allocation configuration
    macphyctl_t macPhyControl;                  //MAC-PHY control structure
    size_t numberBytes = 1500;                     

    //MIMO Configuration
    mimoConfiguration.scheme = NONE;
    mimoConfiguration.num_tx_antenas = 1;
    mimoConfiguration.precoding_mtx = 0;

    //MCS Configuration
    mcsConfiguration.num_info_bytes = numberBytes;
    mcsConfiguration.modulation = QAM64;
    mcsConfiguration.num_info_bytes = 1500;

    //Resource allocation configuration
    allocationConfiguration.first_rb = 0;
    allocationConfiguration.number_of_rb = get_num_required_rb(numerologyID, mimoConfiguration, mcsConfiguration.modulation, codeRate, numberBytes*8);
    allocationConfiguration.target_ue_id = ueID;

    //MAC-PHY Control
    macPhyControl.first_tb_in_subframe = true;
    macPhyControl.last_tb_in_subframe = false;
    macPhyControl.sequence_number = 1;
    macPhyControl.subframe_number = 3;

    //MAC PDU object definition
    macPDU.allocation_ = allocationConfiguration;
    macPDU.mimo_ = mimoConfiguration;
    macPDU.mcs_ = mcsConfiguration;
    macPDU.macphy_ctl_ = macPhyControl;
}
