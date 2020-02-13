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

#include "../Multiplexer/AggregationQueue.h"
#include "../ProtocolPackage/ProtocolPackage.h"
#include "MacAddressTable/MacAddressTable.h"

#define MAX_BUFFERS 8       //Maximum number of SDU buffers (maximum destinations)
using namespace std;

class Multiplexer{
private:
    int numberDestinations;                     //Number of destinations actually stored in Multiplexer
    AggregationQueue** aggregationQueueX;     //Array of AggregationQueues to manage SDU queues
    uint8_t sourceMac;                          //Source MAC Address
    uint8_t* destinationMacX;                   //Destination MAC5GR Address for each AggregationQueue
    uint16_t* numberBytesAgrX;                  //Number of bytes for each UE
    uint16_t* maxNumberBytesX;                  //Maximum number of bytes to fill PDUx
    bool verbose;                               //Verbosity flag
public:
    /**
     * @brief Constructs a Multiplexer with information necessary to manage the queues
     * @param _numberDestinations Number of destinations with PDUs to aggregate
     * @param _sourceMac Source MAC Address
     * @param _destinationMacX Destination MAC Addresses
     * @param _maxNumberBytesX Maximum number of Bytes for each Buffer of PDUs
     * @param _verbose Verbosity flag
     */
    Multiplexer(int _numberDestinations, uint8_t _sourceMac, uint8_t* _destinationMacX, uint16_t* _maxNumberBytesX, bool _verbose);
    
    /**
     * @brief Destroys the Multiplexer object and unallocates memory
     */
    ~Multiplexer();
    
    /**
     * @brief Adds a new SDU to the AggregationQueue that corresponds with MAC address passed as parameter
     * @param sdu SDU buffer
     * @param size Number of bytes of SDU
     * @param flagDataControl Data/Control Flag
     * @param destinationMac Destination MAC Address
     */    
    void addSdu(char* sdu, uint16_t size, uint8_t flagDataControl, uint8_t destinationMac);
    
    /**
     * @brief Gets the multiplexed PDU with MacHeader from AggregationQueue identified by MAC Address
     * @param buffer Buffer where PDU will be stored
     * @param macAddress Destination MAC Address of PDU
     * @returns Size of the PDU
     */    
    ssize_t getPdu(char* buffer, uint8_t macAddress);
    
    /**
     * @brief Gets the number of AggregationQueues allocated in the Multiplexer
     * @returns Number of AggregationQueues
     */    
    int getNumberDestinations();

    /**
     * @brief Informs if SDU can be added to AggregationQueue to certain UE
     * @param macAddress Destination MAC Address
     * @param sduSize Size of SDU in Bytes
     * @returns TRUE if queue is full; FALSE if SDU is supported
     */
    bool aggregationQueueFull(uint8_t macAddress, uint16_t sduSize);

    /**
     * @brief Gets the index refering to destination MAC queue in Multiplexer
     * @param macAddress Destination MAC address 
     * @returns Index of AggregationQueue; -1 if macAddress not found
     */
    int getAggregationQueueIndex(uint8_t macAddress);
};
#endif  //INCLUDED_MULTIPLEXER_H
