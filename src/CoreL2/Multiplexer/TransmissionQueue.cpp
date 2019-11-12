/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "TransmissionQueue.h"

/**
 * @brief Constructs a new TransmissionQueue to accumulate SDUs for future transmission (encoding)
 * @param nBytes Maximum number of bytes supported by a single PDU
 * @param src Source MAC Address
 * @param dst Destination MAC Address
 * @param maxSDUs Maximum number of SDUs supported in one queue
 * @param v Verbosity flag
 */
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

/**
 * @brief Constructs a TransmissionQueue to help decoding an incoming PDU, dequeueing SDUs contained in it
 * @param _buffer Buffer containing PDU for decoding
 * @param _nSDUs Number of SDUs enqueued in PDU
 * @param _sizesSDUs Array with sizes of each SDU enqueued
 * @param _dcsSDUs Array with Data/Control flags of each SDU
 * @param v Verbosity flag
 */
TransmissionQueue::TransmissionQueue(char* _buffer, uint8_t _nSDUs, uint16_t* _sizesSDUs, uint8_t* _dcsSDUs, bool v){
    offset = 0;
    buffer = _buffer;
    nSDUs = _nSDUs;
    sizesSDUs = _sizesSDUs;
    dcsSDUs = _dcsSDUs;
    verbose = v; 
}

/**
 * @brief Destroy a TransmissionQueue object
 */
TransmissionQueue::~TransmissionQueue(){
    delete[] buffer;
    delete[] sizesSDUs;
    delete[] dcsSDUs;
}

/**
 * @brief Returns the current queue buffer length considering the size of each SDU
 * @returns Current TransmissionQueue buffer length in bytes
 */
int 
TransmissionQueue::currentBufferLength(){
    int n=0;
    for(int i=0;i<nSDUs;i++){
        n+=sizesSDUs[i];
    }
    return n;
}

/**
 * @brief Returns the total PDU length in bytes, considering extra overhead of 2 bytes (srcAddr, dstAddr, numSDUs)
 */
int TransmissionQueue::getNumberofBytes(){
    int n=0;
    //Header length:
    n = 2 + 2*nSDUs;
    n+=currentBufferLength();
    return n;
}

/**
 * @brief Adds SDU to especific position of the multiplexing queue
 * @param sdu Buffer containing single SDU
 * @param size SDU size
 * @param dc SDU D/C flag
 * @param position Position where SDU will be added
 * @returns True if SDU was inserted successfully; False otherwise
 */
bool 
TransmissionQueue::addSduPosition(char* sdu, uint16_t size, uint8_t dc, int position){
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

/**
 * @brief Adds SDU to the multiplexing queue
 * @param sdu Buffer containing single SDU
 * @param size SDU size
 * @param dc SDU D/C flag
 * @returns True if SDU was inserted successfully; False otherwise
 */
bool 
TransmissionQueue::addSDU(char* sdu, uint16_t size, uint8_t dc){
    //Verify if it is possible to insert SDU
    if((size+2+getNumberofBytes())>nBytes){
        if(verbose) cout<<"[TransmissionQueue] Tried to multiplex SDU which size extrapolates nBytes."<<endl;
        return false;
    }

    //Adds SDU to position depending if it is Data or Control SDU
    return dc? addSduPosition(sdu, size, dc, nSDUs):addSduPosition(sdu, size, dc, controlOffset);
}

/**
 * @brief Function used for getting SDUs while decoding
 * @param buf Buffer of PDU that is being decoded
 * @returns Next SDU multiplexed; Returns 0 for EOF
 */
ssize_t 
TransmissionQueue::getSDU(char* buf){
    //Test if Decoding queue has ended
    if(offset == nSDUs){
        if(verbose) cout<<"[TransmissionQueue] End of demultiplexing."<<endl;
        return -1;
    }

    //Set an position offset to next SDU position in buffer
    int positionBuffer = 0;
    for(int i=0;i<offset;i++)
        positionBuffer+=sizesSDUs[i];
    
    //Copy SDU from buffer
    for(int i=0;i<sizesSDUs[offset];i++)
        buf[i] = buffer[positionBuffer+i];
    
    //Increment decoding offset
    offset++;
    if(verbose) cout<<"[TransmissionQueue] Demultiplexed SDU "<<offset+1<<endl;
    return sizesSDUs[offset-1];
}

/**
 * @brief Creates a new ProtocolPackage object based on information stored in class variables on encoding process
 * @returns ProtocolPackage for the SDUs multiplexed
 * Requires clearing the buffer before using this TransmissionQueue again
 */
ProtocolPackage* 
TransmissionQueue::getPDUPackage(){
    ProtocolPackage* pdu = new ProtocolPackage(srcAddr, dstAddr, nSDUs, sizesSDUs, dcsSDUs, buffer, verbose);
    return pdu;
}

/**
 * @brief Clears the TransmissionQueue buffer; Make sure the PDU is sent before clearing the buffer
 */
void 
TransmissionQueue::clearBuffer(){
    this->~TransmissionQueue();
    buffer = new char[nBytes];
    sizesSDUs = new uint16_t[maxNSDUs];
    dcsSDUs = new uint8_t[maxNSDUs];
    nSDUs=0;
    controlOffset = 0;
}

/**
 * @brief Gets information of Data/Control flag of previous SDU to be used in decoding
 * @returns Data(1)/Control(0) flag for SDU immediately behind (it is supposed getSDU() has been used before) current SDU in decoding queue
 */
uint8_t 
TransmissionQueue::getCurrentDCFlag(){
    return dcsSDUs[offset-1];
}

/**
 * @brief Gets destination MAC address of TransmissionQueue object
 * @returns Destination MAC address
 */
uint8_t 
TransmissionQueue::getDstAddr(){
    return dstAddr;
}