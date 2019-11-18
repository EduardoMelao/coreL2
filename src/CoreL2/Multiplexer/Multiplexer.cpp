/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "Multiplexer.h"

Multiplexer::Multiplexer(
        uint16_t _maxNumberBytes,   //Maximum number of Bytes in PDU
        uint8_t _srcMac,            //Source MAC Address
        MacAddressTable* _arp,      //MAC - IP table   
        int _maxSDUs,               //Maximum number of SDUs in PDU
        bool _verbose)             //Verbosity flag
{
    transmissionQueues = new TransmissionQueue*[MAX_BUFFERS];
    srcMac = _srcMac;
    dstMac = new uint8_t[MAX_BUFFERS];
    numberBytes = new uint16_t[MAX_BUFFERS];
    maxNumberBytes = _maxNumberBytes;
    nTransmissionQueues = 0;
    arp = _arp;
    maxSDUs = _maxSDUs;
    verbose = _verbose;
    if(_verbose) cout<<"[Multiplexer] Created successfully."<<endl;
}

Multiplexer::~Multiplexer()
{
    for(int i=0;i<nTransmissionQueues;i++)
        delete transmissionQueues[i];
    delete[] transmissionQueues;
    delete[] dstMac;
    delete[] numberBytes; 
}

void 
Multiplexer::setTransmissionQueue(
    uint8_t _dstMac)    //Destination MAC Address
{
    //Check if array is full
    if(nTransmissionQueues>MAX_BUFFERS && verbose){
        cout<<"[Multiplexer] Trying to create more buffers than supported."<<endl;
        exit(1);
    }

    //Allocate new TransmissionQueue and stores it in array
    transmissionQueues[nTransmissionQueues] = new TransmissionQueue(maxNumberBytes, srcMac, _dstMac, maxSDUs, verbose);
    
    //Initialize values of number of bytes and data/control flag
    dstMac[nTransmissionQueues] = _dstMac;
    numberBytes[nTransmissionQueues] = 0;

    //Increment counter
    nTransmissionQueues++;
}

int 
Multiplexer::addSdu(
    char* sdu,          //Data SDU received from TUN
    uint16_t size)      //Number of Bytes in SDU
{
    uint8_t mac;        //MAC address of destination of the SDU
    uint8_t ipAddr[4];  //Destination IP address encapsuled into SDU

    //Gets IP Address from packet
    for(int i=0;i<4;i++)
        ipAddr[i] = (uint8_t) sdu[DST_OFFSET+i]; //Copying IP address
    
    //Search IP Address in MacAddressTable
    mac = arp->getMacAddress(ipAddr);

    return addSdu(sdu, size, 1, mac);
}

int 
Multiplexer::addSdu(
    char* sdu,          //SDU buffer
    uint16_t size,         //Number of Bytes of SDU
    uint8_t flagDC,     //Data/Control flag
    uint8_t _dstMac)    //Destination MAC Address
{
    int i;

    //Look for TransmissionQueue corresponding to Mac Address
    for(i=0;i<nTransmissionQueues;i++)
        if(dstMac[i]==_dstMac)
            break;

    //TransmissionQueue not found
    if(i==nTransmissionQueues){
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
    if(transmissionQueues[i]->numberSDUs+1 == transmissionQueues[i]->maxNSDUs){
        if(verbose) cout<<"[TransmissionQueue] Tried to multiplex more SDUs than supported."<<endl;
        return i;
    }

    //Attempts to add SDU to TransmissionQueue
    if(transmissionQueues[i]->addSDU(sdu, size, flagDC)){
        numberBytes[i]+=size;
        if(verbose&&flagDC) cout<< "[Multiplexer] Data SDU added to queue!"<<endl;
        if(verbose&&!flagDC) cout<< "[Multiplexer] Control SDU added to queue!"<<endl;
        return -1;
    }
    return -2; 
}

ssize_t 
Multiplexer::getPdu(
    char* buffer,   //Buffer to store PDU
    int index)      //Index of TransmissionQueue where PDU is stored
{
    ssize_t size;

    //Test if index is valid, considering transmissionQueues index is sequential
    if(index>=nTransmissionQueues){
        if(verbose) cout<<"[Multiplexer] Could not get PDU: index out of bounds."<<endl;
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
    int index)      //Index of TransmissionQueue where PDU is stored
{
    if(index>=nTransmissionQueues) return true;
    return numberBytes[index]==0;
}

int 
Multiplexer::getNTransmissionQueues(){
    return nTransmissionQueues;
}

