/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "MacController.h"

/**
 * @brief Initializes a MacController object to manage all 5G RANGE MAC Operations
 * @param nEquipments Number of attached equipments. Must be 1 for UEs
 * @param _nB Maximum number of PDU in Bytes 
 * @param _devNameTun Customized name for TUN Interface
 * @param _arp Static table to link IP addresses to 5G-RANGE MAC addresses
 * @param _macAddr Current MAC address
 * @param _l1 Configured CoreL1 object
 * @param v Verbosity flag
 */
MacController::MacController(int nEquipments, uint16_t _nB, const char* _devNameTun, MacAddressTable* _arp, uint8_t _macAddr, CoreL1* _l1, bool v){
    attachedEquipments = nEquipments;
    nB = _nB;
    tunIf = new TunInterface(_devNameTun, v);
    verbose = v;
    arp = _arp;
    macAddr = _macAddr;

    l1 = _l1;
    if(!(tunIf->allocTunInterface())){
        if(verbose) cout << "[MacController] Error allocating tun interface." << endl;
        exit(1);
    }
    macHigh = new MacHighQueue(tunIf, v);

    threads = new thread[4+attachedEquipments];

    mux = new Multiplexer(nB, macAddr, arp, MAXSDUS, verbose);

    ///////PROVISIONAL: BS MAC ADDR = 0//////////////////////////////////////
    bs = (macAddr==0);
    if(bs){
    	for(int i=0;i<nEquipments;i++)
    		mux->setTransmissionQueue(arp->getMacAddress(i+1));
    }
    else mux->setTransmissionQueue(0);
}

/**
 * @brief Destructs MacController object
 */
MacController::~MacController(){
    delete mux;
    delete macHigh;
    delete tunIf;
    delete threads;
}

/**
 * @brief Procedure that executes forever and controls TUN interface reading
 */
void 
MacController::readTunCtl(){
    int val;
    char bufData[MAXLINE];
    ssize_t nread = 0;
    while(1){

        //Test if MacHigh Queue is not empty, i.e. there are SDUs to enqueue
        if(macHigh->getNum()){

            //If multiplexer queue is empty, notify condition variable to trigger timeout timer
            if(mux->emptyPdu(0)) queueCv.notify_all();

            //Fulfill bufData with zeros 
            bzero(bufData, MAXLINE);

            //Gets next SDU from MACHigh Queue
            nread = macHigh->getNextSdu(bufData);
            {   
                //Locks mutex to write in Multiplexer queue
                lock_guard<mutex> lk(queueMutex);
                
                //Adds SDU to multiplexer
                val = mux->addSdu(bufData, nread);

                //If the SDU was added successfully, continues the loop
                if(val==-1)
                    continue;

                //Else, val contains the Transmission Queue index to perform PDU sending. 
                //So, perform PDU sending
                sendPdu(val);

                //Now, it is possible to add SDU to queue
                mux->addSdu(bufData,nread);
            }
        }
    }
}

/**
 * @brief Procedure that executes forever and controls Control SDUs entrance in Multiplexing queue
 */
void 
MacController::controlSduCtl(){
    int val;
    char bufControl[MAXLINE];

    //Creates a new MacCQueue object to generate Control SDUs
    MacCQueue *macc = new MacCQueue();
    ssize_t nread = 0;
    while (1){
        //If multiplexing queue is empty, notify condition variable to trigger timeout timer
        if(mux->emptyPdu(0)) queueCv.notify_all();
        bzero(bufControl, MAXLINE);

        //Test if it is BS or UE and decides which Control SDU to get
        nread = macAddr? macc->getControlSduCSI(bufControl): macc->getControlSduULMCS(bufControl);
        {   
            //Locks mutex to write in multiplexer queue
            lock_guard<mutex> lk(queueMutex);
            if(!bs){
                //Adds SDU to multiplexer
                val = mux->addSdu(bufControl, nread, 0, 0);

                //If the SDU was added successfully, continues the loop
                if(val==-1)
                    continue;

                //Else, val contains the Transmission Queue index to perform PDU sending. 
                //So, perform PDU sending
                sendPdu(val);

                //Now, it is possible to add SDU to queue
                mux->addSdu(bufControl,nread, 0, 0);
            }
            else{
                for(int i=0;i<mux->getNTransmissionQueues();i++){
                //Adds SDU to multiplexer
                val = mux->addSdu(bufControl, nread, 0, arp->getMacAddress(i+1));   //<----PROVISIONAL////////////////////////////////////////////////

                //If the SDU was added successfully, continues the loop
                if(val==-1)
                    continue;

                //Else, val contains the Transmission Queue index to perform PDU sending. 
                //So, perform PDU sending
                sendPdu(val);

                //Now, it is possible to add SDU to queue
                mux->addSdu(bufControl,nread, 0,  arp->getMacAddress(i+1));     //<----PROVISIONAL////////////////////////////////////////////////
                }
            }
        }
    }
    delete macc;
}

/**
 * @brief Declares and starts all threads necessary for MacController
 */
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

/**
 * @brief Performs PDU sending to destination identified by index
 * @param index Index of destination Transmission Queue in Multiplexer
 */
void 
MacController::sendPdu(int index){
    //Declaration of PDU buffer
    char bufPdu[MAXLINE];
    bzero(bufPdu, MAXLINE);

    //Gets PDU from multiplexer
    ssize_t nread = mux->getPdu(bufPdu, index);

    //Creates a Control Header to this PDU and inserts it
    MacCtHeader macCt(bs, verbose);
    nread = macCt.insertControlHeader(bufPdu, nread);

    //Perform CRC calculation
    crcPackageCalculate(bufPdu, nread);

    //Send PDU to all attached equipments ("to air interface")
    for(int i=0;i<attachedEquipments;i++)
        l1->sendPdu(bufPdu, nread+2, (l1->getPorts())[i]);
}

/**
 * @brief Procedure that controls timeout and triggers PDU sending
 */
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
        if(queueCv.wait_for(lk, timeout)==cv_status::no_timeout || mux->emptyPdu(0)) continue;    //HERE
        //Else, perform PDU transmission
        if(verbose) cout<<"[MacController] Timeout!"<<endl;
        sendPdu(0);         //HERE TOO
    }
    delete macc;
}

/**
 * @brief Procedure that performs decoding of PDUs received from L1
 * @param port Receiving socket port
 */
void 
MacController::decoding(uint16_t port){
    int val;
    char buf[MAXLINE], ackBuf[MAXLINE];

    //Communication infinite loop
    while(1){
        //Clear buffer
        bzero(buf,sizeof(buf));

        //Read packet from  Socket
        int r = l1->receivePdu(buf, MAXLINE, port);

        //Error checking
        if(r==-1 && verbose){ 
            cout<<"[MacController] Error reading from socket."<<endl;
            break;
        }

        //EOF checking
        if(r==0) break;

        if(verbose) cout<<"[MacController] Decoding "<<port<<": in progress..."<<endl;

        //CRC checking
        if(!crcPackageChecking(buf, r)){
            if(verbose) cout<<"[MacController] Drop packet due to CRC error."<<endl;
        }

        //Remove CRC bytes from size count
        r-=2;

        //Create MacCtHeader object and remove CONTROL header
        MacCtHeader macCt(bs, buf, r, verbose);
        r = macCt.removeControlHeader(buf,r);

        //Create ProtocolPackage object to help removing Mac Header
        ProtocolPackage pdu(buf, r , verbose);
        pdu.removeMacHeader();

        //Verify Destination
        if(pdu.getDstMac()!=macAddr){
            if(verbose) cout<<"[MacController] Drop package: Wrong destination."<<endl;
            continue;       //Drop packet
        }

        //Create TransmissionQueue object to help unstacking SDUs contained in the PDU
        TransmissionQueue *tqueue = pdu.getMultiplexedSDUs();
        while((r = tqueue->getSDU(buf))>0){
            //Test if it is Control SDU
            if(tqueue->getCurrentDCFlag()==0){
                buf[r] = '\0';
                if(verbose){
                	cout<<"[MacController] Control SDU received: ";
                	for(int i=0;i<r;i++)
						cout<<buf[i];
					cout<<endl;
                }

                //PROVISIONAL: BS_MAC=0 AND UE_MAC !=0, ALWAYS//////////////////////////
                if(macAddr){    //UE needs to return ACK to BS
                    /////////////////ACK/////////////////////
                    if(mux->emptyPdu(0)) queueCv.notify_all();
                    bzero(ackBuf, MAXLINE);
                    r = macc->getAck(ackBuf);
                    lock_guard<mutex> lk(queueMutex);
                    val = mux->addSdu(ackBuf, r, 0,0);

                    if(val==-1) continue;

                    sendPdu(val);

                    mux->addSdu(ackBuf, r, 0,0);
                }
                continue;
            }
            //In case this SDU is Data SDU
            if(verbose) cout<<"[MacController] Received from socket. Forwarding to TUN."<<endl;
            tunIf->writeTunInterface(buf, r);
        }
        delete tqueue;
    }
}

/**
 * @brief Calculares CRC of current PDU passed as parameter
 * @param buffer Bytes of current PDU
 * @param size Size of PDU in bytes
 */
void 
MacController::crcPackageCalculate(char *buffer, int size){
    unsigned short crc = 0x0000;
    for(int i=0;i<size;i++){
        crc = auxCalcCRC(buffer[i],crc);
    }
    buffer[size] = crc>>8;
    buffer[size+1] = crc&255;
    if(verbose) cout<<"[MacController] CRC inserted at the end of the PDU."<<endl;
}

/**
 * @brief Checks if CRC contained in received PDU matches calculated CRC
 * @param buffer Bytes of current PDU
 * @param size Size of PDU in bytes
 * @returns True if CRC match; False otherwise
 */
bool 
MacController::crcPackageChecking(char *buffer, int size){
    unsigned short crc1, crc2;
    crc1 = ((buffer[size-2]&255)<<8)|((buffer[size-1])&255);
    crc2 = 0x0000;
    for(int i=0;i<size-2;i++){
        crc2 = auxCalcCRC(buffer[i],crc2);
    }

    return crc1==crc2;
}

/**
 * @brief Auxiliary function for CRC calculation
 * @param data Single byte from PDU
 * @param crc CRC history
 * @returns 2-byte CRC calculation
 */
unsigned short 
MacController::auxCalcCRC(char data, unsigned short crc){
    char i, bit;
    for(i=0x01;i;i<<=1){
        bit = (((crc&0x0001)?1:0)^((data&i)?1:0));
        crc>>=1;
        if(bit) crc^=0x9299;
    }
    return crc;
}
