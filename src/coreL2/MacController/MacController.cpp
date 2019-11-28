/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacController.cpp
@Classification : MAC Controller
@
@Last alteration : November 28th, 2019
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
    SDUs concatenation into PDU is made by this module, as well as CRC calculation and checking.
*/

#include "MacController.h"

MacController::MacController(
        int numberEquipments,           //Number of equipments attached
        uint8_t* _macAddressEquipments, //MAC Address of each attached equipment
        uint16_t _maxNumberBytes,       //Maximum number of Bytes in PDU
        const char* _deviceNameTun,     //TUN device name
        MacAddressTable* _ipMacTable,   //MAC address - IP address table
        uint8_t _macAddress,            //5GR MAC address
        CoreL1* _l1,                    //L1 object
        bool _verbose)                  //Verbosity flag
{
    attachedEquipments = numberEquipments;
    macAddressEquipents = _macAddressEquipments;
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
    l1l2Interface = new L1L2Interface(_l1);

    //Create reception and transmission protocols
    receptionProtocol = new ReceptionProtocol(l1l2Interface, tunInterface, verbose);
    transmissionProtocol = new TransmissionProtocol(l1l2Interface,tunInterface, verbose);

    macHigh = new MacHighQueue(receptionProtocol, _verbose);

    //Create threads
    threads = new thread[3+2*attachedEquipments]; //4 threads : Tun reading, L3 SDU multiplexing, Control PDU generation, Timeout controls, decondings

    //Create Multiplexer and set its TransmissionQueues
    mux = new Multiplexer(maxNumberBytes, macAddress, ipMacTable, MAXSDUS, flagBS, verbose);
    if(flagBS){
    	for(int i=0;i<numberEquipments;i++)
    		mux->setTransmissionQueue(macAddressEquipents[i]);
    }
    else mux->setTransmissionQueue(macAddressEquipents[0]);
}

MacController::~MacController(){
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
MacController::readTunControl(){
    int macSendingPDU;
    char bufferData[MAXLINE];
    ssize_t numberBytesRead = 0;
    while(1){

        //Test if MacHigh Queue is not empty, i.e. there are SDUs to enqueue
        if(macHigh->getNumberPackets()){

            //If multiplexer queue is empty, notify condition variable to trigger timeout timer
            for(int i=0;i<attachedEquipments;i++)
                if(mux->emptyPdu(macAddressEquipents[i])) 
                    queueConditionVariables[i].notify_all();

            //Fulfill bufferData with zeros 
            bzero(bufferData, MAXLINE);

            //Gets next SDU from MACHigh Queue
            numberBytesRead = macHigh->getNextSdu(bufferData);
            {   
                //Locks mutex to write in Multiplexer queue
                lock_guard<mutex> lk(queueMutex);
                
                //Adds SDU to multiplexer
                macSendingPDU = mux->addSdu(bufferData, numberBytesRead);

                //If the SDU was added successfully, continues the loop
                if(macSendingPDU==-1)
                    continue;

                //Else, macSendingPDU contains the Transmission Queue destination MAC to perform PDU sending. 
                //So, perform PDU sending
                sendPdu(macSendingPDU);

                //Now, it is possible to add SDU to queue
                mux->addSdu(bufferData,numberBytesRead);
            }
        }
    }
}

void 
MacController::controlSduControl(){
    int macSendingPDU;
    char bufControl[MAXLINE];

    //Creates a new MacCQueue object to generate Control SDUs
    MacCQueue *macc = new MacCQueue();
    ssize_t numberBytesRead = 0;
    while (1){
        //If multiplexing queue is empty, notify condition variable to trigger timeout timer
        for(int i=0;i<attachedEquipments;i++)
            if(mux->emptyPdu(macAddressEquipents[i])) 
                queueConditionVariables[i].notify_all();

        //Fulfill bufferControl with zeros        
        bzero(bufControl, MAXLINE);

        //Test if it is BS or UE and decides which Control SDU to get
        numberBytesRead = flagBS? macc->getControlSduCSI(bufControl): macc->getControlSduULMCS(bufControl);
        {   
            //Locks mutex to write in multiplexer queue
            lock_guard<mutex> lk(queueMutex);
            
            //Send control PDU to all attached equipments
            for(int i=0;i<attachedEquipments;i++){
                //Adds SDU to multiplexer
                macSendingPDU = mux->addSdu(bufControl, numberBytesRead, 0, macAddressEquipents[i]);

                //If the SDU was added successfully, continues the loop
                if(macSendingPDU==-1)
                    continue;

                //Else, macSendingPDU contains the Transmission Queue MAC Address to perform PDU sending. 
                //So, perform PDU sending
                sendPdu(macSendingPDU);

                //Now, it is possible to add SDU to queue
                mux->addSdu(bufControl,numberBytesRead, 0,  macAddressEquipents[i]);
            }
        }
    }
    delete macc;
}

void 
MacController::startThreads(){
    int i, j;   //Auxiliary variables for loops

    //Gets all ports to declare decoding procedures
    uint16_t* ports = l1l2Interface->getPorts();

    //For each port
    for(i=0;i<attachedEquipments;i++){
        //Decoding threads - threads[0 .. attachedEquipments]
        threads[i] = thread(&MacController::decoding, this, ports[i]);
    }

    for(j=0;j<attachedEquipments;j++){
        //timeoutController threads - threads[attachedEquipments .. 2*attachedEquipments-1]
        threads[i+j] = thread(&MacController::timeoutController, this, j);
    }

    //TUN queue control thread
    threads[i+j] = thread(&MacController::readTunControl, this);

    //Control SDUs controller thread
    threads[i+j+1] = thread(&MacController::controlSduControl, this);

    //TUN reading and enqueueing thread
    threads[i+j+2] = thread(&MacHighQueue::reading, macHigh);

    //Join all threads
    for(i=0;i<2*attachedEquipments+3;i++)
        threads[i].join();
    
    delete ports;
}

void 
MacController::sendPdu(
    uint8_t macAddress)     //Destination MAC Address of TransmissionQueue
{
    //Declaration of PDU buffer
    char bufferPdu[MAXLINE];
    char bufferControl[MAXLINE];
    bzero(bufferPdu, MAXLINE);
    bzero(bufferControl, MAXLINE);

    //Gets PDU from multiplexer
    ssize_t numberDataBytesRead = mux->getPdu(bufferPdu, macAddress);

    //Creates a Control Header to this PDU and inserts it
    MacCtHeader macControlHeader(flagBS, verbose);
    ssize_t numberControlBytesRead = macControlHeader.getControlData(bufferControl);

    //Send PDU to all attached equipments ("to air interface")
    for(int i=0;i<attachedEquipments;i++)
        transmissionProtocol->sendPackageToL1(bufferPdu, numberDataBytesRead,bufferControl, numberControlBytesRead,(l1l2Interface->getPorts())[i]);
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
        if(queueConditionVariables[index].wait_for(lk, timeout)==cv_status::no_timeout || mux->emptyPdu(macAddressEquipents[index])) continue; 
        //Else, perform PDU transmission
        if(verbose) cout<<"[MacController] Timeout!"<<endl;
        sendPdu(macAddressEquipents[index]);
    }
}

void 
MacController::decoding(
    uint16_t port)      //Port of Receiving socket
{
    int macSendingPDU;
    char buffer[MAXLINE], ackBuffer[MAXLINE];

    //Communication infinite loop
    while(1){
        //Clear buffer
        bzero(buffer,sizeof(buffer));

        //Read packet from  Socket
        int numberDecodingBytes = receptionProtocol->receivePackageFromL1(buffer, MAXLINE, port);

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

        if(verbose) cout<<"[MacController] Decoding "<<port<<": in progress..."<<endl;

        //Create ProtocolPackage object to help removing Mac Header
        ProtocolPackage pdu(buffer, numberDecodingBytes , verbose);
        pdu.removeMacHeader();

        //Verify Destination
        if(pdu.getDstMac()!=macAddress){
            if(verbose) cout<<"[MacController] Drop package: Wrong destination."<<endl;
            continue;       //Drop packet
        }

        //Create TransmissionQueue object to help unstacking SDUs contained in the PDU
        TransmissionQueue *transmissionQueue = pdu.getMultiplexedSDUs();
        while((numberDecodingBytes = transmissionQueue->getSDU(buffer))>0){
            //Test if it is Control SDU
            if(transmissionQueue->getCurrentDataControlFlag()==0){
                buffer[numberDecodingBytes] = '\0';
                if(verbose){
                	cout<<"[MacController] Control SDU received: ";
                	for(int i=0;i<numberDecodingBytes;i++)
						cout<<buffer[i];
					cout<<endl;
                }

                if(!flagBS){    //UE needs to return ACK to BS
                    // ACK
                    if(macAddressEquipents[0]) queueConditionVariables[0].notify_all();     //index 0: UE has only BS as equipment
                    bzero(ackBuffer, MAXLINE);
                    numberDecodingBytes = macControlQueue->getAck(ackBuffer);
                    lock_guard<mutex> lk(queueMutex);
                    macSendingPDU = mux->addSdu(ackBuffer, numberDecodingBytes, 0,0);

                    if(macSendingPDU==-1) continue;

                    sendPdu(macSendingPDU);

                    mux->addSdu(ackBuffer, numberDecodingBytes, 0,0);
                }
                continue;
            }
            //In case this SDU is Data SDU
            if(verbose) cout<<"[MacController] Received from socket. Forwarding to TUN."<<endl;
            transmissionProtocol->sendPackageToL3(buffer, numberDecodingBytes);
        }
        delete transmissionQueue;
    }
}