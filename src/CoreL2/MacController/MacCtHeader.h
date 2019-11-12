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
    uint8_t flagBS;     
    uint8_t id;         //BS->UE UE->BS 8 bits
    uint8_t ulMCS;      //BS->UE        8 bits
    uint8_t rbStart;    //BS->UE        8 bits
    uint8_t numRBs;     //BS->UE        8 bits
    uint8_t MIMOon;     //BS->UE        1 bit
    uint8_t MIMOdiv;    //BS->UE        1 bit
    uint8_t MIMOantenna;//BS->UE        1 bit
    uint8_t MIMOolcl;   //BS->UE        1 bit
    bool verbose;

public:
    MacCtHeader(bool bs, char* b, int size, bool v);
    MacCtHeader(bool bs, bool v);
    ssize_t insertControlHeader(char* buf, int size);
    ssize_t removeControlHeader(char* buf, int size);
};
