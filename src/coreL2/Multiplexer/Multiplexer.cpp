/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : Multiplexer.cpp
@Classification : Multiplexer
@
@Last alteration : February 13th, 2020
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module manages the queues of SDUs that will generate PDUs 
    to send to destination.
*/

#include "Multiplexer.h"

Multiplexer::Multiplexer(
    int _numberDestinations,        //Number of destinations with PDUs to aggregate
    uint8_t _sourceMac,             //Source MAC Address
    uint8_t* _destinationMacX,      //Destination MAC Addresses
    uint16_t* _maxNumberBytesX,     //Maximum number of Bytes for each Buffer of PDUs
    bool _verbose)                  //Verbosity flag
{
    numberDestinations = _numberDestinations;
    sourceMac = _sourceMac;
    destinationMacX = _destinationMacX;
    maxNumberBytesX = _maxNumberBytesX;
    verbose = _verbose;

    //Alloc array of number of Bytes aggregated and AggregationQueues for each destination
    numberBytesAgrX = new uint16_t[numberDestinations];
    aggregationQueueX = new AggregationQueue*[numberDestinations];

    //Initialize each position of arrays
    for(int i=0;i<numberDestinations;i++){
    	numberBytesAgrX[i] = 0;
        aggregationQueueX[i] = new AggregationQueue(maxNumberBytesX[i], sourceMac, destinationMacX[i], verbose);
    }
    if(_verbose) cout<<"[Multiplexer] Created successfully."<<endl;
}

Multiplexer::~Multiplexer()
{
    for(int i=0;i<numberDestinations;i++)
        delete aggregationQueueX[i];
    delete[] aggregationQueueX;
    delete[] numberBytesAgrX;
}

void 
Multiplexer::addSdu(
    char* sdu,                  //SDU buffer
    uint16_t size,              //Number of Bytes of SDU
    uint8_t flagDataControl,    //Data/Control flag
    uint8_t destinationMac)     //Destination MAC Address
{
    int index = getAggregationQueueIndex(destinationMac);

    //Test if index is valid
    if(index==-1){
        if(verbose) cout<<"[Multiplexer] Bad MAC Address found trying to Add SDU to Aggregation Queue."<<endl;
        return;
    }

    //Test if queue is full
    if(aggregationQueueFull(destinationMac, size)){
        if(verbose) cout<<"[Multiplexer] Number of bytes exceed buffer max length."<<endl;
        return;
    }

    //Add SDU to AggregationQueue
    if(aggregationQueueX[index]->addSDU(sdu, size, flagDataControl)){
        numberBytesAgrX[index]+=size;
        if(verbose&&flagDataControl) cout<< "[Multiplexer] Data SDU added to queue!"<<endl;
        if(verbose&&!flagDataControl) cout<< "[Multiplexer] Control SDU added to queue!"<<endl;
    }
}

ssize_t 
Multiplexer::getPdu(
    char* buffer,       //Buffer to store PDU
    uint8_t macAddress) //Destination MAC Address of PDU
{
    ssize_t size;   //Size of PDU
    int index = getAggregationQueueIndex(macAddress);

    //Test if index is valid
    if(index==-1){
        if(verbose) cout<<"[Multiplexer] Bad MAC Address found trying to Get SDU from Aggregation Queue."<<endl;
        return -1;
    }

    //Test if there are bytes to return
    if(numberBytesAgrX[index] == 0){
        if(verbose) cout<<"[Multiplexer] Could not get PDU: no Bytes to transfer."<<endl;
        return -1;
    }

    //Creates a ProtocolPackage to receive the PDU
    ProtocolPackage* pdu = aggregationQueueX[index]->getPDUPackage();

    if(verbose) cout<<"[Multiplexer] Inserting MAC Header."<<endl;

    //Inserts MacHeader and returns PDU size
    pdu->insertMacHeader();
    size = pdu->getPduSize();
    memcpy(buffer, pdu->buffer, size);
    delete pdu;

    return size;
}

int 
Multiplexer::getNumberDestinations(){
    return numberDestinations;
}

bool
Multiplexer::aggregationQueueFull(
    uint8_t macAddress,     //Destination MAC Address
    uint16_t sduSize)       //Size of SDU to be enqueued
{
    int index = getAggregationQueueIndex(macAddress);

    //Verify bad macAddress
    if(index==-1){
        if(verbose) cout<<"[Multiplexer] MAC Address not found verifying if Aggregation Queue is full"<<endl;
        exit(5); 
    }

    //Returns if size of:
    //  -> sduSize: Size of SDU that wants to be added by Multiplexer
    //  -> 2: CRC
    //  -> 2: Header additions - Size (15bits) + Flag D/C (1 bit)
    //  -> AggregationQueue::getNumberofBytes(): Size of SDUs aggregated + Header of them
    //is bigger tham maximum number of bytes
    return ((sduSize + 2 + 2 + aggregationQueueX[index]->getNumberofBytes())>maxNumberBytesX[index]);
}

int
Multiplexer::getAggregationQueueIndex(
    uint8_t macAddress)     //Destination MAC Address
{
    for(int i=0;i<numberDestinations;i++){
        if(destinationMacX[i]==macAddress)
            return i;
    }
    return -1;
}
