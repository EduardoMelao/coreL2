/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#pragma once
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
    uint8_t flagBS;     //Flag to identify if this equipment is BS or UE
    uint8_t id;         //BS->UE UE->BS 8 bits
    uint8_t ulMCS;      //BS->UE        8 bits
    uint8_t rbStart;    //BS->UE        8 bits
    uint8_t numRBs;     //BS->UE        8 bits
    uint8_t MIMOon;     //BS->UE        1 bit
    uint8_t MIMOdiv;    //BS->UE        1 bit
    uint8_t MIMOantenna;//BS->UE        1 bit
    uint8_t MIMOolcl;   //BS->UE        1 bit
    bool verbose;       //Verbosity flag

public:
    /**
     * @brief Builds a Control Header from static information to be used on encoding
     * @param _flagBS BS flag: true for BS; false for UE
     * @param _verbose Verbosity flag
     */
    MacCtHeader(bool _flagBS, bool _verbose);
    

    /**
     * @brief Builds a Control Header from a PDU received on decoding and makes possible the use of these informations
     * @param _flagBS BS flag: true for BS; false for UE
     * @param buffer Buffer containing PDU
     * @param size Size in bytes of PDU
     * @param _verbose Verbosity flag
     */ 
    MacCtHeader(bool _flagBS, char* buffer, int size, bool _verbose);

    /**
     * @brief Inserts a Control Header in PDU's encoding process
     * @param buffer Buf containing PDU to be encoded
     * @param size Size of PDU in bytes
     * @returns New PDU size after Header insertion
     */
    ssize_t insertControlHeader(char* buffer, int size);

    /**
     * @brief Removes a Control Header in PDU's decoding process
     * @param buffer Buf containing PDU to be decoded
     * @param size Size of PDU in bytes
     * @returns Size of decoded PDU
     */
    ssize_t removeControlHeader(char* buffer, int size);
};
