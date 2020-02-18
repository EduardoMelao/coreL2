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

#include "../LinkAdaptation/LinkAdaptation.h"
#include "../MacController/MacController.h"
#include "../AdaptiveModulationCoding/AdaptiveModulationCoding.h"
#include "../Cosora/Cosora.h"

/**
 * @brief Class to manage including MACC SDUs in Multiplexer and interlayer messages.
 */
class ProtocolControl{
private:
    MacController* macController;           //MacController object with all system modules
    RxMetrics* rxMetrics;                   //Object to handle RxMetrics collected in UE and sent to BS
    bool verbose;                           //Verbosity flag

public:
    /**
     * @brief Constructs a new ProtocolControl object
     * @param _macController MacController object with all system modules
     * @param _verbose Verbosity flag
     */
    ProtocolControl(MacController* _macController,  bool _verbose);

    /**
     * @brief Destroys ProtocolControl object
     */
    ~ProtocolControl();

    /**
     * @brief Receives and treat Control SDUs on decoding
     * @param currentMacMode Current MAC execution mode
     * @param buffer Buffer containing Control SDU
     * @param numberDecodingBytes Size of Control SDU in Bytes
     * @param macAddress Source MAC Address
     */
    void decodeControlSdus(MacModes & currentMacMode, char* buffer, size_t numberDecodingBytes, uint8_t macAddress);

    /**
     * @brief Perform transmission of Interlayer Control Messages to PHY
     * @param buffer Buffer containing the message bytes
     * @param numberBytes Size of message in Bytes
     */
    void sendInterlayerMessages(char* buffer, size_t numberBytes);

    /**
     * @brief Perform reception of Interlayer Control Messages from PHY and decides what to do
     * @param currentMacMode Actual MAC Mode to control enqueueing while system is in another modes, e.g. RECONFIG_MODE or STOP_MODE
     * @param currentMacRxMode Actual MAC Rx Mode to signal to system if it is in an active mode, e.g. ACTIVE_MODE_RX
     */
    void receiveInterlayerMessages(MacModes & currentMacMode, MacRxModes & currentMacRxMode);

    /**
     * @brief [UE] Receives bytes referring to Dynamic Parameters coming by MACC SDU and updates class with new information
     * @param currentMacMode Current MAC execution Mode
     * @param bytesDynamicParameters Serialized bytes from CLIL2Interface object
     * @param size Number of bytes of serialized information
     */ 
    void managerDynamicParameters(MacModes & currentMacMode, uint8_t* bytesDynamicParameters, size_t numberBytes);

    /**
     * @brief (Only UE) Periodically sends RxMetrics Report to BS
     */
    void rxMetricsReport();
};


#endif  //INCLUDED_PROTOCOL_CONTROL_H
