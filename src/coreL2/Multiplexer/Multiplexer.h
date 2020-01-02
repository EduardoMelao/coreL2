/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_MULTIPLEXER_H
#define INCLUDED_MULTIPLEXER_H

#include <iostream>
#include <list>
#include <string.h>

#include "../Multiplexer/TransmissionQueue.h"
#include "../ProtocolPackage/ProtocolPackage.h"
#include "MacAddressTable/MacAddressTable.h"

#define MAX_BUFFERS 8       //Maximum number of SDU buffers (maximum destinations)
#define DST_OFFSET 16       //IP address offset in L3 packet
using namespace std;

class Multiplexer{
private:
    TransmissionQueue** transmissionQueues;     //Array of TransmissionQueues to manage SDU queues
    uint8_t sourceMac;                          //Source MAC Address
    uint8_t* destinationMac;                    //Destination MAC5GR for each TransmissionQueue
    uint16_t* numberBytes;                      //Number of bytes for each IP Address
    uint16_t maxNumberBytes;                    //Maximum number of bytes to fill PDU
    int numberTransmissionQueues;               //Number of TransmissionQueues actually stored in Multiplexer
    int maxSDUs;                                //Maximum number of SDUs multiplexed
    MacAddressTable* ipMacTable;                //Table of correlation of IP Addresses and MAC5GR Addresses
    bool flagBS;                                //Flag to know if equipment is BS or UE
    bool verbose;                               //Verbosity flag
public:
    /**
     * @brief Constructs a Multiplexer with information necessary to manage the queues
     * @param _maxNumberBytes Maximum number of bytes supported in a single PDU
     * @param _sourceMac Source MAC Address
     * @param _ipMacTable Static declared MacAddressTable
     * @param _maxSDUs Maximum number of SDUs supported in a single PDU
     * @param _flagBS Flag that is true if equipment is BS
     * @param _verbose Verbosity flag
     */
    Multiplexer(uint16_t _maxNumberBytes, uint8_t _sourceMac, MacAddressTable* _ipMacTable, int _maxSDUs, bool _flagBS, bool _verbose);
    
    /**
     * @brief Destroys the Multiplexer object and unallocates memory
     */
    ~Multiplexer();

    /**
     * @brief Given the SDU, open it and look for its IP in Mac Address Table
     * @param dataSDU SDU containing IP bytes
     * @returns Destination MAC Address
     */
    uint8_t getMacAddress(char* dataSdu);
    
    /**
     * @brief Defines a new TransmissionQueue to specific destination and adds
     * it to array of TransmissionQueues in the Multiplexer.
     * 
     * @param _destinationMac Destination MAC Address
     */
    void setTransmissionQueue(uint8_t _destinationMac);
    
    /**
     * @brief Adds a new DATA SDU to the TransmissionQueue that corresponds with IP Address of the L3 Packet
     * @param sdu Data SDU received from TUN interface
     * @param size Number of bytes in SDU
     * @returns -1 if successful; MAC Address of queue to send data if queue is full for Tx; -2 for errors
     */
    int addSdu(char* sdu, uint16_t size);
    
    /**
     * @brief Adds a new SDU to the TransmissionQueue that corresponds with MAC address passed as parameter
     * @param sdu SDU buffer
     * @param size Number of bytes of SDU
     * @param flagDataControl Data/Control Flag
     * @param _destinationMac Destination MAC Address
     * @returns -1 if successful; MAC Address of queue to send data if queue is full for Tx; -2 for errors
     */    
    int addSdu(char* sdu, uint16_t size, uint8_t flagDataControl, uint8_t _destinationMac);
    
    /**
     * @brief Gets the multiplexed PDU with MacHeader from TransmissionQueue identified by MAC Address
     * @param buffer Buffer where PDU will be stored
     * @param macAddress Destination MAC Address of PDU
     * @returns Size of the PDU
     */    
    ssize_t getPdu(char* buffer, uint8_t macAddress);
    
    /**
     * @brief Verifies if PDU is empty
     * @param macAddress Destination MAC Address of PDU
     * @returns true if empty; false otherwise
     */    
    bool emptyPdu(uint8_t macAddress);
    
    /**
     * @brief Gets the number of TransmissionQueues allocated in the Multiplexer
     * @returns Number of TransmissionQueues
     */    
    int getNumberTransmissionQueues();
};
#endif  //INCLUDED_MULTIPLEXER_H