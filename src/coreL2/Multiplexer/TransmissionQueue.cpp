/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : TransmissionQueue.cpp
@Classification : Multiplexer
@
@Last alteration : December 13th, 2019
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module manages a single queue of SDUs and is used on encoding 
    and decoding of the MAC PDU. 
*/

#include "TransmissionQueue.h"

TransmissionQueue::TransmissionQueue(
    int _maxNumberBytes,            //Maximum number of Bytes in a PDU
    uint8_t _sourceAddress,         //Source MAC Address
    uint8_t _destinationAddress,    //Destination MAC Address
    int _maximumNumberSDUs,         //Maximum number of SDUs supported by a PDU
    bool _verbose)                  //Verbosity flag
{
    maxNumberBytes = _maxNumberBytes;
    buffer = new char[maxNumberBytes];
    sourceAddress = _sourceAddress;
    destinationAddress = _destinationAddress;
    numberSDUs = 0;
    controlOffset = 0;
    maximumNumberSDUs = _maximumNumberSDUs;
    sizesSDUs = new uint16_t[maximumNumberSDUs];
    flagsDataControl = new uint8_t[maximumNumberSDUs];
    verbose = _verbose;
}

TransmissionQueue::TransmissionQueue(
    char* _buffer,                      //Buffer containing PDU for decoding
    uint8_t _numberSDUs,                //Number of SDUs into PDU
    uint16_t* _sizesSDUs,               //Array of sizes of each SDU
    uint8_t* _flagsDataControl_SDUS,    //Array of Data/Control flags
    bool _verbose)                      //Verbosity flag
{
    offset = 0;
    buffer = _buffer;
    numberSDUs = _numberSDUs;
    sizesSDUs = _sizesSDUs;
    flagsDataControl = _flagsDataControl_SDUS;
    verbose = _verbose; 
}

TransmissionQueue::~TransmissionQueue(){
    delete[] buffer;
    delete[] sizesSDUs;
    delete[] flagsDataControl;
}

int 
TransmissionQueue::currentBufferLength(){
    int length = 0;
    for(int i=0;i<numberSDUs;i++){
        length += sizesSDUs[i];
    }
    return length;
}

int TransmissionQueue::getNumberofBytes(){
    int numberBytes = 0;
    //Header length:
    numberBytes = 2 + 2*numberSDUs;
    numberBytes +=currentBufferLength();
    return numberBytes;
}

bool 
TransmissionQueue::addSduPosition(
    char* sdu,                  //Buffer containing single SDU
    uint16_t size,              //SDU size
    uint8_t flagDataControl,    //SDU Data/Control flag
    int position)               //Position in the queue where SDU will be added
{
    int bufferOffset = 0;                   //Buffer offset to manage copying arrays
    int length = currentBufferLength();     //Current length of the queue

    //Copying forward information already in queue
    for(int i=numberSDUs-1;i>=position;i--){
        sizesSDUs[i+1] = sizesSDUs[i];
        flagsDataControl[i+1] = flagsDataControl[i];
        bufferOffset+=sizesSDUs[i+1];
    }
    for(int i=(length-1);i>=(length-bufferOffset);i--){
        buffer[i+size] = buffer[i];
    }

    //Implanting actual SDU
    sizesSDUs[position] = size;
    flagsDataControl[position] = flagDataControl;
    for(int i=0;i<size;i++)
        buffer[length-bufferOffset+i] = sdu[i];
    numberSDUs++;
    if(!flagDataControl) controlOffset++;
    if(verbose) cout<<"[TransmissionQueue] Multiplexed "<<(int)numberSDUs<<" SDUs into PDU."<<endl;
    return true;
}

bool 
TransmissionQueue::addSDU(
    char* sdu,                  //Buffer containing single SDU
    uint16_t size,              //SDU size
    uint8_t flagDataControl)    //SDU Data/Control flag
{
    //Verify if it is possible to insert SDU
    if((size+2+getNumberofBytes())>maxNumberBytes){
        if(verbose) cout<<"[TransmissionQueue] Tried to multiplex SDU which size extrapolates maxNumberBytes."<<endl;
        exit(4);
    }

    //Adds SDU to position depending if it is Data or Control SDU
    return flagDataControl? addSduPosition(sdu, size, flagDataControl, numberSDUs):addSduPosition(sdu, size, flagDataControl, controlOffset);
}

ssize_t 
TransmissionQueue::getSDU(
    char* sdu)      //Buffer to store SDU
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
        sdu[i] = buffer[positionBuffer+i];
    
    //Increment decoding offset
    offset++;
    if(verbose) cout<<"[TransmissionQueue] Demultiplexed SDU "<<offset<<endl;
    return sizesSDUs[offset-1];
}

ProtocolPackage* 
TransmissionQueue::getPDUPackage(){
    ProtocolPackage* pdu = new ProtocolPackage(sourceAddress, destinationAddress, numberSDUs, sizesSDUs, flagsDataControl, buffer, verbose);
    return pdu;
}

void 
TransmissionQueue::clearBuffer(){
    this->~TransmissionQueue();
    buffer = new char[maxNumberBytes];
    sizesSDUs = new uint16_t[maximumNumberSDUs];
    flagsDataControl = new uint8_t[maximumNumberSDUs];
    numberSDUs=0;
    controlOffset = 0;
}

uint8_t 
TransmissionQueue::getCurrentDataControlFlag(){
    return flagsDataControl[offset-1];
}

uint8_t 
TransmissionQueue::getDestinationAddress(){
    return destinationAddress;
}
