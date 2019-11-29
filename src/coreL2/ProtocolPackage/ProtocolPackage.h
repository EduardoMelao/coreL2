/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_PROTOCOL_PACKAGE_H
#define INCLUDED_PROTOCOL_PACKAGE_H

#include <stdint.h> //uint8_t
#include <iostream> //cout
#include <string.h>	//memcpy
using namespace std;

#include "../Multiplexer/TransmissionQueue.h"

//Predefinition of class TransmissionQueue 
class TransmissionQueue;

/**
 * @brief Class used to build MAC Header with all information it needs
 */
class ProtocolPackage{
private:
    uint8_t sourceAddress;          //Source MAC Address (4 bits)
    uint8_t destinationAddress;     //Destination MAC Address (4 bits)
    uint8_t numberSDUs;             //Number of MAC SDUs (8 bits)
    uint16_t *sizes;                //Sizes of each MAC SDU (15 bits each)
    uint8_t *flagsDataControl;      //Data(1)/Control(0) flag of each SDU (1 bit each)
    size_t PDUsize;                 //PDU length
    unsigned short crc;             //Cyclic Redundancy Check (16 bits)
    bool verbose;                   //Verbosity flag

public:
    char* buffer;   //PDU buffer

    /**
     * @brief Constructs ProtocolPackage with all information need to encode header and no verbosity
     * @param sourceAddress Source MAC Address
     * @param destinationAddress Destination MAC Address
     * @param _numberSDUs Number of SDUs
     * @param _sizes Array of sizes of each SDU
     * @param _flagsDataControl Array of D/C flags of each SDU
     * @param _buffer Buffer containing multiplexed SDUs
     */
    ProtocolPackage(uint8_t sourceAddress, uint8_t destinationAddress, uint8_t _numberSDUs, uint16_t* _sizes, uint8_t* _flagsDataControl, char* _buffer);
    
    /**
     * @brief Constructs ProtocolPackage with all information need to encode header
     * @param sourceAddress Source MAC Address
     * @param destinationAddress Destination MAC Address
     * @param _numberSDUs Number of SDUs
     * @param _sizes Array of sizes of each SDU
     * @param _flagsDataControl Array of D/C flags of each SDU
     * @param _buffer Buffer containing multiplexed SDUs
     * @param _verbosity Verbosity flag
     */
    ProtocolPackage(uint8_t sourceAddress, uint8_t destinationAddress, uint8_t _numberSDUs, uint16_t* _sizes, uint8_t* _flagsDataControl, char* _buffer, bool _verbose);
    
    /**
     * @brief Constructs ProtocolPackage with an incoming PDU to be decoded and no verbosity
     * @param pdu Buffer containing full PDU
     * @param _size Size of PDU in bytes
     */
    ProtocolPackage(char* pdu, size_t _size);
    
    /**
     * @brief Constructs ProtocolPackage with an incoming PDU to be decoded
     * @param pdu Buffer containing full PDU
     * @param _size Size of PDU in bytes
     * @param _verbose Verbosity flag
     */
    ProtocolPackage(char* pdu, size_t _size, bool _verbose);
    
    /**
     * @brief Destroys ProtocolPackage object
     */
    ~ProtocolPackage();
        
    /**
     * @brief Inserts MAC header based on class variables values
     */
    void insertMacHeader();
    
    /**
     * @brief Removes MAC header and input its information to class variables
     */
    void removeMacHeader();
    
    /**
     * @brief Gets a TransmissionQueue object that will be used to unstack SDUs in decoding
     * @returns TransmissionQueue to perform demultiplexing
     */
    TransmissionQueue* getMultiplexedSDUs();
    
    /**
     * @brief Gets total PDU size
     * @returns PDU size in bytes
     */
    ssize_t getPduSize();
    
    /**
     * @brief Gets destination MAC Address of the ProtocolPackage
     * @returns Destination MAC address
     */
    uint8_t getDstMac();

    /**
     * @brief Gets source MAC Address of the ProtocolPackage
     * @returns Source MAC address
     */
    uint8_t getSrcMac();
};
#endif  //INCLUDED_PROTOCOL_PACKAGE_H