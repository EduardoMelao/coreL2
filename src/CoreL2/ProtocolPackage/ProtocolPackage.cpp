/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "ProtocolPackage.h"

/**
 * @brief Constructs ProtocolPackage with all information need to encode header and no verbosity
 * @param sa Source MAC Address
 * @param da Destination MAC Address
 * @param n Number of SDUs
 * @param _sizes Array of sizes of each SDU
 * @param _dcs Array of D/C flags of each SDU
 * @param buf Buffer containing multiplexed SDUs
 */
ProtocolPackage::ProtocolPackage(uint8_t sa, uint8_t da, uint8_t n, uint16_t* _sizes, uint8_t* _dcs, char* buf){
    ProtocolPackage(sa, da, n, _sizes, _dcs, buf, false);
}

/**
 * @brief Constructs ProtocolPackage with all information need to encode header
 * @param sa Source MAC Address
 * @param da Destination MAC Address
 * @param n Number of SDUs
 * @param _sizes Array of sizes of each SDU
 * @param _dcs Array of D/C flags of each SDU
 * @param buf Buffer containing multiplexed SDUs
 * @param v Verbosity flag
 */
ProtocolPackage::ProtocolPackage(uint8_t sa, uint8_t da, uint8_t n, uint16_t* _sizes, uint8_t* _dcs, char* buf, bool v){
    srcAddr = sa;
    dstAddr = da;
    num = n;
    sizes = _sizes;
    dcs = _dcs;
    buffer = buf;
    verbose = v;
    size = 2 + 2*n; //SA, DA , NUM and 2 for each SDU ; Only header bytes here
    for(int i=0;i<n;i++){
        size += sizes[i];
    }
}

/**
 * @brief Constructs ProtocolPackage with an incoming PDU to be decoded and no verbosity
 * @param pdu Buffer containing full PDU
 * @param _size Size of PDU in bytes
 */
ProtocolPackage::ProtocolPackage(char* pdu, size_t _size){
    ProtocolPackage(pdu, _size, false);
}

/**
 * @brief Constructs ProtocolPackage with an incoming PDU to be decoded
 * @param pdu Buffer containing full PDU
 * @param _size Size of PDU in bytes
 * @param v Verbosity flag
 */
ProtocolPackage::ProtocolPackage(char* pdu, size_t _size, bool v){
    verbose = v;
    size = _size;
    buffer = pdu;
}

/**
 * @brief Destructs ProtocolPackage object
 */
ProtocolPackage::~ProtocolPackage(){
}

/**
 * @brief Inserts MAC header based on class variables values
 */
void 
ProtocolPackage::insertMacHeader(){
    int i, j;

    //Allocate new buffer
    char* buffer2 = new char[size];

    //Fills the 2 first slots
    buffer2[0] = (srcAddr<<4)|(dstAddr&15);
    buffer2[1] = num;

    //Fills with the SDUs informations
    for(i=0;i<num;i++){
        buffer2[2+2*i] = (dcs[i]<<7)|(sizes[i]>>8);
        buffer2[3+2*i] = sizes[i]&255;
    }

    //Copy the buffer with the SDUs multiplexed to the new buffer
    for(i=2+2*i,j=0;i<size;i++,j++){
        buffer2[i] = buffer[j];
    }

    buffer = buffer2;

    if(verbose) cout<<"[ProtocolPackage] MAC Header inserted."<<endl;
}

/**
 * @brief Removes MAC header and input its information to class variables
 */
void 
ProtocolPackage::removeMacHeader(){
    int i;

    //Get information from first 2 slots
    srcAddr = (uint8_t) (buffer[0]>>4);
    dstAddr = (uint8_t) (buffer[0]&15);
    num = (uint8_t) buffer[1];

    //Declare new sizes and D/Cs arrays and get information for SDUs decoding
    dcs = new uint8_t[num];
    sizes = new uint16_t[num];
    for(int i=0;i<num;i++){
        dcs[i] = (buffer[2+2*i]&255)>>7;
        sizes[i] = ((buffer[2+2*i]&127)<<8)|((buffer[3+2*i])&255);
    }
    i = 2+2*num;
    size-=i;

    //Create a new buffer that will contain only SDUs, no header
    char* buffer2 = new char[size];

    for(int j=0;j<size;i++,j++){
        buffer2[j]=buffer[i];
    }
    
    buffer = buffer2;
    if(verbose) cout<<"[ProtocolPackage] MAC Header removed successfully."<<endl;
    
}

/**
 * @brief Gets a TransmissionQueue object that will be used to unstack SDUs in decoding
 * @returns TransmissionQueue to perform demultiplexing
 */
TransmissionQueue* 
ProtocolPackage::getMultiplexedSDUs(){
    TransmissionQueue* tqueue = new TransmissionQueue(buffer, num, sizes, dcs, verbose);
    return tqueue;
}

/**
 * @brief Gets total PDU size
 * @returns PDU size in bytes
 */
ssize_t 
ProtocolPackage::getPduSize(){
    return size;
}

/**
 * @brief Gets destination MAC Address of the ProtocolPackage
 * @returns Destination MAC address
 */
uint8_t 
ProtocolPackage::getDstMac(){
    return dstAddr;
}
