/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_MAC_CONFIG_REQUEST_H
#define INCLUDED_MAC_CONFIG_REQUEST_H

#include <iostream>
#include <fstream>      //File stream
#include <string.h>
#include <vector>
#include <cstdlib> 		//atoi
using namespace std;

#include "../../common/lib5grange/lib5grange.h"
using namespace lib5grange;

/**
 * @brief Structure to receive and persist information from MAC 5G RANGE CLI
 */
class MacConfigRequest{
private:
    fstream configurationFile;                      //File where configurations are persisted
    int numberUEs;                                  //Number of UEs configured
    vector<uint8_t> tpcs;                           //Transmission Power Control
    vector<allocation_cfg_t> uplinkReservations;    //Array of Uplink reservation configuration structures
    bool verbose;                                   //Verbosity flag

public:
    bool flagModified;  //Flag to indicate if configurations have been changed

    /**
     * @brief Constructs a Mac Config Request object and initializes its variables with values persisted
     * @param _verbose Verbosity flag
     */
    MacConfigRequest(bool _verbose);

    /**
     * @brief Destroys Mac Config Request object and deallocates its arrays
     */
    ~MacConfigRequest();

    /**
     * @brief Adds a new UE configuration and persists it to file 
     * @param ueId User equipment ID (1 Byte)
     * @param tpc Transmission Power Control for this UE
     * @param rbStart Resource block start information
     * @param rbEnd Resource block end information
     */
    void setPhyParameters(uint8_t ueId, uint8_t tpc, uint8_t rbStart, uint8_t rbEnd);

    /**
     * @brief Gets all Dynamic Configurations of UL Reservation  
     * @param buffer Buffer where Configuration Bytes will be read
     */
    void getULReservation(vector<uint8_t> & buffer);

    /**
     * @brief Decodes ULReservation to class variables
     * @param buffer Buffer containing coded Bytes
     */
    void decodeULReservation(vector<uint8_t> buffer);
};

#endif  //INCLUDED_MAC_CONFIG_REQUEST_H
