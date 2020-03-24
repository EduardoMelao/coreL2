/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_RECEPTION_PROTOCOL_H
#define INCLUDED_RECEPTION_PROTOCOL_H

#include <iostream>
#include "../L1L2Interface/L1L2Interface.h"
#include "../CoreTunInterface/TunInterface.h"

using namespace std;

/**
 * @brief Class responsible for receiving data from PHY layer and from Linux IP layer
 */
class ReceptionProtocol{
private:
    L1L2Interface* l1l2Interface;   //Object to receive packets from L1
    TunInterface* tunInterface;     //Object to receive packets from L3
    bool verbose;                   //Verbosity flag

public:
    /**
     * @brief Creates a new ReceptionProtocol object with desired verbosity
     * @param _l1l2Interface Object to receive packets from L1
     * @param _tunInterface Object to receive packets from L3
     * @param _verbose Verbosity flag
     */
    ReceptionProtocol(L1L2Interface* _l1l2Interface, TunInterface* _tunInterface, bool _verbose);

    /**
     * @brief Destructs ReceptionProtocol object
     */
    ~ReceptionProtocol();

    /**
     * @brief Receives packet from PHY Layer
     * @param buffer Buffer where packet will be stored
     * @param maximumSize Maximum size of buffer in Bytes
     */
    void receivePackageFromL1(vector<vector<uint8_t>> & buffer, int maximumSize);

    /**
     * @brief Receives packet from Linux IP Layer
     * @param buffer Buffer where packet will be stored
     * @param maximumSize Maximum size of buffer in Bytes
     * @returns Size of information received in Bytes; 0 for EOF; -1 for errors
     */
    ssize_t receivePackageFromL3(char* buffer, int maximumSize);

    /**
     * @brief Performs select() procedure
     * @returns FALSE for timeout; TRUE if information is ready
     */
    bool isL3Ready();

};
#endif  //INCLUDED_RECEPTION_PROTOCOL_H
