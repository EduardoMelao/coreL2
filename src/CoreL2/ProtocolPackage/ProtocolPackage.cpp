#include "ProtocolPackage.h"

ProtocolPackage::ProtocolPackage(uint8_t sa, uint8_t da, uint8_t n, uint16_t* _sizes, uint8_t* _dcs, char* buf){
    ProtocolPackage(sa, da, n, _sizes, _dcs, buf, false);
}

ProtocolPackage::ProtocolPackage(uint8_t sa, uint8_t da, uint8_t n, uint16_t* _sizes, uint8_t* _dcs, char* buf, bool v){
    srcAddr = sa;
    dstAddr = da;
    num = n;
    sizes = _sizes;
    dcs = _dcs;
    buffer = buf;
    verbose = v;
    size = 2 + 2*n; //SA, DA , NUM and 2 for each SDU //Only header bytes here
    for(int i=0;i<n;i++){
        size += sizes[i];
    }
}

ProtocolPackage::ProtocolPackage(char* pdu, size_t _size){
    ProtocolPackage(pdu, _size, false);
}

ProtocolPackage::ProtocolPackage(char* pdu, size_t _size, bool v){
    verbose = v;
    size = _size;
    buffer = pdu;
}

ProtocolPackage::~ProtocolPackage(){
}

//Inserts MAC header base on class variables values
void ProtocolPackage::insertMacHeader(){
    int i, j;
    char* buffer2 = new char[size + 2];
    buffer2[0] = (srcAddr<<4)|(dstAddr&15);
    buffer2[1] = num;
    for(i=0;i<num;i++){
        buffer2[2+2*i] = (dcs[i]<<7)|(sizes[i]>>8);
        buffer2[3+2*i] = sizes[i]&255;
    }
    for(i=2+2*i,j=0;i<size;i++,j++){
        buffer2[i] = buffer[j];
    }

    buffer = buffer2;

    if(verbose) cout<<"[ProtocolPackage] MAC Header inserted."<<endl;
}

//Removes MAC header and input its information to class variables.
void ProtocolPackage::removeMacHeader(){
    int i;
    srcAddr = (uint8_t) (buffer[0]>>4);
    dstAddr = (uint8_t) (buffer[0]&15);
    num = (uint8_t) buffer[1];

    dcs = new uint8_t[num];  //Have to free
    sizes = new uint16_t[num];  //Have to free
    for(int i=0;i<num;i++){
        dcs[i] = (buffer[2+2*i]&255)>>7;
        sizes[i] = ((buffer[2+2*i]&127)<<8)|((buffer[3+2*i])&255);
    }
    i = 2+2*num;
    size-=i;
    char* buffer2 = new char[size];

    for(int j=0;j<size;i++,j++){
        buffer2[j]=buffer[i];
    }
    
    buffer = buffer2;
    if(verbose) cout<<"[ProtocolPackage] MAC Header removed successfully."<<endl;
    
}

//Retuns an TransmissionQueue object to perform demultiplexing
TransmissionQueue* ProtocolPackage::getMultiplexedSDUs(){
    TransmissionQueue* tqueue = new TransmissionQueue(buffer, num, sizes, dcs, verbose);
    return tqueue;
}

//Returns total PDU size in bytes
ssize_t ProtocolPackage::getPduSize(){
    return size;
}

//Returns destination MAC address
uint8_t ProtocolPackage::getDstMac(){
    return dstAddr;
}
