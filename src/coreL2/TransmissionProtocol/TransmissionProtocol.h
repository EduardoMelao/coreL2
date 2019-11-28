/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_TRANSMISSION_PROTOCOL_H
#define INCLUDED_TRANSMISSION_PROTOCOL_H

#include <iostream>
#include "../L1L2Interface/L1L2Interface.h"
#include "../CoreTunInterface/TunInterface.h"

using namespace std;

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
     * @param dataBuffer Information buffer
     * @param dataSize Size of information in bytes
     * @param controlBuffer Buffer with Control information
     * @param controlSize Control information size in bytes
     * @param port Socket port to identify which socket to send information
     * @returns True if transmission was successful, false otherwise
     */
    bool sendPackageToL1(char* dataBuffer, size_t dataSize, char* controlBuffer, size_t controlSize, int port);

    /**
     * @brief Receives packet from Linux IP Layer
     * @param buffer Buffer where packet will be stored
     * @param size Size of information in Bytes
     * @returns True if transmission was successful, false otherwise
     */
    bool sendPackageToL3(char* buffer, size_t size);

};
#endif  //INCLUDED_TRANSMISSION_PROTOCOL_H