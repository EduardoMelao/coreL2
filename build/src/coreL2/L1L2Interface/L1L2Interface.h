/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_L1_L2_INTERFACE_H
#define INCLUDED_L1_L2_INTERFACE_H

#include <iostream>
#include <mqueue.h>         //Posix Message Queues
#include <vector>
#include <string.h>         //bzero()
#include <unistd.h>         //close()
#include <errno.h>          //errno
#include <boost/crc.hpp>    //Boost CRC library
#include "../../common/lib5grange/lib5grange.h"
#include "../../common/libMac5gRange/libMac5gRange.h"

using namespace std;
using namespace lib5grange;

class L1L2Interface{
private:
    l1_l2_interface_t l1l2InterfaceQueues;      //Struct containing all queues to change messages with PHY  
    bool verbose;                               //Verbosity flag
    /**
    * @brief Auxiliary function for CRC calculation
    * @param data Single byte from PDU
    * @param crc CRC history
    * @returns 2-byte CRC calculation
    */
    unsigned short auxiliaryCalculationCRC(char data, unsigned short crc);

public:
    /**
     * @brief Constroys a L1L2Interface object, initializes class variables with static information and allocate sockets tocommunicate with PHY
     * @param _verbose verbosity flag
     */
    L1L2Interface(bool _verbose);

    /**
     * @brief Destroys a L1L2Interface object
     */
    ~L1L2Interface();

    /**
     * @param macPdus Array of MAC PDUs structures containing all information PHY needs
     */
    void sendPdus(vector<MacPDU> macPdus);

    /**
     * @brief Receives PDUs from PHY Layer
     * @param buffer Buffer where PDUs are going to be stored
     */
    void receivePdus(vector<MacPDU*> & buffer);

    /**
     * @brief Sends Control Message to PHY
     * @param buffer Buffer containing message
     * @param numberBytes Size of message in bytes
     */
    void sendControlMessage(char* buffer, size_t numberBytes);

    /**
     * @brief Received Control Message from PHY
     * @param buffer Buffer where message will be stored
     * @returns Size of message received in Bytes
     */
    ssize_t receiveControlMessage(char* buffer);

    /**
     * @brief Calculates CRC of current PDU passed as parameter
     * @param buffer Bytes of current PDU
     * @param size Size of PDU in bytes
     */
    void crcPackageCalculate(char* buffer, int size);

    /**
     * @brief Checks if CRC contained in received PDU matches calculated CRC
     * @param buffer Bytes of current PDU
     * @param size Size of PDU in bytes
     * @returns True if CRC match; False otherwise
     */
    bool crcPackageChecking(char* buffer, int size);
};
#endif  //INCLUDED_L1_L2_INTERFACE_H