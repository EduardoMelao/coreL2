/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : Multiplexer.cpp
@Classification : Multiplexer
@
@Last alteration : March 13th, 2020
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module manages a single queue of aggregatedSDUs and 
    is used on encoding and decoding of the MAC PDU. 
*/

#include "Multiplexer.h"

Multiplexer::Multiplexer(
    int _maxNumberBytes,            //Maximum number of Bytes in a PDU
    uint8_t _sourceAddress,         //Source MAC Address
    uint8_t _destinationAddress,    //Destination MAC Address
    bool _verbose)                  //Verbosity flag
{
    //Initialize class variables
    currentPDUSize = 4;     //SA, DA and NUM fields in MAC HEADER + CRC
    maxNumberBytes = _maxNumberBytes;
    sduBuffer = new char[maxNumberBytes];
    sourceAddress = _sourceAddress;
    destinationAddress = _destinationAddress;
    numberSDUs = 0;
    controlOffset = 0;
    verbose = _verbose;
}

Multiplexer::Multiplexer(
    uint8_t* pdu,          //Buffer containing PDU
    bool _verbose)      //Verbosity flag
{
    verbose = _verbose;
    sduBuffer = (char*)pdu;
    offset = 0;
    currentPDUSize = 0;
}

Multiplexer::~Multiplexer(){
    delete[] sduBuffer;
}

int 
Multiplexer::currentBufferLength(){
    int length = 0;
    for(int i=0;i<numberSDUs;i++){
        length += sizesSDUs[i];
    }
    return length;
}

int 
Multiplexer::getNumberofBytes(){
    return currentPDUSize;
}

bool 
Multiplexer::addSduPosition(
    char* sdu,                  //Buffer containing single SDU
    uint16_t size,              //SDU size
    uint8_t flagDataControl,    //SDU Data/Control flag
    int position)               //Position in the queue where SDU will be added
{
    int bufferOffset = 0;                   //Buffer offset to manage copying arrays
    int length = currentBufferLength();     //Current length of the queue

    //Resize vectors:
    sizesSDUs.resize(numberSDUs+1);
    flagsDataControlSDUs.resize(numberSDUs+1);

    //Copying forward information already in queue
    for(int i=numberSDUs-1;i>=position;i--){
        sizesSDUs[i+1] = sizesSDUs[i];
        flagsDataControlSDUs[i+1] = flagsDataControlSDUs[i];
        bufferOffset+=sizesSDUs[i+1];
    }
    for(int i=(length-1);i>=(length-bufferOffset);i--){
        sduBuffer[i+size] = sduBuffer[i];
    }

    //Implanting actual SDU
    sizesSDUs[position] = size;
    flagsDataControlSDUs[position] = flagDataControl;
    for(int i=0;i<size;i++)
        sduBuffer[length-bufferOffset+i] = sdu[i];
    numberSDUs++;
    currentPDUSize += 2 + size;     //2 for D/C flag + size fields
    if(!flagDataControl) controlOffset++;
    if(verbose) cout<<"[Multiplexer] Multiplexed "<<(int)numberSDUs<<" SDUs into PDU."<<endl;
    return true;
}

bool 
Multiplexer::addSDU(
    char* sdu,                  //Buffer containing single SDU
    uint16_t size,              //SDU size
    uint8_t flagDataControl)    //SDU Data/Control flag
{
    //Verify if it is possible to insert SDU (considering CRC)
    if((size+2+currentPDUSize)>maxNumberBytes){
        if(verbose) cout<<"[Multiplexer] Tried to multiplex SDU which size extrapolates maxNumberBytes."<<endl;
        exit(4);
    }

    //Adds SDU to position depending if it is Data or Control SDU
    return flagDataControl? addSduPosition(sdu, size, flagDataControl, numberSDUs):addSduPosition(sdu, size, flagDataControl, controlOffset);
}

ssize_t 
Multiplexer::getSDU(
    char* sdu)      //Buffer to store SDU
{
    //Test if Decoding queue has ended
    if(offset == numberSDUs){
        if(verbose) cout<<"[Multiplexer] End of demultiplexing."<<endl;
        return -1;
    }

    //Set an position offset to next SDU position in buffer
    int positionBuffer = 0;
    for(int i=0;i<offset;i++)
        positionBuffer+=sizesSDUs[i];
    
    //Copy SDU from buffer
    for(int i=0;i<sizesSDUs[offset];i++)
        sdu[i] = sduBuffer[positionBuffer+i];
    
    //Increment decoding offset
    offset++;
    if(verbose) cout<<"[Multiplexer] Demultiplexed SDU "<<offset<<endl;
    return sizesSDUs[offset-1];
}

void 
Multiplexer::getPDU(
    vector<uint8_t> & pduBuffer)
{
    //Create new PDU object and inserts MAC Header
    insertMacHeader();
    
    //Copy PDU to buffer
    for(int i=0;i<currentPDUSize-2;i++)     //-2 because of CRC
        pduBuffer.push_back(sduBuffer[i]);
}

uint8_t 
Multiplexer::getCurrentDataControlFlag(){
    return flagsDataControlSDUs[offset-1];
}

uint8_t 
Multiplexer::getDestinationAddress(){
    return destinationAddress;
}

void 
Multiplexer::insertMacHeader(){
    int i, j;   //Auxiliary variables

    //Allocate new buffer
    char* buffer = new char[currentPDUSize-2];  //-2 because of CRC

    //Fills the 2 first slots
    buffer[0] = (sourceAddress<<4)|(destinationAddress&15);
    buffer[1] = numberSDUs;

    //Fills with the SDUs informations
    for(i=0;i<numberSDUs;i++){
        buffer[2+2*i] = (flagsDataControlSDUs[i]<<7)|(sizesSDUs[i]>>8);
        buffer[3+2*i] = sizesSDUs[i]&255;
    }

    //Copy the buffer with the SDUs multiplexed to the new buffer
    for(i=2+2*i,j=0;i<currentPDUSize-2;i++,j++){
        buffer[i] = sduBuffer[j];
    }

    memcpy(sduBuffer, buffer, currentPDUSize-2);

    delete [] buffer;

    if(verbose) cout<<"[Multiplexer] MAC Header inserted."<<endl;
}

void 
Multiplexer::removeMacHeader(){
    int i;  //Auxiliary variable

    //Get information from first 2 slots
    sourceAddress = (uint8_t) (sduBuffer[0]>>4);
    destinationAddress = (uint8_t) (sduBuffer[0]&15);
    numberSDUs = (uint8_t) sduBuffer[1];

    //Declare new sizes and Data/Control flags arrays and get information for SDUs decoding
    for(i=0;i<numberSDUs;i++){
        flagsDataControlSDUs.push_back((sduBuffer[2+2*i]&255)>>7);
        sizesSDUs.push_back(((sduBuffer[2+2*i]&127)<<8)|((sduBuffer[3+2*i])&255));
        currentPDUSize+=sizesSDUs[i];
    }

    //Update i value to use in further loop
    i = 2 + 2*i;

    //Create a new buffer that will contain only SDUs, no header
    char* buffer2 = new char[currentPDUSize];

    for(int j=0;j<currentPDUSize;i++,j++){
        buffer2[j]=sduBuffer[i];
    }
    
    sduBuffer = buffer2;
    if(verbose) cout<<"[Multiplexer] MAC Header removed successfully."<<endl;
}

bool
Multiplexer::isEmpty(){
    return numberSDUs==0;
}
