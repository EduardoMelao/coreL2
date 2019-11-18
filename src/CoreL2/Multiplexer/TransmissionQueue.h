/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#pragma once
#include <iostream>

#include "../ProtocolPackage/ProtocolPackage.h"
#include "../ProtocolPackage/MacAddressTable/MacAddressTable.h"
using namespace std;

//Predefinition of class ProtocolPackage 
class ProtocolPackage;

/**
 * @brief Queue used to store SDUs that will be transmitted to a specific destination
 */
class TransmissionQueue{
private:
    int maxNumberBytes;     //Maximum number of bytes
    char* buffer;           //Buffer accumulates SDUs
    uint8_t srcAddr;        //Source MAC address
    uint8_t dstAddr;        //Destination MAC address
    int offset;             //Offset for decoding
    int controlOffset;      //Offset for encoding Control SDUs
    uint16_t* sizesSDUs;    //Sizes of each SDU multiplexed
    uint8_t* flagsDC;       //Data(1)/Control(0) flag
    bool verbose;           //Verbosity flag
    
    /**
     * @brief Adds SDU to especific position of the multiplexing queue
     * @param sdu Buffer containing single SDU
     * @param size SDU size
     * @param dc SDU D/C flag
     * @param position Position where SDU will be added
     * @returns True if SDU was inserted successfully; False otherwise
     */
    bool addSduPosition(char* sdu, uint16_t size, uint8_t dc, int position);  
    
    /**
     * @brief Returns the current queue buffer length considering the size of each SDU
     * @returns Current TransmissionQueue buffer length in bytes
     */
    int currentBufferLength();     
public:
    uint8_t numberSDUs;          //Number of SDUs multiplexed
    int maxNSDUs;           //Maximum number of SDUs multiplexed

    /**
     * @brief Constructs a new TransmissionQueue to accumulate SDUs for future transmission (encoding)
     * @param maxNumberBytes Maximum number of bytes supported by a single PDU
     * @param src Source MAC Address
     * @param dst Destination MAC Address
     * @param maxSDUs Maximum number of SDUs supported in one queue
     * @param v Verbosity flag
     */
    TransmissionQueue(int _maxNumberBytes, uint8_t src, uint8_t dst, int maxSDUs, bool v);
    
    /**
     * @brief Constructs a TransmissionQueue to help decoding an incoming PDU, dequeueing SDUs contained in it
     * @param _buffer Buffer containing PDU for decoding
     * @param _nSDUs Number of SDUs enqueued in PDU
     * @param _sizesSDUs Array with sizes of each SDU enqueued
     * @param _flagsDC_SDUS Array with Data/Control flags of each SDU
     * @param v Verbosity flag
     */
    TransmissionQueue(char* _buffer, uint8_t _nSDUs, uint16_t* _sizesSDUs, uint8_t* _flagsDC_SDUS, bool v);
    
    /**
     * @brief Destroy a TransmissionQueue object
     */
    ~TransmissionQueue();

    /**
     * @brief Returns the total PDU length in bytes, considering extra overhead of 2 bytes (srcAddr, dstAddr, numSDUs)
     */
    int getNumberofBytes();
    
    /**
     * @brief Adds SDU to the multiplexing queue
     * @param sdu Buffer containing single SDU
     * @param size SDU size
     * @param dc SDU D/C flag
     * @returns True if SDU was inserted successfully; False otherwise
     */     
    bool addSDU(char* sdu, uint16_t size, uint8_t flagDC);

    /**
     * @brief Function used for getting SDUs while decoding
     * @param buf Buffer to store SDU
     * @returns Size of next SDU multiplexed; Returns 0 for EOF
     */
    ssize_t getSDU(char* buf);
    
    /**
     * @brief Creates a new ProtocolPackage object based on information stored in class variables on encoding process
     * @returns ProtocolPackage for the SDUs multiplexed
     * Requires clearing the buffer before using this TransmissionQueue again
     */
    ProtocolPackage* getPDUPackage();
    
    /**
     * @brief Clears the TransmissionQueue buffer; Make sure the PDU is sent before clearing the buffer
     */   
    void clearBuffer(); 

    /**
     * @brief Gets information of Data/Control flag of previous SDU to be used in decoding
     * @returns Data(1)/Control(0) flag for SDU immediately behind (it is supposed getSDU() has been used before) current SDU in decoding queue
     */
    uint8_t getCurrentDCFlag();

    /**
     * @brief Gets destination MAC address of TransmissionQueue object
     * @returns Destination MAC address
     */
    uint8_t getDstAddr();
};
