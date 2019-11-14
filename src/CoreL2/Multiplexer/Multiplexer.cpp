/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "Multiplexer.h"

/**
 * @brief Constructs a Multiplexer with information necessary to manage the queues
 * @param nB Maximum number of bytes supported in a single PDU
 * @param _srcMac Source MAC Address
 * @param _arp Static declared MacAddressTable
 * @param _maxSDUs Maximum number of SDUs supported in a single PDU
 * @param v Verbosity flag
 */
Multiplexer::Multiplexer(uint16_t nB, uint8_t _srcMac, MacAddressTable* _arp, int _maxSDUs, bool v){
    TransmissionQueues = new TransmissionQueue*[MAX_BUFFERS];
    srcMac = _srcMac;
    dstMac = new uint8_t[MAX_BUFFERS];
    nBytes = new uint16_t[MAX_BUFFERS];
    maxNBytes = nB;
    nTransmissionQueues = 0;
    arp = _arp;
    maxSDUs = _maxSDUs;
    verbose = v;
    if(v) cout<<"[Multiplexer] Created successfully."<<endl;
}

/**
 * @brief Destroys the Multiplexer object and unnalocates memory
 */
Multiplexer::~Multiplexer(){
    for(int i=0;i<nTransmissionQueues;i++)
        delete TransmissionQueues[i];
    delete[] TransmissionQueues;
    delete[] dstMac;
    delete[] nBytes; 
}

/**
 * @brief Defines a new TransmissionQueue to specific destination and adds
 * it to array of TransmissionQueues in the Multiplexer.
 * 
 * @param _dstMac Destination MAC Address
 */
void 
Multiplexer::setTransmissionQueue(uint8_t _dstMac){
    //Check if array is full
    if(nTransmissionQueues>MAX_BUFFERS && verbose){
        cout<<"[Multiplexer] Trying to create more buffers than supported."<<endl;
        exit(1);
    }

    //Allocate new TransmissionQueue and stores it in array
    TransmissionQueues[nTransmissionQueues] = new TransmissionQueue(maxNBytes, srcMac, _dstMac, maxSDUs, verbose);
    
    //Initialize values of number of bytes and data/control flag
    dstMac[nTransmissionQueues] = _dstMac;
    nBytes[nTransmissionQueues] = 0;

    //Increment counter
    nTransmissionQueues++;
}

/**
 * @brief Adds a new DATA SDU to the TransmissionQueue that corresponds with IP Address of the L3 Packet
 * @param sdu Data SDU received from TUN interface
 * @param n Number of bytes in SDU
 * @returns -1 if successfull; index of queue to send data if queue is full for Tx; -2 for errors
 */
int 
Multiplexer::addSdu(char* sdu, uint16_t n){
    uint8_t mac;
    uint8_t ipAddr[4];

    //Gets IP Address from packet
    for(int i=0;i<4;i++)
        ipAddr[i] = (uint8_t) sdu[DST_OFFSET+i]; //Copying IP address
    
    //Search IP Address in MacAddressTable
    mac = arp->getMacAddress(ipAddr);

    return addSdu(sdu, n, 1, mac);
}

/**
 * @brief Adds a new SDU to the TransmissionQueue that corresponds with MAC address passed as parameter
 * @param sdu SDU buffer
 * @param n Number of bytes of Control SDU
 * @param dc Data/Control Flag
 * @param _dstMac Destination MAC Address
 * @returns -1 if successfull; index of queue to send data if queue is full for Tx; -2 for errors
 */
int 
Multiplexer::addSdu(char* sdu, uint16_t n, uint8_t dc, uint8_t _dstMac){
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
    if((n + 2 + TransmissionQueues[i]->getNumberofBytes())>maxNBytes){
        if(verbose) cout<<"[Multiplexer] Number of bytes exceed buffer max length. Returning index."<<endl;
        return i;
    }

    //Test if there number of SDUs extrapolates maximum
    if(TransmissionQueues[i]->nSDUs+1 == TransmissionQueues[i]->maxNSDUs){
        if(verbose) cout<<"[TransmissionQueue] Tried to multiplex more SDUs than supported."<<endl;
        return i;
    }

    //Attepts to add SDU to TransmissionQueue
    if(TransmissionQueues[i]->addSDU(sdu, n, dc)){
        nBytes[i]+=n;
        if(verbose&&dc) cout<< "[Multiplexer] Data SDU added to queue!"<<endl;
        if(verbose&&!dc) cout<< "[Multiplexer] Control SDU added to queue!"<<endl;
        return -1;
    }
    return -2; 
}

/**
 * @brief Gets the multiplexed PDU with MacHeader from TransmissionQueue identified by index
 * @param buffer Buffer where PDU will be stored
 * @param index Identification of the TransmissionQueue to get the PDU
 * @returns Size of the PDU
 */
ssize_t 
Multiplexer::getPdu(char* buffer, int index){
    ssize_t size;

    //Test if index is valid, considering TransmissionQueues index is sequential
    if(index>=nTransmissionQueues){
        if(verbose) cout<<"[Multiplexer] Could not get PDU: index out of bounds."<<endl;
        return -1;
    }

    //Test if there are bytes to return
    if(nBytes[index] == 0){
        if(verbose) cout<<"[Multiplexer] Could not get PDU: no Bytes to transfer."<<endl;
        return -1;
    }

    //Creates a ProtocolPackage to receive the PDU
    ProtocolPackage* pdu = TransmissionQueues[index]->getPDUPackage();

    if(verbose) cout<<"[Multiplexer] Inserting MAC Header."<<endl;

    //Inserts MacHeader and returns PDU size
    pdu->insertMacHeader();
    size = pdu->getPduSize();
    memcpy(buffer, pdu->buffer, size);
    delete pdu;

    TransmissionQueues[index]->clearBuffer();
    nBytes[index] = 0;

    return size;
}

/**
 * @brief Verifies if PDU is empty
 * @param index Identification of TransmissionQueue where PDI is being stored
 * @returns true if empty; false otherwise
 */
bool Multiplexer::emptyPdu(int index){
    if(index>=nTransmissionQueues) return true;
    return nBytes[index]==0;
}

/**
 * @brief Gets the number of TransmissionQueues allocated in the Multiplexer
 * @returns Number of TransmissionQueues
 */
int Multiplexer::getNTransmissionQueues(){
    return nTransmissionQueues;
}

