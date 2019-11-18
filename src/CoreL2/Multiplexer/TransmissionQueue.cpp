/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "TransmissionQueue.h"

TransmissionQueue::TransmissionQueue(
    int _maxNumberBytes,    //Maximum number of Bytes in a PDU
    uint8_t src,            //Source MAC Address
    uint8_t dst,            //Destination MAC Address
    int maxSDUs,            //Maximum number of SDUs supported by a PDU
    bool _verbose)          //Verbosity flag
{
    maxNumberBytes = _maxNumberBytes;
    buffer = new char[maxNumberBytes];
    srcAddr = src;
    dstAddr = dst;
    numberSDUs = 0;
    controlOffset = 0;
    maxNSDUs = maxSDUs;
    sizesSDUs = new uint16_t[maxNSDUs];
    flagsDC = new uint8_t[maxNSDUs];
    verbose = _verbose;
}

TransmissionQueue::TransmissionQueue(
    char* _buffer,              //Buffer containing PDU for decoding
    uint8_t _numberSDUs,        //Number of SDUs into PDU
    uint16_t* _sizesSDUs,       //Array of sizes of each SDU
    uint8_t* _flagsDC_SDUS,     //Array of Data/Control flags
    bool _verbose)                     //Verbosity flag
{
    offset = 0;
    buffer = _buffer;
    numberSDUs = _numberSDUs;
    sizesSDUs = _sizesSDUs;
    flagsDC = _flagsDC_SDUS;
    verbose = _verbose; 
}

TransmissionQueue::~TransmissionQueue(){
    delete[] buffer;
    delete[] sizesSDUs;
    delete[] flagsDC;
}

int 
TransmissionQueue::currentBufferLength(){
    int n=0;
    for(int i=0;i<numberSDUs;i++){
        n+=sizesSDUs[i];
    }
    return n;
}

int TransmissionQueue::getNumberofBytes(){
    int n=0;
    //Header length:
    n = 2 + 2*numberSDUs;
    n+=currentBufferLength();
    return n;
}

bool 
TransmissionQueue::addSduPosition(
    char* sdu,          //Buffer containing single SDU
    uint16_t size,      //SDU size
    uint8_t flagDC,     //SDU Data/Control flag
    int position)       //Position in the queue where SDU will be added
{
    int bufferOffset = 0;                   //Buffer offset to manage copying arrays
    int length = currentBufferLength();     //Current length of the queue

    //Copying forward information already in queue
    for(int i=numberSDUs-1;i>=position;i--){
        sizesSDUs[i+1] = sizesSDUs[i];
        flagsDC[i+1] = flagsDC[i];
        bufferOffset+=sizesSDUs[i+1];
    }
    for(int i=(length-1);i>=(length-bufferOffset);i--){
        buffer[i+size] = buffer[i];
    }

    //Implanting actual Sdu
    sizesSDUs[position] = size;
    flagsDC[position] = flagDC;
    for(int i=0;i<size;i++)
        buffer[length-bufferOffset+i] = sdu[i];
    numberSDUs++;
    controlOffset++;
    if(verbose) cout<<"[TransmissionQueue] Multiplexed "<<(int)numberSDUs<<" SDUs into PDU."<<endl;
    return true;
}

bool 
TransmissionQueue::addSDU(
    char* sdu,          //Buffer containing single SDU
    uint16_t size,      //SDU size
    uint8_t flagDC)     //SDU Data/Control flag
{
    //Verify if it is possible to insert SDU
    if((size+2+getNumberofBytes())>maxNumberBytes){
        if(verbose) cout<<"[TransmissionQueue] Tried to multiplex SDU which size extrapolates maxNumberBytes."<<endl;
        return false;
    }

    //Adds SDU to position depending if it is Data or Control SDU
    return flagDC? addSduPosition(sdu, size, flagDC, numberSDUs):addSduPosition(sdu, size, flagDC, controlOffset);
}

ssize_t 
TransmissionQueue::getSDU(
    char* buf)      //Buffer to store SDU
{
    //Test if Decoding queue has ended
    if(offset == numberSDUs){
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

ProtocolPackage* 
TransmissionQueue::getPDUPackage(){
    ProtocolPackage* pdu = new ProtocolPackage(srcAddr, dstAddr, numberSDUs, sizesSDUs, flagsDC, buffer, verbose);
    return pdu;
}

void 
TransmissionQueue::clearBuffer(){
    this->~TransmissionQueue();
    buffer = new char[maxNumberBytes];
    sizesSDUs = new uint16_t[maxNSDUs];
    flagsDC = new uint8_t[maxNSDUs];
    numberSDUs=0;
    controlOffset = 0;
}

uint8_t 
TransmissionQueue::getCurrentDCFlag(){
    return flagsDC[offset-1];
}

uint8_t 
TransmissionQueue::getDstAddr(){
    return dstAddr;
}