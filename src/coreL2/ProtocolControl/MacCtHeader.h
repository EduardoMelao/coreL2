/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_MAC_CONTROL_HEADER_H
#define INCLUDED_MAC_CONTROL_HEADER_H

#include <stdint.h> //uint8_t
#include <iostream>
#include <string.h>
#define CONTROLBYTES2BS 1 
#define CONTROLBYTES2UE 5

using namespace std;

/**
 * @brief PDU Control Header definition
 */
class MacCtHeader{
private:
    uint8_t flagBS;                 //Flag to identify if this equipment is BS or UE
    uint8_t id;                     //BS->UE UE->BS 8 bits
    uint8_t uplinkMCS;              //BS->UE        8 bits
    uint8_t rbStart;                //BS->UE        8 bits
    uint8_t numberRBs;              //BS->UE        8 bits
    uint8_t MIMOon;                 //BS->UE        1 bit
    uint8_t MIMOdiversity;          //BS->UE        1 bit
    uint8_t MIMOantenna;            //BS->UE        1 bit
    uint8_t MIMOopenLoopClosedLoop; //BS->UE        1 bit
    bool verbose;                   //Verbosity flag

public:
    /**
     * @brief Builds a Control Header from static information to be used on encoding
     * @param _flagBS BS flag: true for BS; false for UE
     * @param _verbose Verbosity flag
     */
    MacCtHeader(bool _flagBS, bool _verbose);

    /**
     * @brief Gets the Control Header to be shared with PHY in PDU's encoding process
     * @param buffer Buffer to store control information
     * @returns Control data size in bytes
     */
    ssize_t getControlData(char* buffer);
};
#endif  //INCLUDED_MAC_CONTROL_HEADER_H