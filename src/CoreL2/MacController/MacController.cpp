#include "MacController.h"

MacController::MacController(int nEquipments, uint16_t _nB, const char* _devNameTun, MacAddressTable* _arp, uint8_t _macAddr, CoreL1* _l1, bool v){
    attachedEquipments = nEquipments;
    nB = _nB;
    tunIf = new TunInterface(_devNameTun, v);
    verbose = v;
    arp = _arp;
    macAddr = _macAddr;
    bs = (macAddr==0);
    l1 = _l1;
    if(!(tunIf->allocTunInterface())){
        if(verbose) cout << "[MacController] Error allocating tun interface." << endl;
        exit(1);
    }
    macHigh = new MacHighQueue(tunIf, v);

    threads = new thread[4+attachedEquipments];

    mux = new Multiplexer(nB, macAddr, arp, MAXSDUS, verbose);
}

MacController::~MacController(){
    delete mux;
    delete macHigh;
    delete tunIf;
    delete threads;
}

void MacController::readTunCtl(){
    int val;
    char bufData[MAXLINE];
    ssize_t nread = 0;
    while(1){
        if(macHigh->getNum()){
            if(mux->emptyPdu(0)) queueCv.notify_all();
            bzero(bufData, MAXLINE);
            nread = macHigh->getNextSdu(bufData);
            {
                lock_guard<mutex> lk(queueMutex);

                val = mux->addSdu(bufData, nread, 1);

                if(val==-1)
                    continue;

                sendPdu(val);
            
                mux->addSdu(bufData,nread, 1);
            }
        }
    }
}

void MacController::controlSduCtl(){
    int val;
    char bufControl[MAXLINE];
    MacCQueue *macc = new MacCQueue();
    ssize_t nread = 0;
    while (1){
        if(mux->emptyPdu(0)) queueCv.notify_all();
        bzero(bufControl, MAXLINE);
        nread = macAddr? macc->getControlSduCSI(bufControl): macc->getControlSduULMCS(bufControl);
        {
            lock_guard<mutex> lk(queueMutex);
            val = mux->addSdu(bufControl, nread, 0);

            if(val==-1) continue;

            sendPdu(val);

            mux->addSdu(bufControl, nread, 0);
        }
    }
    delete macc;
}

void MacController::startThreads(){
    int i;
    uint16_t* ports = l1->getPorts();
    for(i=0;i<attachedEquipments;i++)
        threads[i] = thread(&MacController::decoding, this, ports[i]);
    threads[i] = thread(&MacController::encoding, this);
    threads[i+1] = thread(&MacController::readTunCtl, this);
    threads[i+2] = thread(&MacController::controlSduCtl, this);
    threads[i+3] = thread(&MacHighQueue::reading, macHigh);
    for(i=0;i<attachedEquipments+3;i++)
        threads[i].join();
    delete ports;
}

void MacController::sendPdu(int index){
    char bufPdu[MAXLINE];
    bzero(bufPdu, MAXLINE);
    ssize_t nread2 = mux->getPdu(bufPdu, index);

    MacCtHeader macCt(bs, verbose);
    nread2 = macCt.insertControlHeader(bufPdu, nread2);

    crcPackageCalculate(bufPdu, nread2);
    for(int i=0;i<attachedEquipments;i++)
        l1->sendPdu(bufPdu, nread2+2, (l1->getPorts())[i]);
}

void MacController::encoding(){
    chrono::milliseconds timeout = chrono::milliseconds(10);    //ms

    //Communication infinite loop
    while(1){
        unique_lock<mutex> lk(queueMutex);
        /////////////PROVISIONAL: INDEX 0 IN FOLLOWING FUNCTIONS//////////////////////////
        if(queueCv.wait_for(lk, timeout)==cv_status::no_timeout || mux->emptyPdu(0)) continue;    //HERE
        if(verbose) cout<<"[MacController] Timeout!"<<endl;
        sendPdu(0);         //HERE TOO
    }
    delete macc;
}

void MacController::decoding(uint16_t port){
    int val;
    char buf[MAXLINE], ackBuf[MAXLINE];
    ProtocolPackage* pdu;
    TransmissionQueue* tqueue;

    //Communication infinite loop
    while(1){
        //Clear buffer
        bzero(buf,sizeof(buf));

        //Read packet from UE01 Socket
        int r = l1->receivePdu(buf, MAXLINE, port);
        if(r==-1 && verbose){ 
            cout<<"[MacController] Error reading from socket."<<endl;
            break;
        }
        if(r==0) break;

        if(verbose) cout<<"[MacController] Decoding "<<port<<": in progress..."<<endl;


        if(!crcPackageChecking(buf, r)){
            if(verbose) cout<<"[MacController] Drop packet due to CRC error."<<endl;
        }

        r-=2;


        MacCtHeader macCt(bs, buf, r, verbose);
        r = macCt.removeControlHeader(buf,r);

        pdu = new ProtocolPackage(buf, r, verbose);

        pdu->removeMacHeader();

        if(pdu->getDstMac()!=macAddr){
            if(verbose) cout<<"[MacController] Drop package: Wrong destination."<<endl;
            delete pdu;
            continue;       //Drop packet
        }

        tqueue = pdu->getMultiplexedSDUs();
        while((r = tqueue->getSDU(buf))>0){
            if(tqueue->getCurrentDCFlag()==0){ //Control SDU
                buf[r] = '\0';
                if(verbose){
                	cout<<"[MacController] Control SDU received: ";
                	for(int i=0;i<r;i++)
						cout<<buf[i];
					cout<<endl;
                }

                if(macAddr){    //UE needs to return ACK to BS
                    /////////////////ACK/////////////////////
                    if(mux->emptyPdu(0)) queueCv.notify_all();
                    bzero(ackBuf, MAXLINE);
                    r = macc->getAck(ackBuf);
                    lock_guard<mutex> lk(queueMutex);
                    val = mux->addSdu(ackBuf, r, 0);

                    if(val==-1) continue;

                    sendPdu(val);

                    mux->addSdu(ackBuf, r, 0);
                }
                continue;
            }
            if(verbose) cout<<"[MacController] Received from socket. Forwarding to TUN."<<endl;
            tunIf->writeTunInterface(buf, r);
        }
        delete tqueue;
        delete pdu;
    }
}

//Calculates CRC of current PDU.
void MacController::crcPackageCalculate(char *buffer, int size){
    unsigned short crc = 0x0000;
    for(int i=0;i<size;i++){
        crc = auxCalcCRC(buffer[i],crc);
    }
    buffer[size] = crc>>8;
    buffer[size+1] = crc&255;
    if(verbose) cout<<"[MacController] CRC inserted at the end of the PDU."<<endl;
}

//Returns false if there is CRC error. 
bool MacController::crcPackageChecking(char *buffer, int size){
    unsigned short crc1, crc2;
    crc1 = ((buffer[size-2]&255)<<8)|((buffer[size-1])&255);
    crc2 = 0x0000;
    for(int i=0;i<size-2;i++){
        crc2 = auxCalcCRC(buffer[i],crc2);
    }

    return crc1==crc2;
}

//Auxiliary function for CRC calculation
unsigned short MacController::auxCalcCRC(char data, unsigned short crc){
    char i, bit;
    for(i=0x01;i;i<<=1){
        bit = (((crc&0x0001)?1:0)^((data&i)?1:0));
        crc>>=1;
        if(bit) crc^=0x9299;
    }
    return crc;
}
