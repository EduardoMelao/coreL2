/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "MacCQueue.h"

/**
 * @brief Constructs an empty Control Queue
 */
MacCQueue::MacCQueue(){ }

/**
 * @brief Destroys control queue
 */
MacCQueue::~MacCQueue() { }

/**
 * @brief Gets ACK control message that will be used for UE->BS acknowledgement
 * @param buf Buffer that will store the message
 * @returns Size of the message in bytes
 */
ssize_t 
MacCQueue::getAck(char* buf){
    const char ack[4] = "Ack";
    memcpy(buf, ack, 3);
    return 3;
}

/**
 * @brief Gets CSI Control SDU
 * @param buf Buffer that will store the message
 * @returns Size of the message in bytes
 */
ssize_t 
MacCQueue::getControlSduCSI(char* buf){
    ////////////////PROVISIONAL/////////////
    return getControlSduULMCS(buf);
}

/**
 * @brief Gets ULMCS Control SDU
 * @param buf Buffer that will store the message
 * @returns Size of the message in bytes
 */
ssize_t 
MacCQueue::getControlSduULMCS(char* buf){
    ////////////////PROVISIONAL/////////////
    const char macCSdu[22] = "Control PDU: each 10s";
    ssize_t size = 21;
    ////////////////////////////////////////
    this_thread::sleep_for(chrono::milliseconds(10000));
    memcpy(buf, macCSdu, size);
    return size;
}