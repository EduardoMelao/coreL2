/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : Multiplexer.cpp
@Classification : Multiplexer
@
@Last alteration : November 19th, 2019
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module manages the queues of SDUs that will generate PDUs 
    to send to destination. This module is used on Transmission only.
*/

#include "Multiplexer.h"

Multiplexer::Multiplexer(
        uint16_t _maxNumberBytes,       //Maximum number of Bytes in PDU
        uint8_t _sourceMac,             //Source MAC Address
        MacAddressTable* _ipMacTable,   //MAC - IP table   
        int _maxSDUs,                   //Maximum number of SDUs in PDU
        bool _verbose)                  //Verbosity flag
{
    transmissionQueues = new TransmissionQueue*[MAX_BUFFERS];
    sourceMac = _sourceMac;
    destinationMac = new uint8_t[MAX_BUFFERS];
    numberBytes = new uint16_t[MAX_BUFFERS];
    maxNumberBytes = _maxNumberBytes;
    numberTransmissionQueues = 0;
    ipMacTable = _ipMacTable;
    maxSDUs = _maxSDUs;
    verbose = _verbose;
    if(_verbose) cout<<"[Multiplexer] Created successfully."<<endl;
}

Multiplexer::~Multiplexer()
{
    for(int i=0;i<numberTransmissionQueues;i++)
        delete transmissionQueues[i];
    delete[] transmissionQueues;
    delete[] destinationMac;
    delete[] numberBytes; 
}

void 
Multiplexer::setTransmissionQueue(
    uint8_t _destinationMac)    //Destination MAC Address
{
    //Check if array is full
    if(numberTransmissionQueues>MAX_BUFFERS && verbose){
        cout<<"[Multiplexer] Trying to create more buffers than supported."<<endl;
        exit(1);
    }

    //Allocate new TransmissionQueue and stores it in array
    transmissionQueues[numberTransmissionQueues] = new TransmissionQueue(maxNumberBytes, sourceMac, _destinationMac, maxSDUs, verbose);
    
    //Initialize values of number of bytes and data/control flag
    destinationMac[numberTransmissionQueues] = _destinationMac;
    numberBytes[numberTransmissionQueues] = 0;

    //Increment counter
    numberTransmissionQueues++;
}

int 
Multiplexer::addSdu(
    char* sdu,          //Data SDU received from TUN
    uint16_t size)      //Number of Bytes in SDU
{
    uint8_t mac;        //MAC address of destination of the SDU
    uint8_t ipAddress[4];  //Destination IP address encapsulated into SDU

    //Gets IP Address from packet
    for(int i=0;i<4;i++)
        ipAddress[i] = (uint8_t) sdu[DST_OFFSET+i]; //Copying IP address
    
    //Search IP Address in MacAddressTable
    mac = ipMacTable->getMacAddress(ipAddress);

    return addSdu(sdu, size, 1, mac);
}

int 
Multiplexer::addSdu(
    char* sdu,                  //SDU buffer
    uint16_t size,              //Number of Bytes of SDU
    uint8_t flagDataControl,    //Data/Control flag
    uint8_t _destinationMac)    //Destination MAC Address
{
    int i;

    //Look for TransmissionQueue corresponding to Mac Address
    for(i=0;i<numberTransmissionQueues;i++)
        if(destinationMac[i]==_destinationMac)
            break;

    //TransmissionQueue not found
    if(i==numberTransmissionQueues){
        if(verbose){
            cout<<"[Multiplexer] Error: no TransmissionQueue found."<<endl;
            return -2;
        }
    }

    //Test if queue is full: if so, returns the index
    if((size + 2 + transmissionQueues[i]->getNumberofBytes())>maxNumberBytes){
        if(verbose) cout<<"[Multiplexer] Number of bytes exceed buffer max length. Returning index."<<endl;
        return i;
    }

    //Test if there number of SDUs extrapolates maximum
    if(transmissionQueues[i]->numberSDUs+1 == transmissionQueues[i]->maximumNumberSDUs){
        if(verbose) cout<<"[TransmissionQueue] Tried to multiplex more SDUs than supported."<<endl;
        return i;
    }

    //Attempts to add SDU to TransmissionQueue
    if(transmissionQueues[i]->addSDU(sdu, size, flagDataControl)){
        numberBytes[i]+=size;
        if(verbose&&flagDataControl) cout<< "[Multiplexer] Data SDU added to queue!"<<endl;
        if(verbose&&!flagDataControl) cout<< "[Multiplexer] Control SDU added to queue!"<<endl;
        return -1;
    }
    return -2; 
}

ssize_t 
Multiplexer::getPdu(
    char* buffer,       //Buffer to store PDU
    uint8_t macAddress) //Destination MAC Address of PDU
{
    ssize_t size;   //Size of PDU
    int index;      //Auxiliary variable for loop

    for(index=0;index<numberTransmissionQueues;index++){
        if(destinationMac[index]==macAddress)
            break;
    }
    //Test if macAddress was found
    if(index==numberTransmissionQueues){
        if(verbose) cout<<"[Multiplexer] Could not get PDU: MAC Address not found."<<endl;
        return -1;
    }

    //Test if there are bytes to return
    if(numberBytes[index] == 0){
        if(verbose) cout<<"[Multiplexer] Could not get PDU: no Bytes to transfer."<<endl;
        return -1;
    }

    //Creates a ProtocolPackage to receive the PDU
    ProtocolPackage* pdu = transmissionQueues[index]->getPDUPackage();

    if(verbose) cout<<"[Multiplexer] Inserting MAC Header."<<endl;

    //Inserts MacHeader and returns PDU size
    pdu->insertMacHeader();
    size = pdu->getPduSize();
    memcpy(buffer, pdu->buffer, size);
    delete pdu;

    transmissionQueues[index]->clearBuffer();
    numberBytes[index] = 0;

    return size;
}

bool 
Multiplexer::emptyPdu(
    uint8_t macAddress)      //Destination MAC Address of PDU
{
    for(int i=0;i<numberTransmissionQueues;i++){
        if(destinationMac[i]==macAddress)
            return numberBytes[i]==0;
    }
    if(verbose) cout<<"[Multiplexer] MAC address not found verifying empty PDU."<<endl;
    return true;
}

int 
Multiplexer::getNumberTransmissionQueues(){
    return numberTransmissionQueues;
}

