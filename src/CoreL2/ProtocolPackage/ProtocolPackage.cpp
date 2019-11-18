/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "ProtocolPackage.h"

ProtocolPackage::ProtocolPackage(
    uint8_t sourceAddress,      //Source MAC Address
    uint8_t destinationAddress, //Destination MAC Address
    uint8_t numberSDUs,         //Number of SDUs into PDU
    uint16_t* _sizes,           //Array of sizes for each SDU
    uint8_t* _flagsDC,          //Array of Data/Control flags for each SDU
    char* buffer)                  //Buffer containing multiplexed SDUs
{
    ProtocolPackage(sourceAddress, destinationAddress, numberSDUs, _sizes, _flagsDC, buffer, false);
}

ProtocolPackage::ProtocolPackage(
    uint8_t sourceAddress,      //Source MAC Address
    uint8_t destinationAddress, //Destination MAC Address
    uint8_t numberSDUs,         //Number of SDUs into PDU
    uint16_t* _sizes,           //Array of sizes for each SDU
    uint8_t* _flagsDC,          //Array of Data/Control flags for each SDU
    char* buffer,               //Buffer containing multiplexed SDUs
    bool _verbose)              //Verbosity flag
{
    srcAddr = sourceAddress;
    dstAddr = destinationAddress;
    numberSDUs = numberSDUs;
    sizes = _sizes;
    flagsDC = _flagsDC;
    buffer = buffer;
    verbose = _verbose;
    PDUsize = 2 + 2*numberSDUs; //SA, DA , NUM and 2 for each SDU ; Only header bytes here
    for(int i=0;i<numberSDUs;i++){
        PDUsize += sizes[i];
    }
}

ProtocolPackage::ProtocolPackage(
    char* pdu,          //Buffer containing full PDU
    size_t _PDUsize)    //Size of PDU in Bytes
{
    ProtocolPackage(pdu, _PDUsize, false);
}

ProtocolPackage::ProtocolPackage(
    char* pdu,          //Buffer containing PDU
    size_t _PDUsize,    //Size of PDU in Bytes
    bool _verbose)      //Verbosity flag
{
    verbose = _verbose;
    PDUsize = _PDUsize;
    buffer = pdu;
}

ProtocolPackage::~ProtocolPackage(){
}

void 
ProtocolPackage::insertMacHeader(){
    int i, j;       //Auxiliar variables

    //Allocate new buffer
    char* buffer2 = new char[PDUsize];

    //Fills the 2 first slots
    buffer2[0] = (srcAddr<<4)|(dstAddr&15);
    buffer2[1] = numberSDUs;

    //Fills with the SDUs informations
    for(i=0;i<numberSDUs;i++){
        buffer2[2+2*i] = (flagsDC[i]<<7)|(sizes[i]>>8);
        buffer2[3+2*i] = sizes[i]&255;
    }

    //Copy the buffer with the SDUs multiplexed to the new buffer
    for(i=2+2*i,j=0;i<PDUsize;i++,j++){
        buffer2[i] = buffer[j];
    }

    buffer = buffer2;

    if(verbose) cout<<"[ProtocolPackage] MAC Header inserted."<<endl;
}

void 
ProtocolPackage::removeMacHeader(){
    int i;      //Auxiliar variable

    //Get information from first 2 slots
    srcAddr = (uint8_t) (buffer[0]>>4);
    dstAddr = (uint8_t) (buffer[0]&15);
    numberSDUs = (uint8_t) buffer[1];

    //Declare new sizes and D/Cs arrays and get information for SDUs decoding
    flagsDC = new uint8_t[numberSDUs];
    sizes = new uint16_t[numberSDUs];
    for(int i=0;i<numberSDUs;i++){
        flagsDC[i] = (buffer[2+2*i]&255)>>7;
        sizes[i] = ((buffer[2+2*i]&127)<<8)|((buffer[3+2*i])&255);
    }
    i = 2+2*numberSDUs;
    PDUsize-=i;

    //Create a new buffer that will contain only SDUs, no header
    char* buffer2 = new char[PDUsize];

    for(int j=0;j<PDUsize;i++,j++){
        buffer2[j]=buffer[i];
    }
    
    buffer = buffer2;
    if(verbose) cout<<"[ProtocolPackage] MAC Header removed successfully."<<endl;
    
}

TransmissionQueue* 
ProtocolPackage::getMultiplexedSDUs(){
    TransmissionQueue* tqueue = new TransmissionQueue(buffer, numberSDUs, sizes, flagsDC, verbose);
    return tqueue;
}

ssize_t 
ProtocolPackage::getPduSize(){
    return PDUsize;
}

uint8_t 
ProtocolPackage::getDstMac(){
    return dstAddr;
}
