/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacController.cpp
@Classification : MAC Controller
@
@Last alteration : December 9th, 2019
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
     * 0 .. attachedEquipments-1                    ---> Decoding threads
     * attachedEquipments .. 2*attachedEquipments-1 ---> Timeout control threads
     * 2*attachedEquipments                         ---> ProtocolData MACD SDU enqueueing
     * 2*attachedEquipments+1                       ---> ProtocolControl MACC SDU heneration and enqueueing
     * 2*attachedEquipments+2                       ---> Data SDU enqueueing from TUN interface in MacHighQueue
     * 2*attachedEquipments+3                       ---> Reading control messages from PHY   
     */
    threads = new thread[4+2*attachedEquipments];

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

    //Create new MacConfigRequest to deal with CLI
    macConfigRequest = new MacConfigRequest(verbose);
}

MacController::~MacController(){
    delete macConfigRequest;
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
        //Decoding threads - threads[0 .. attachedEquipments]
        threads[i] = thread(&MacController::decoding, this, macAddressEquipments[i]);
    }

    for(j=0;j<attachedEquipments;j++){
        //timeoutController threads - threads[attachedEquipments .. 2*attachedEquipments-1]
        threads[i+j] = thread(&MacController::timeoutController, this, j);
    }

    //TUN queue control thread
    threads[i+j] = thread(&ProtocolData::enqueueDataSdus, protocolData);

    //Control SDUs controller thread
    threads[i+j+1] = thread(&ProtocolControl::enqueueControlSdus, protocolControl);

    //TUN reading and enqueueing thread
    threads[i+j+2] = thread(&MacHighQueue::reading, macHigh);

    //Control messages from PHY reading
    threads[i+j+3] = thread(&ProtocolControl::receiveInterlayerMessages, protocolControl);

    //Join all threads
    for(i=0;i<2*attachedEquipments+4;i++)
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

    //Downlink routine:
    string subFrameStartMessage("BSSubframeTx.Start");
    string subFrameEndMessage("BSSubframeTx.End");
    //vector<uint8_t> ulReservation;
    //if(macConfigRequest->flagModified)
    //    macConfigRequest->getULReservation(ulReservation);
    //subFrameStartMessage.resize(subFrameStartMessage.size()+ulReservation.size());
    transmissionProtocol->sendControlMessageToL1(&subFrameStartMessage[0], subFrameStartMessage.size());
    transmissionProtocol->sendPackageToL1(bufferPdu, numberDataBytesRead,bufferControl, numberControlBytesRead, macAddress);
    transmissionProtocol->sendControlMessageToL1(&subFrameEndMessage[0], subFrameEndMessage.size());
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
MacController::decoding(
    uint8_t macAddress) //Source MAC Address
{
    char buffer[MAXIMUM_BUFFER_LENGTH];

    //Communication infinite loop
    while(1){
        //Clear buffer
        bzero(buffer,sizeof(buffer));

        //Read packet from  Socket
        ssize_t numberDecodingBytes = receptionProtocol->receivePackageFromL1(buffer, MAXIMUM_BUFFER_LENGTH, macAddress);

        //Error checking
        if(numberDecodingBytes==-1 && verbose){ 
            cout<<"[MacController] Error reading from socket."<<endl;
            break;
        }

        //CRC checking
        if(numberDecodingBytes==-2 && verbose){ 
            cout<<"[MacController] Drop packet due to CRC Error."<<endl;
            continue;
        }

        //EOF checking
        if(numberDecodingBytes==0 && verbose){ 
            cout<<"[MacController] End of Transmission."<<endl;
            break;
        }

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
}