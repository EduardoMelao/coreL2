/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_PROTOCOL_CONTROL_H
#define INCLUDED_PROTOCOL_CONTROL_H

#include <iostream>
#include <mutex>
#include <condition_variable>

#include "../Multiplexer/Multiplexer.h"
#include "../MacController/MacController.h"
#include "../LinkAdaptation/LinkAdaptation.h"
#include "../AdaptiveModulationCoding/AdaptiveModulationCoding.h"
#include "../Cosora/Cosora.h"

class MacController;	//Initializing class that will be defined in other .h file

/**
 * @brief Class to manage including MACc SDUs in Multiplexer
 */
class ProtocolControl{
private:
    MacController* macController;   //MAC Controller that instantiated this object and has all shared variables
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
     * @param controlSdu MAC Control SDU Bytes
     * @param numberBytes Size of MACC SDU in Bytes
     * @param macAddress Destination MAC Address
     */
    void enqueueControlSdus(uint8_t* controlSdu, size_t numberBytes, uint8_t macAddress);

    /**
     * @brief Receives and treat Control SDUs on decoding
     * @param buffer Buffer containing Control SDU
     * @param numberDecodingBytes Size of Control SDU in Bytes
     * @param macAddress Source MAC Address
     */
    void decodeControlSdus(char* buffer, size_t numberDecodingBytes, uint8_t macAddress);

    /**
     * @brief Perform transmission of Interlayer Control Messages to PHY
     * @param buffer Buffer containing the message bytes
     * @param numberBytes Size of message in Bytes
     */
    void sendInterlayerMessages(char* buffer, size_t numberBytes);

    /**
     * @brief Perform reception of Interlayer Control Messages from PHY and decides what to do
     * @param currentMacMode Actual MAC Mode to control enqueueing while system is in another modes, e.g. RECONFIG_MODE or STOP_MODE
     */
    void receiveInterlayerMessages(atomic<MacModes> & curentMacMode);
};


#endif  //INCLUDED_PROTOCOL_CONTROL_H
