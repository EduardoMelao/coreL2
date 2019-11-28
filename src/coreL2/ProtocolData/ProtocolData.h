/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_PROTOCOL_DATA_H
#define INCLUDED_PROTOCOL_DATA_H

#include <iostream>
#include <mutex>
#include <condition_variable>

#include "MacHighQueue.h"
#include "../Multiplexer/Multiplexer.h"
#include "../MacController/MacController.h"

class ProtocolData{
private:
    MacHighQueue* macHigh;          //Queue to receive and enqueue L3 packets
    MacController* macController;   //MAC Controller that instantiated this class and has all shared variables
public:
    ProtocolData(MacController* _macController, MacHighQueue* _macHigh);
    ~ProtocolData();
    
    /**
     * @brief Procedure that executes forever and controls TUN interface reading, adding SDUs from MAC High Queue to Multiplexer
     */
    void enqueueDataSdus();
};
#endif