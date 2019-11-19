/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacController.cpp
@Classification : MAC Controller
@
@Last alteration : November 19th, 2019
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
        uint16_t _maxNumberBytes,       //Maximum number of Bytes in PDU
        const char* _deviceNameTun,     //TUN device name
        MacAddressTable* _ipMacTable,   //MAC address - IP address table
        uint8_t _macAddr,               //5GR MAC address
        CoreL1* _l1,                    //L1 object
        bool _verbose)                  //Verbosity flag
{
    attachedEquipments = numberEquipments;
    maxNumberBytes = _maxNumberBytes;
    tunInterface = new TunInterface(_deviceNameTun, _verbose);
    verbose = _verbose;
    ipMacTable = _ipMacTable;
    macAddr = _macAddr;

    l1 = _l1;
    if(!(tunInterface->allocTunInterface())){
        if(verbose) cout << "[MacController] Error allocating tun interface." << endl;
        exit(1);
    }

    macHigh = new MacHighQueue(tunInterface, _verbose);

    threads = new thread[4+attachedEquipments]; //4 threads : Tun reading, L3 SDU multiplexing, Control PDU generation, Timeout control

    mux = new Multiplexer(maxNumberBytes, macAddr, ipMacTable, MAXSDUS, verbose);

    ///////PROVISIONAL: BS MAC ADDR = 0//////////////////////////////////////
    flagBS = (macAddr==0);
    if(flagBS){
    	for(int i=0;i<numberEquipments;i++)
    		mux->setTransmissionQueue(ipMacTable->getMacAddress(i+1));
    }
    else mux->setTransmissionQueue(0);
}

MacController::~MacController(){
    delete mux;
    delete macHigh;
    delete tunInterface;
    delete threads;
}

void 
MacController::readTunControl(){
    int indexSendingPDU;
    char bufferData[MAXLINE];
    ssize_t numberBytesRead = 0;
    while(1){

        //Test if MacHigh Queue is not empty, i.e. there are SDUs to enqueue
        if(macHigh->getNum()){

            //If multiplexer queue is empty, notify condition variable to trigger timeout timer
            if(mux->emptyPdu(0)) queueConditionVariable.notify_all();

            //Fulfill bufferData with zeros 
            bzero(bufferData, MAXLINE);

            //Gets next SDU from MACHigh Queue
            numberBytesRead = macHigh->getNextSdu(bufferData);
            {   
                //Locks mutex to write in Multiplexer queue
                lock_guard<mutex> lk(queueMutex);
                
                //Adds SDU to multiplexer
                indexSendingPDU = mux->addSdu(bufferData, numberBytesRead);

                //If the SDU was added successfully, continues the loop
                if(indexSendingPDU==-1)
                    continue;

                //Else, indexSendingPDU contains the Transmission Queue index to perform PDU sending. 
                //So, perform PDU sending
                sendPdu(indexSendingPDU);

                //Now, it is possible to add SDU to queue
                mux->addSdu(bufferData,numberBytesRead);
            }
        }
    }
}

void 
MacController::controlSduControl(){
    int indexSendingPDU;
    char bufControl[MAXLINE];

    //Creates a new MacCQueue object to generate Control SDUs
    MacCQueue *macc = new MacCQueue();
    ssize_t numberBytesRead = 0;
    while (1){
        //If multiplexing queue is empty, notify condition variable to trigger timeout timer
        if(mux->emptyPdu(0)) queueConditionVariable.notify_all();
        bzero(bufControl, MAXLINE);

        //Test if it is BS or UE and decides which Control SDU to get
        numberBytesRead = macAddr? macc->getControlSduCSI(bufControl): macc->getControlSduULMCS(bufControl);
        {   
            //Locks mutex to write in multiplexer queue
            lock_guard<mutex> lk(queueMutex);
            if(!flagBS){
                //Adds SDU to multiplexer
                indexSendingPDU = mux->addSdu(bufControl, numberBytesRead, 0, 0);

                //If the SDU was added successfully, continues the loop
                if(indexSendingPDU==-1)
                    continue;

                //Else, indexSendingPDU contains the Transmission Queue index to perform PDU sending. 
                //So, perform PDU sending
                sendPdu(indexSendingPDU);

                //Now, it is possible to add SDU to queue
                mux->addSdu(bufControl,numberBytesRead, 0, 0);
            }
            else{
                for(int i=0;i<mux->getNTransmissionQueues();i++){
                //Adds SDU to multiplexer
                indexSendingPDU = mux->addSdu(bufControl, numberBytesRead, 0, ipMacTable->getMacAddress(i+1));   //<----PROVISIONAL////////////////////////////////////////////////

                //If the SDU was added successfully, continues the loop
                if(indexSendingPDU==-1)
                    continue;

                //Else, indexSendingPDU contains the Transmission Queue index to perform PDU sending. 
                //So, perform PDU sending
                sendPdu(indexSendingPDU);

                //Now, it is possible to add SDU to queue
                mux->addSdu(bufControl,numberBytesRead, 0,  ipMacTable->getMacAddress(i+1));     //<----PROVISIONAL////////////////////////////////////////////////
                }
            }
        }
    }
    delete macc;
}

void 
MacController::startThreads(){
    int i;

    //Gets all ports to declare decoding procedures
    uint16_t* ports = l1->getPorts();

    //For each port
    for(i=0;i<attachedEquipments;i++){
        //Decoding threads - threads[0-attachetEquipments]
        threads[i] = thread(&MacController::decoding, this, ports[i]);
    }
    //timeoutController thread
    threads[i] = thread(&MacController::timeoutController, this);

    //TUN queue control thread
    threads[i+1] = thread(&MacController::readTunControl, this);

    //Control SDUs controller thread
    threads[i+2] = thread(&MacController::controlSduControl, this);

    //TUN reading and enqueueing thread
    threads[i+3] = thread(&MacHighQueue::reading, macHigh);

    //Join all threads
    for(i=0;i<attachedEquipments+3;i++)
        threads[i].join();
    
    delete ports;
}

void 
MacController::sendPdu(
    int index)     //Index of TransmissionQueue
{
    //Declaration of PDU buffer
    char bufferPdu[MAXLINE];
    bzero(bufferPdu, MAXLINE);

    //Gets PDU from multiplexer
    ssize_t numberBytesRead = mux->getPdu(bufferPdu, index);

    //Creates a Control Header to this PDU and inserts it
    MacCtHeader macControlHeader(flagBS, verbose);
    numberBytesRead = macControlHeader.insertControlHeader(bufferPdu, numberBytesRead);

    //Perform CRC calculation
    crcPackageCalculate(bufferPdu, numberBytesRead);

    //Send PDU to all attached equipments ("to air interface")
    for(int i=0;i<attachedEquipments;i++)
        l1->sendPdu(bufferPdu, numberBytesRead+2, (l1->getPorts())[i]);
}

void 
MacController::timeoutController(){
    //Timeout declaration: static
    chrono::milliseconds timeout = chrono::milliseconds(TIMEOUT);    //ms

    //Communication infinite loop
    while(1){
        //Lock mutex one time to read
        unique_lock<mutex> lk(queueMutex);
        
        /////////////PROVISIONAL: INDEX 0 IN FOLLOWING FUNCTIONS MEANS POINT-TO-POINT//////////////////////////

        //If there's no timeout OR the PDU is empty, no transmission is necessary
        if(queueConditionVariable.wait_for(lk, timeout)==cv_status::no_timeout || mux->emptyPdu(0)) continue;    //HERE
        //Else, perform PDU transmission
        if(verbose) cout<<"[MacController] Timeout!"<<endl;
        sendPdu(0);         //HERE TOO
    }
    delete macControlQueue;
}

void 
MacController::decoding(
    uint16_t port)      //Port of Receiving socket
{
    int indexSendingPDU;
    char buffer[MAXLINE], ackBuffer[MAXLINE];

    //Communication infinite loop
    while(1){
        //Clear buffer
        bzero(buffer,sizeof(buffer));

        //Read packet from  Socket
        int numberDecodingBytes = l1->receivePdu(buffer, MAXLINE, port);

        //Error checking
        if(numberDecodingBytes==-1 && verbose){ 
            cout<<"[MacController] Error reading from socket."<<endl;
            break;
        }

        //EOF checking
        if(numberDecodingBytes==0) break;

        if(verbose) cout<<"[MacController] Decoding "<<port<<": in progress..."<<endl;

        //CRC checking
        if(!crcPackageChecking(buffer, numberDecodingBytes)){
            if(verbose) cout<<"[MacController] Drop packet due to CRC error."<<endl;
        }

        //Remove CRC bytes from size count
        numberDecodingBytes-=2;

        //Create MacCtHeader object and remove CONTROL header
        MacCtHeader macControlHeader(flagBS, buffer, numberDecodingBytes, verbose);
        numberDecodingBytes = macControlHeader.removeControlHeader(buffer,numberDecodingBytes);

        //Create ProtocolPackage object to help removing Mac Header
        ProtocolPackage pdu(buffer, numberDecodingBytes , verbose);
        pdu.removeMacHeader();

        //Verify Destination
        if(pdu.getDstMac()!=macAddr){
            if(verbose) cout<<"[MacController] Drop package: Wrong destination."<<endl;
            continue;       //Drop packet
        }

        //Create TransmissionQueue object to help unstacking SDUs contained in the PDU
        TransmissionQueue *transmissionQueue = pdu.getMultiplexedSDUs();
        while((numberDecodingBytes = transmissionQueue->getSDU(buffer))>0){
            //Test if it is Control SDU
            if(transmissionQueue->getCurrentDCFlag()==0){
                buffer[numberDecodingBytes] = '\0';
                if(verbose){
                	cout<<"[MacController] Control SDU received: ";
                	for(int i=0;i<numberDecodingBytes;i++)
						cout<<buffer[i];
					cout<<endl;
                }

                //PROVISIONAL: BS_MAC=0 AND UE_MAC !=0, ALWAYS//////////////////////////
                if(macAddr){    //UE needs to return ACK to BS
                    /////////////////ACK/////////////////////
                    if(mux->emptyPdu(0)) queueConditionVariable.notify_all();
                    bzero(ackBuffer, MAXLINE);
                    numberDecodingBytes = macControlQueue->getAck(ackBuffer);
                    lock_guard<mutex> lk(queueMutex);
                    indexSendingPDU = mux->addSdu(ackBuffer, numberDecodingBytes, 0,0);

                    if(indexSendingPDU==-1) continue;

                    sendPdu(indexSendingPDU);

                    mux->addSdu(ackBuffer, numberDecodingBytes, 0,0);
                }
                continue;
            }
            //In case this SDU is Data SDU
            if(verbose) cout<<"[MacController] Received from socket. Forwarding to TUN."<<endl;
            tunInterface->writeTunInterface(buffer, numberDecodingBytes);
        }
        delete transmissionQueue;
    }
}

void 
MacController::crcPackageCalculate(
    char* buffer,       //Buffer of Bytes of PDU
    int size)           //PDU size in Bytes
{
    unsigned short crc = 0x0000;
    for(int i=0;i<size;i++){
        crc = auxiliaryCalculationCRC(buffer[i],crc);
    }
    buffer[size] = crc>>8;
    buffer[size+1] = crc&255;
    if(verbose) cout<<"[MacController] CRC inserted at the end of the PDU."<<endl;
}

bool 
MacController::crcPackageChecking(
    char* buffer,       //Bytes of PDU
    int size)           //Size of PDU in Bytes
{
    unsigned short crc1, crc2;
    crc1 = ((buffer[size-2]&255)<<8)|((buffer[size-1])&255);
    crc2 = 0x0000;
    for(int i=0;i<size-2;i++){
        crc2 = auxiliaryCalculationCRC(buffer[i],crc2);
    }

    return crc1==crc2;
}

unsigned short 
MacController::auxiliaryCalculationCRC(
    char data,              //Byte from PDU
    unsigned short crc)     //CRC history
{
    char i, bit;
    for(i=0x01;i;i<<=1){
        bit = (((crc&0x0001)?1:0)^((data&i)?1:0));
        crc>>=1;
        if(bit) crc^=0x9299;
    }
    return crc;
}
