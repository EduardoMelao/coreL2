/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_TRANSMISSION_PROTOCOL_H
#define INCLUDED_TRANSMISSION_PROTOCOL_H

#include <iostream>
#include "../L1L2Interface/L1L2Interface.h"
#include "../CoreTunInterface/TunInterface.h"
#include "../../common/lib5grange/lib5grange.h"

using namespace std;

/**
 * @brief Class responsible for sending data to PHY layer and to Linux IP layer
 */
class TransmissionProtocol{
private:
    L1L2Interface* l1l2Interface;   //Object to send packets to L1
    TunInterface* tunInterface;     //Object to send packets to L3
    bool verbose;                   //Verbosity flag

public:
    /**
     * @brief Creates a new TransmissionProtocol object with desired verbosity
     * @param _l1l2Interface Object to send packets to L1
     * @param _tunInterface Object to send packets to L3
     * @param _verbose Verbosity flag
     */
    TransmissionProtocol(L1L2Interface* _l1l2Interface, TunInterface* _tunInterface, bool _verbose);

    /**
     * @brief Destructs TransmissionProtocol object
     */
    ~TransmissionProtocol();

    /**
     * @param macPdu MAC PDU structure containing all information PHY needs
     * @param macAddress Destination MAC Address
     */
    void sendPackageToL1(MacPDU macPdu, uint8_t macAddress);
    
    /**
     * @param controlBuffer Buffer with Control message
     * @param controlSize Control information size in bytes
     */
    void sendControlMessageToL1(char* controlBuffer, size_t controlSize);

    /**
     * @brief Receives packet from Linux IP Layer
     * @param buffer Buffer where packet will be stored
     * @param size Size of information in Bytes
     * @returns True if transmission was successful, false otherwise
     */
    bool sendPackageToL3(char* buffer, size_t size);

};
#endif  //INCLUDED_TRANSMISSION_PROTOCOL_H