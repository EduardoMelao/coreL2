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
    char* buffer;           //Buffer accumulates SDUs
    uint8_t srcAddr;        //Source MAC address
    uint8_t dstAddr;        //Destination MAC address
    int offset;             //Offset for decoding
    int controlOffset;      //Offset for encoding Control SDUs
    uint16_t* sizesSDUs;    //Sizes of each SDU multiplexed
    uint8_t* dcsSDUs;       //Data(1)/Control(0) flag
    bool verbose;           //Verbosity flag
    bool addSduPosition(char* sdu, uint16_t size, uint8_t dc, int position);  
    int currentBufferLength();     
public:
    TransmissionQueue(int _nBytes, uint8_t src, uint8_t dst, int maxSDUs, bool v);
    TransmissionQueue(char* _buffer, uint8_t _nSDUs, uint16_t* _sizesSDUs, uint8_t* dcs_SDUs, bool v);
    ~TransmissionQueue();
    int nBytes;         //Maximum number of bytes
    uint8_t nSDUs;      //Number of SDUs multiplexed
    int maxNSDUs;   //Maximum number of SDUs multiplexed
    int getNumberofBytes();     
    bool addSDU(char* sdu, uint16_t size, uint8_t dc);
    ssize_t getSDU(char* buf);  
    ProtocolPackage* getPDUPackage();   
    void clearBuffer(); 
    uint8_t getCurrentDCFlag();
    uint8_t getDstAddr();
};
