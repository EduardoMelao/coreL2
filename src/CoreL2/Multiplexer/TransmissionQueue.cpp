#include "TransmissionQueue.h"

TransmissionQueue::TransmissionQueue(int _nBytes, uint8_t src, uint8_t dst, int maxSDUs, bool v){
    nBytes = _nBytes;
    buffer = new char[nBytes];
    srcAddr = src;
    dstAddr = dst;
    nSDUs = 0;
    controlOffset = 0;
    maxNSDUs = maxSDUs;
    sizesSDUs = new uint16_t[maxNSDUs];
    dcsSDUs = new uint8_t[maxNSDUs];
    verbose = v;
}

TransmissionQueue::TransmissionQueue(char* _buffer, uint8_t _nSDUs, uint16_t* _sizesSDUs, uint8_t* _dcsSDUs, bool v){
    offset = 0;
    buffer = _buffer;
    nSDUs = _nSDUs;
    sizesSDUs = _sizesSDUs;
    dcsSDUs = _dcsSDUs;
    verbose = v; 
}

TransmissionQueue::~TransmissionQueue(){
    delete[] buffer;
    delete[] sizesSDUs;
    delete[] dcsSDUs;
}

int TransmissionQueue::currentBufferLength(){
    int n=0;
    for(int i=0;i<nSDUs;i++){
        n+=sizesSDUs[i];
    }
    return n;
}

//Returns the actual PDU length in bytes.
int TransmissionQueue::getNumberofBytes(){
    int n=0;
    //Header length:
    n = 2 + 2*nSDUs;
    n+=currentBufferLength();
    return n;
}

//Adds SDU to especific position of the multiplexing queue.
bool TransmissionQueue::addSduPosition(char* sdu, uint16_t size, uint8_t dc, int position){
    int bufferOffset = 0, length = currentBufferLength();
    //Copying forward information already in queue
    for(int i=nSDUs-1;i>=position;i--){
        sizesSDUs[i+1] = sizesSDUs[i];
        dcsSDUs[i+1] = dcsSDUs[i];
        bufferOffset+=sizesSDUs[i+1];
    }
    for(int i=(length-1);i>=(length-bufferOffset);i--){
        buffer[i+size] = buffer[i];
    }

    //Implanting actual Sdu
    sizesSDUs[position] = size;
    dcsSDUs[position] = dc;
    for(int i=0;i<size;i++)
        buffer[length-bufferOffset+i] = sdu[i];
    nSDUs++;
    controlOffset++;
    if(verbose) cout<<"[TransmissionQueue] Multiplexed "<<(int)nSDUs<<" SDUs into PDU."<<endl;
    return true;
}

//Adds SDU to the multiplexing queue.
bool TransmissionQueue::addSDU(char* sdu, uint16_t size, uint8_t dc){
    if((size+2+getNumberofBytes())>nBytes){
        if(verbose) cout<<"[TransmissionQueue] Tried to multiplex SDU which size extrapolates nBytes."<<endl;
        return false;
    }

    return dc? addSduPosition(sdu, size, dc, nSDUs):addSduPosition(sdu, size, dc, controlOffset);
}

//Returns the next SDU multiplexed. Returns 0 for EOF.
ssize_t TransmissionQueue::getSDU(char* buf){
    if(offset == nSDUs){
        if(verbose) cout<<"[TransmissionQueue] End of demultiplexing."<<endl;
        return -1;
    }
    int positionBuffer = 0;
    for(int i=0;i<offset;i++)
        positionBuffer+=sizesSDUs[i];
    for(int i=0;i<sizesSDUs[offset];i++)
        buf[i] = buffer[positionBuffer+i];
    if(verbose) cout<<"[TransmissionQueue] Demultiplexed SDU "<<offset+1<<endl;
    offset++;
    return sizesSDUs[offset-1];
}

//Returns the ProtocolPackage for the SDUs multiplexed.
//Requires clearing the buffer before using this TransmissionQueue again.
ProtocolPackage* TransmissionQueue::getPDUPackage(){
    ProtocolPackage* pdu = new ProtocolPackage(srcAddr, dstAddr, nSDUs, sizesSDUs, dcsSDUs, buffer, verbose);
    return pdu;
}

//Clears the TransmissionQueue buffer. Make sure the PDU is sent before clearing the buffer.
void TransmissionQueue::clearBuffer(){
    this->~TransmissionQueue();
    buffer = new char[nBytes];
    sizesSDUs = new uint16_t[maxNSDUs];
    dcsSDUs = new uint8_t[maxNSDUs];
    nSDUs=0;
    controlOffset = 0;
}

//Retuns the Data(1)/Control(0) flag for SDU immediately behind (it is supposed getSDU() has been used before) current SDU in decoding queue
uint8_t TransmissionQueue::getCurrentDCFlag(){
    return dcsSDUs[offset-1];
}


