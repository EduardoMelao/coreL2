#include "Multiplexer.h"

Multiplexer::Multiplexer(uint16_t nB, uint8_t _srcMac, MacAddressTable* _arp, int _maxSDUs, bool v){
    TransmissionQueues = new TransmissionQueue*[MAX_BUFFERS];
    srcMac = _srcMac;
    dstMac = new uint8_t[MAX_BUFFERS];
    nBytes = new uint16_t[MAX_BUFFERS];
    maxNBytes = nB;
    nTransmissionQueues = 0;
    arp = _arp;
    maxSDUs = _maxSDUs;
    verbose = v;
    if(v) cout<<"[Multiplexer] Created successfully."<<endl;
}

Multiplexer::~Multiplexer(){
    for(int i=0;i<nTransmissionQueues;i++)
        delete TransmissionQueues[i];
    delete[] TransmissionQueues;
    delete[] dstMac;
    delete[] nBytes; 
}

int Multiplexer::addSdu(char* sdu, uint16_t n, uint8_t dc){
    int i;
    uint8_t mac;
    //First, see if there's a TransmissionQueue ready
    if(dc){
        uint8_t ipAddr[4];
        for(int i=0;i<4;i++)
            ipAddr[i] = (uint8_t) sdu[DST_OFFSET+i]; //Copying IP address
        mac = arp->getMacAddress(ipAddr);
    }
    
    ////////////////////PROVISIONAL///////////////////////////////
    else mac = srcMac? 0:1;///////////////////////////////////////
    //////////////////////////////////////////////////////////////

    for(i=0;i<nTransmissionQueues;i++)
        if(dstMac[i]==mac)
            break;

    if(i==nTransmissionQueues){   //Means there's no TransmissionQueue for this address
        if(nTransmissionQueues>MAX_BUFFERS && verbose){
            cout<<"[Multiplexer] Trying to create more buffers than supported."<<endl;
            return -2;
        }
        TransmissionQueues[i] = new TransmissionQueue(maxNBytes, srcMac, mac, maxSDUs, verbose);
        dstMac[i] = mac;
        nBytes[i] = 0;
        nTransmissionQueues++;
    }

    if((n + 2 + TransmissionQueues[i]->getNumberofBytes())>maxNBytes){
        if(verbose) cout<<"[Multiplexer] Number of bytes exceed buffer max length. Returning index."<<endl;
        return i;
    }

    if(TransmissionQueues[i]->nSDUs+1 == TransmissionQueues[i]->maxNSDUs){
        if(verbose) cout<<"[TransmissionQueue] Tried to multiplex more SDUs than supported."<<endl;
        return i;
    }

    if(TransmissionQueues[i]->addSDU(sdu, n, dc)){
        nBytes[i]+=n;
        if(verbose&&dc) cout<< "[Multiplexer] Data SDU added to queue!"<<endl;
        if(verbose&&!dc) cout<< "[Multiplexer] Control SDU added to queue!"<<endl;
        return -1;
    }
    return -2; 
}

ssize_t Multiplexer::getPdu(char* buffer, int index){
    ssize_t size;
    if(index>=nTransmissionQueues){
        if(verbose) cout<<"[Multiplexer] Could not get PDU: index out of bounds."<<endl;
        return -1;
    }
    if(nBytes[index] == 0){
        if(verbose) cout<<"[Multiplexer] Could not get PDU: no Bytes to transfer."<<endl;
        return -1;
    }
    ProtocolPackage* pdu = TransmissionQueues[index]->getPDUPackage();
    if(verbose) cout<<"[Multiplexer] Inserting MAC Header and CRC."<<endl;
    pdu->insertMacHeader();
    size = pdu->getPduSize();
    memcpy(buffer, pdu->buffer, size);
    delete pdu;

    TransmissionQueues[index]->clearBuffer();
    nBytes[index] = 0;

    return size;
}

bool Multiplexer::emptyPdu(int index){
    if(index>=nTransmissionQueues) return true;
    return nBytes[index]==0;
}

