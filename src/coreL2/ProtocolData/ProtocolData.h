/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_PROTOCOL_DATA_H
#define INCLUDED_PROTOCOL_DATA_H

#include <iostream>
#include <mutex>
#include <condition_variable>

#include "../SduBuffers/SduBuffers.h"
#include "../Multiplexer/Multiplexer.h"
#include "../MacController/MacController.h"

class MacController;	//Initializing class that will be defined in other .h file

/**
 * @brief Class to manage including MACD SDUs in Multiplexer
 */
class ProtocolData{
private:
    MacHighQueue* macHigh;          //Queue to receive and enqueue L3 packets
    MacController* macController;   //MAC Controller that instantiated this class and has all shared variables
    bool verbose;                   //Verbosity flag

public:
    /**
     * @brief Constructs a new ProtocolData object
     * @param _macController MacController object which has all information of mutexes, condition variables and main multiplexer
     * @param _macHigh Queue with MACD SDUs to transmit
     * @param _verbose Verbosity flag
     */
    ProtocolData(MacController* _macController, MacHighQueue* _macHigh, bool _verbose);

    /**
     * @brief Destroys ProtocolData object
     */
    ~ProtocolData();
    
    /**
     * @brief Procedure that executes forever and controls TUN interface reading, adding SDUs from MAC High Queue to Multiplexer
     * @param currentMacMode Actual MAC Mode to control enqueueing while system is in another modes, e.g. RECONFIG_MODE or STOP_MODE
     * @param currentMacTxMode Actual MAC Tx Mode to signal to system if it is in an active mode, e.g. DISABLED_MODE_TX
     */
    void enqueueDataSdus(MacModes & currentMacMode, MacTxModes & currentMacTxMode);

    /**
     * @brief Receives and treat Data SDUs on decoding
     * @param buffer Buffer containing Data SDU
     * @param numberDecodingBytes Size of Data SDU in Bytes
     */
    void decodeDataSdus(char* buffer, size_t numberDecodingBytes);
};
#endif
