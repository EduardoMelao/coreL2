#include "MacCQueue.h"

MacCQueue::MacCQueue(){ }

MacCQueue::~MacCQueue() { }

ssize_t MacCQueue::getAck(char* buf){
    const char ack[4] = "Ack";
    memcpy(buf, ack, 3);
    return 3;
}

ssize_t MacCQueue::getControlSduCSI(char* buf){
    ////////////////PROVISIONAL/////////////
    return getControlSduULMCS(buf);
}

ssize_t MacCQueue::getControlSduULMCS(char* buf){
    ////////////////PROVISIONAL/////////////
    const char macCSdu[22] = "Control PDU: each 10s";
    ssize_t size = 21;
    ////////////////////////////////////////
    this_thread::sleep_for(chrono::milliseconds(10000));
    memcpy(buf, macCSdu, size);
    return size;
}