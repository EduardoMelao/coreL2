/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : ProtocolPackage.cpp
@Classification : Protocol Package
@
@Last alteration : November 13th, 2019
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module encodes and decodes the Control Header, used to communicate with PHY. 
*/

#include "ProtocolPackage.h"

ProtocolPackage::ProtocolPackage(
    uint8_t sourceAddress,          //Source MAC Address
    uint8_t destinationAddress,     //Destination MAC Address
    uint8_t _numberSDUs,            //Number of SDUs into PDU
    uint16_t* _sizes,               //Array of sizes for each SDU
    uint8_t* _flagsDataControl,     //Array of Data/Control flags for each SDU
    char* _buffer)                  //Buffer containing multiplexed SDUs
{
    ProtocolPackage(sourceAddress, destinationAddress, _numberSDUs, _sizes, _flagsDataControl, _buffer, false);
}

ProtocolPackage::ProtocolPackage(
    uint8_t _sourceAddress,         //Source MAC Address
    uint8_t _destinationAddress,    //Destination MAC Address
    uint8_t _numberSDUs,            //Number of SDUs into PDU
    uint16_t* _sizes,               //Array of sizes for each SDU
    uint8_t* _flagsDataControl,     //Array of Data/Control flags for each SDU
    char* _buffer,                  //Buffer containing multiplexed SDUs
    bool _verbose)                  //Verbosity flag
{
    sourceAddress = _sourceAddress;
    destinationAddress = _destinationAddress;
    numberSDUs = _numberSDUs;
    sizes = _sizes;
    flagsDataControl = _flagsDataControl;
    buffer = _buffer;
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
    int i, j;   //Auxiliary variables

    //Allocate new buffer
    char* buffer2 = new char[PDUsize];

    //Fills the 2 first slots
    buffer2[0] = (sourceAddress<<4)|(destinationAddress&15);
    buffer2[1] = numberSDUs;

    //Fills with the SDUs informations
    for(i=0;i<numberSDUs;i++){
        buffer2[2+2*i] = (flagsDataControl[i]<<7)|(sizes[i]>>8);
        buffer2[3+2*i] = sizes[i]&255;
    }

    //Copy the buffer with the SDUs multiplexed to the new buffer
    for(i=2+2*i,j=0;i<PDUsize;i++,j++){
        buffer2[i] = buffer[j];
    }

    memcpy(buffer, buffer2, PDUsize);

    delete [] buffer2;

    if(verbose) cout<<"[ProtocolPackage] MAC Header inserted."<<endl;
}

void 
ProtocolPackage::removeMacHeader(){
    int i;  //Auxiliary variable

    //Get information from first 2 slots
    sourceAddress = (uint8_t) (buffer[0]>>4);
    destinationAddress = (uint8_t) (buffer[0]&15);
    numberSDUs = (uint8_t) buffer[1];

    //Declare new sizes and Data/Control flags arrays and get information for SDUs decoding
    flagsDataControl = new uint8_t[numberSDUs];
    sizes = new uint16_t[numberSDUs];
    for(int i=0;i<numberSDUs;i++){
        flagsDataControl[i] = (buffer[2+2*i]&255)>>7;
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
    TransmissionQueue* tqueue = new TransmissionQueue(buffer, numberSDUs, sizes, flagsDataControl, verbose);
    return tqueue;
}

ssize_t 
ProtocolPackage::getPduSize(){
    return PDUsize;
}

uint8_t 
ProtocolPackage::getDstMac(){
    return destinationAddress;
}

uint8_t 
ProtocolPackage::getSrcMac(){
    return sourceAddress;
}
