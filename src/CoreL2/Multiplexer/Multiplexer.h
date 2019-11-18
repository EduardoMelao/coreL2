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
#define DST_OFFSET 16       //IP address offset in L3 packet
using namespace std;

class Multiplexer{
private:
    TransmissionQueue** transmissionQueues;     //Array of TransmissionQueues to manage SDU queues
    uint8_t srcMac;                 //Source MAC Address
    uint8_t* dstMac;                //Destination MAC5GR for each TransmissionQueue
    uint16_t* numberBytes;          //Number of bytes for each IP Addr
    uint16_t maxNumberBytes;        //Maximum number of bytes to fill PDU
    int nTransmissionQueues;        //Number of TransmissionQueues actually stored in Multiplexer
    int maxSDUs;                    //Maximum number of SDUs multiplexed
    MacAddressTable* arp;           //Table of correlation of IP Addrs and MAC5GR Addrs
    bool verbose;                   //Verbosity flag
public:
    /**
     * @brief Constructs a Multiplexer with information necessary to manage the queues
     * @param _maxNumberBytes Maximum number of bytes supported in a single PDU
     * @param _srcMac Source MAC Address
     * @param _arp Static declared MacAddressTable
     * @param _maxSDUs Maximum number of SDUs supported in a single PDU
     * @param _verbose Verbosity flag
     */
    Multiplexer(uint16_t _maxNumberBytes, uint8_t _srcMac, MacAddressTable* _arp, int _maxSDUs, bool _verbose);
    
    /**
     * @brief Destroys the Multiplexer object and unnalocates memory
     */
    ~Multiplexer();

    /**
     * @brief Defines a new TransmissionQueue to specific destination and adds
     * it to array of TransmissionQueues in the Multiplexer.
     * 
     * @param _dstMac Destination MAC Address
     */
    void setTransmissionQueue(uint8_t _dstMac);
    
    /**
     * @brief Adds a new DATA SDU to the TransmissionQueue that corresponds with IP Address of the L3 Packet
     * @param sdu Data SDU received from TUN interface
     * @param n Number of bytes in SDU
     * @returns -1 if successfull; index of queue to send data if queue is full for Tx; -2 for errors
     */
    int addSdu(char* sdu, uint16_t size);
    
    /**
     * @brief Adds a new SDU to the TransmissionQueue that corresponds with MAC address passed as parameter
     * @param sdu SDU buffer
     * @param n Number of bytes of SDU
     * @param flagDC Data/Control Flag
     * @param _dstMac Destination MAC Address
     * @returns -1 if successfull; index of queue to send data if queue is full for Tx; -2 for errors
     */    
    int addSdu(char* sdu, uint16_t n, uint8_t flagDC, uint8_t _dstMac);
    
    /**
     * @brief Gets the multiplexed PDU with MacHeader from TransmissionQueue identified by index
     * @param buffer Buffer where PDU will be stored
     * @param index Identification of the TransmissionQueue to get the PDU
     * @returns Size of the PDU
     */    
    ssize_t getPdu(char* buffer, int index);
    
    /**
     * @brief Verifies if PDU is empty
     * @param index Identification of TransmissionQueue where PDU is being stored
     * @returns true if empty; false otherwise
     */    
    bool emptyPdu(int index);
    
    /**
     * @brief Gets the number of TransmissionQueues allocated in the Multiplexer
     * @returns Number of TransmissionQueues
     */    
    int getNTransmissionQueues();
};
