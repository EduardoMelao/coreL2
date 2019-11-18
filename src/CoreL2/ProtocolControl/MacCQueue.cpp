/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "MacCQueue.h"

MacCQueue::MacCQueue(){ }

MacCQueue::~MacCQueue() { }

ssize_t 
MacCQueue::getAck(
    char* buffer)   //Buffer where message will be stored
{
    const char ack[4] = "Ack";
    memcpy(buffer, ack, 3);
    return 3;
}

ssize_t 
MacCQueue::getControlSduCSI(
    char* buffer)   //Buffer where message will be stored
{
    ////////////////PROVISIONAL/////////////
    return getControlSduULMCS(buffer);
}

ssize_t 
MacCQueue::getControlSduULMCS(
    char* buffer)   //Buffer where message will be stored
{
    ////////////////PROVISIONAL/////////////
    const char macCSdu[22] = "Control PDU: each 10s";
    ssize_t size = 21;
    ////////////////////////////////////////
    this_thread::sleep_for(chrono::milliseconds(10000));
    memcpy(buffer, macCSdu, size);
    return size;
}