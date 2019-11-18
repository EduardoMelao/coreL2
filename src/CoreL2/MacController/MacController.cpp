/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "MacController.h"

MacController::MacController(
        int numberEquipments,       //Number of equipments attached
        uint16_t _maxNumberBytes,   //Maximum number of Bytes in PDU
        const char* _devNameTun,    //TUN device name
        MacAddressTable* _arp,      //MAC address - IP address table
        uint8_t _macAddr,           //5GR MAC address
        CoreL1* _l1,                //L1 object
        bool _verbose)             //Verbosity flag
    {
    attachedEquipments = numberEquipments;
    maxNumberBytes = _maxNumberBytes;
    tunIf = new TunInterface(_devNameTun, _verbose);
    verbose = _verbose;
    arp = _arp;
    macAddr = _macAddr;

    l1 = _l1;
    if(!(tunIf->allocTunInterface())){
        if(verbose) cout << "[MacController] Error allocating tun interface." << endl;
        exit(1);
    }
    macHigh = new MacHighQueue(tunIf, _verbose);

    threads = new thread[4+attachedEquipments];

    mux = new Multiplexer(maxNumberBytes, macAddr, arp, MAXSDUS, verbose);

    ///////PROVISIONAL: BS MAC ADDR = 0//////////////////////////////////////
    flagBS = (macAddr==0);
    if(flagBS){
    	for(int i=0;i<numberEquipments;i++)
    		mux->setTransmissionQueue(arp->getMacAddress(i+1));
    }
    else mux->setTransmissionQueue(0);
}

MacController::~MacController(){
    delete mux;
    delete macHigh;
    delete tunIf;
    delete threads;
}

void 
MacController::readTunCtl(){
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
MacController::controlSduCtl(){
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
                indexSendingPDU = mux->addSdu(bufControl, numberBytesRead, 0, arp->getMacAddress(i+1));   //<----PROVISIONAL////////////////////////////////////////////////

                //If the SDU was added successfully, continues the loop
                if(indexSendingPDU==-1)
                    continue;

                //Else, indexSendingPDU contains the Transmission Queue index to perform PDU sending. 
                //So, perform PDU sending
                sendPdu(indexSendingPDU);

                //Now, it is possible to add SDU to queue
                mux->addSdu(bufControl,numberBytesRead, 0,  arp->getMacAddress(i+1));     //<----PROVISIONAL////////////////////////////////////////////////
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
    threads[i+1] = thread(&MacController::readTunCtl, this);

    //Control SDUs controller thread
    threads[i+2] = thread(&MacController::controlSduCtl, this);

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
    MacCtHeader macCt(flagBS, verbose);
    numberBytesRead = macCt.insertControlHeader(bufferPdu, numberBytesRead);

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
    delete macc;
}

void 
MacController::decoding(
    uint16_t port)      //Port of Receiving socket
{
    int indexSendingPDU;
    char buf[MAXLINE], ackBuf[MAXLINE];

    //Communication infinite loop
    while(1){
        //Clear buffer
        bzero(buf,sizeof(buf));

        //Read packet from  Socket
        int numberDecodingBytes = l1->receivePdu(buf, MAXLINE, port);

        //Error checking
        if(numberDecodingBytes==-1 && verbose){ 
            cout<<"[MacController] Error reading from socket."<<endl;
            break;
        }

        //EOF checking
        if(numberDecodingBytes==0) break;

        if(verbose) cout<<"[MacController] Decoding "<<port<<": in progress..."<<endl;

        //CRC checking
        if(!crcPackageChecking(buf, numberDecodingBytes)){
            if(verbose) cout<<"[MacController] Drop packet due to CRC error."<<endl;
        }

        //Remove CRC bytes from size count
        numberDecodingBytes-=2;

        //Create MacCtHeader object and remove CONTROL header
        MacCtHeader macCt(flagBS, buf, numberDecodingBytes, verbose);
        numberDecodingBytes = macCt.removeControlHeader(buf,numberDecodingBytes);

        //Create ProtocolPackage object to help removing Mac Header
        ProtocolPackage pdu(buf, numberDecodingBytes , verbose);
        pdu.removeMacHeader();

        //Verify Destination
        if(pdu.getDstMac()!=macAddr){
            if(verbose) cout<<"[MacController] Drop package: Wrong destination."<<endl;
            continue;       //Drop packet
        }

        //Create TransmissionQueue object to help unstacking SDUs contained in the PDU
        TransmissionQueue *tqueue = pdu.getMultiplexedSDUs();
        while((numberDecodingBytes = tqueue->getSDU(buf))>0){
            //Test if it is Control SDU
            if(tqueue->getCurrentDCFlag()==0){
                buf[numberDecodingBytes] = '\0';
                if(verbose){
                	cout<<"[MacController] Control SDU received: ";
                	for(int i=0;i<numberDecodingBytes;i++)
						cout<<buf[i];
					cout<<endl;
                }

                //PROVISIONAL: BS_MAC=0 AND UE_MAC !=0, ALWAYS//////////////////////////
                if(macAddr){    //UE needs to return ACK to BS
                    /////////////////ACK/////////////////////
                    if(mux->emptyPdu(0)) queueConditionVariable.notify_all();
                    bzero(ackBuf, MAXLINE);
                    numberDecodingBytes = macc->getAck(ackBuf);
                    lock_guard<mutex> lk(queueMutex);
                    indexSendingPDU = mux->addSdu(ackBuf, numberDecodingBytes, 0,0);

                    if(indexSendingPDU==-1) continue;

                    sendPdu(indexSendingPDU);

                    mux->addSdu(ackBuf, numberDecodingBytes, 0,0);
                }
                continue;
            }
            //In case this SDU is Data SDU
            if(verbose) cout<<"[MacController] Received from socket. Forwarding to TUN."<<endl;
            tunIf->writeTunInterface(buf, numberDecodingBytes);
        }
        delete tqueue;
    }
}

void 
MacController::crcPackageCalculate(
    char* buffer,       //Buffer of Bytes of PDU
    int size)           //PDU size in Bytes
{
    unsigned short crc = 0x0000;
    for(int i=0;i<size;i++){
        crc = auxCalcCRC(buffer[i],crc);
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
        crc2 = auxCalcCRC(buffer[i],crc2);
    }

    return crc1==crc2;
}

unsigned short 
MacController::auxCalcCRC(
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
