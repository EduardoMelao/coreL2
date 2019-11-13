/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#pragma once
#include <iostream>
#include <list>
#include <string.h>

#include "../Multiplexer/TransmissionQueue.h"
#include "../ProtocolPackage/ProtocolPackage.h"
#include "../ProtocolPackage/MacAddressTable/MacAddressTable.h"

#define MAX_BUFFERS 8       //Maximum number of sdu buffers (maximum destinations)
#define DST_OFFSET 16
using namespace std;

class Multiplexer{
private:
    TransmissionQueue** TransmissionQueues;     //Array of TransmissionQueues to manage SDU queues
    uint8_t srcMac;
    uint8_t* dstMac;                //Destination MAC5GR for each TransmissionQueue
    uint16_t* nBytes;               //Number of bytes for each IP Addr
    uint16_t maxNBytes;             //Maximum number of bytes to fill PDU
    int nTransmissionQueues;              //Number of TransmissionQueues actually stored in Multiplexer
    int maxSDUs;                    //Maximum number of SDUs multiplexed
    MacAddressTable* arp;           //Table of correlation of IP Addrs and MAC5GR Addrs
    bool verbose;                   //Verbose flag
    void clearBuffer(int index);
public:
    Multiplexer(uint16_t nB, uint8_t _srcMac, MacAddressTable* _arp, int _maxSDUS,  bool v);
    ~Multiplexer();
    void setTransmissionQueue(uint8_t _dstMac);
    int addSdu(char* sdu, uint16_t n);   //Attempts to add SDU to queue. If queue is full for Tx, returns the index to get PDU. Else returns -1.
    int addSdu(char* sdu, uint16_t n, uint8_t dc, uint8_t _dstMac);   //Attempts to add SDU to queue. If queue is full for Tx, returns the index to get PDU. Else returns -1.
    ssize_t getPdu(char* buffer, int index);      //Get full PDU for transmission
    bool emptyPdu(int index);
    int getNTransmissionQueues();
};
