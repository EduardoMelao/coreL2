/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_MULTIPLEXER_H
#define INCLUDED_MULTIPLEXER_H

#include <iostream>
#include <vector>
#include <string.h>	//memcpy

using namespace std;

/**
 * @brief Queue used to store SDUs that will be transmitted to a specific destination
 */
class Multiplexer{
private:
    uint8_t numberSDUs;                     //Number of SDUs multiplexed
    int currentPDUSize;                     //Size of PDU that is being aggregated
    int maxNumberBytes;                     //Maximum number of bytes
    char* sduBuffer;                        //Buffer accumulates SDUs
    uint8_t sourceAddress;                  //Source MAC address
    uint8_t destinationAddress;             //Destination MAC address
    int offset;                             //Offset for decoding
    int controlOffset;                      //Offset for encoding Control SDUs
    vector<uint16_t> sizesSDUs;             //Sizes of each SDU multiplexed
    vector<uint8_t> flagsDataControlSDUs;   //Data(1)/Control(0) flag
    bool verbose;                           //Verbosity flag
    
    /**
     * @brief Adds SDU to specific position of the multiplexing queue
     * @param sdu Buffer containing single SDU
     * @param size SDU size
     * @param flagDataControl SDU D/C flag
     * @param position Position where SDU will be added
     * @returns True if SDU was inserted successfully; False otherwise
     */
    bool addSduPosition(char* sdu, uint16_t size, uint8_t flagDataControl, int position);  
    
    /**
     * @brief Returns the current queue buffer length considering the size of each SDU
     * @returns Current Multiplexer buffer length in bytes
     */
    int currentBufferLength();     
public:

    /**
     * @brief Constructs a new Multiplexer to accumulate SDUs for future transmission (encoding)
     * @param maxNumberBytes Maximum number of bytes supported by a single PDU
     * @param sourceAddress Source MAC Address
     * @param destinationAddress Destination MAC Address
     * @param _verbose Verbosity flag
     */
    Multiplexer(int _maxNumberBytes, uint8_t sourceAddress, uint8_t destinationAddress, bool _verbose);
    
    /**
     * @brief Constructs Multiplexer with an incoming PDU to be decoded
     * @param pdu Buffer containing full PDU
     * @param _verbose Verbosity flag
     */
    Multiplexer(uint8_t* pdu, bool _verbose);
    
    /**
     * @brief Destroy a Multiplexer object
     */
    ~Multiplexer();

    /**
     * @brief Returns the total PDU length in bytes, considering extra overhead of 2 bytes (sourceAddress, destinationAddress, numSDUs)
     */
    int getNumberofBytes();
    
    /**
     * @brief Adds SDU to the multiplexing queue
     * @param sdu Buffer containing single SDU
     * @param size SDU size
     * @param flagDataControl SDU D/C flag
     * @returns True if SDU was inserted successfully; False otherwise
     */     
    bool addSDU(char* sdu, uint16_t size, uint8_t flagDataControl);

    /**
     * @brief Function used for getting SDUs while decoding
     * @param sdu Buffer to store SDU
     * @returns Size of next SDU multiplexed; Returns 0 for EOF
     */
    ssize_t getSDU(char* sdu);
    
    /**
     * @brief Creates a new ProtocolPackage object based on information stored in class variables on encoding process and returns serialized PDU
     * @param pduBuffer Buffer to store PDU with SDUs multiplexed
     */
    void getPDU(vector<uint8_t> & pduBuffer);

    /**
     * @brief Gets information of Data/Control flag of previous SDU to be used in decoding
     * @returns Data(1)/Control(0) flag for SDU immediately behind (it is supposed getSDU() has been used before) current SDU in decoding queue
     */
    uint8_t getCurrentDataControlFlag();

    /**
     * @brief Gets destination MAC address of Multiplexer object
     * @returns Destination MAC address;
     */
    uint8_t getDestinationAddress();

    /**
     * @brief Inserts MAC header based on class variables values
     */
    void insertMacHeader();
    
    /**
     * @brief Removes MAC header and input its information to class variables
     */
    void removeMacHeader();

    /**
     * @brief Tests if Multiplexed has no MAC SDUs aggregated
     * @returns TRUE if Multiplexer is empty; FALSE otherwise
     */
    bool isEmpty();
};
#endif  //INCLUDED__MULTIPLEXER_H
