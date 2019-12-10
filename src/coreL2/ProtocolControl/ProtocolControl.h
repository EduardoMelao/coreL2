/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_PROTOCOL_CONTROL_H
#define INCLUDED_PROTOCOL_CONTROL_H

#include <iostream>
#include <mutex>
#include <condition_variable>

#include "MacCQueue.h"

#include "../Multiplexer/Multiplexer.h"
#include "../MacController/MacController.h"

class MacController;	//Initializing class that will be defined in other .h file

/**
 * @brief Class to manage including MACc SDUs in Multiplexer
 */
class ProtocolControl{
private:
    MacController* macController;   //MAC Controller that instantiated this object and has all shared variables
    MacCQueue* macControlqueue;     //Queue that generates Control SDUs for transmission
    bool verbose;                   //Verbosity flag

public:
    /**
     * @brief Constructs a new ProtocolControl object
     * @param _macController MacController object which has all information of mutexes, condition variables and main multiplexer
     * @param _verbose Verbosity flag
     */
    ProtocolControl(MacController* _macController, bool _verbose);

    /**
     * @brief Destroys ProtocolControl object
     */
    ~ProtocolControl();
    
    /**
     * @brief Procedure that executes forever and controls MACC SDUs generation, adding SDUs from MAC Control Queue to Multiplexer
     */
    void enqueueControlSdus();

    /**
     * @brief Receives and treat Control SDUs on decoding
     * @param buffer Buffer containing Control SDU
     * @param numberDecodingBytes Size of Control SDU in Bytes
     */
    void decodeControlSdus(char* buffer, size_t numberDecodingBytes);

    /**
     * @brief Perform transmission of Interlayer Control Messages to PHY
     * @param buffer Buffer containing the message bytes
     * @param numberBytes Size of message in Bytes
     */
    void sendInterlayerMessages(char* buffer, size_t numberBytes);

    /**
     * @brief Perform reception of Interlayer Control Messages from PHY and decides what to do
     */
    void receiveInterlayerMessages();
};


#endif  //INCLUDED_PROTOCOL_CONTROL_H