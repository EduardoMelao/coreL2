/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#pragma once
#include <vector>
#include <mutex>
#include "TunInterface.h"

#define MAXLINE 2048
#define SRC_OFFSET 12
#define DST_OFFSET 16

using namespace std;

/**
 * @brief Queue to store packets received from Linux IP layer
 */
class MacHighQueue{
private:
    TunInterface* tunIf;    //Tun Interface object
    vector<char*> queue;    //Vector of L3 packets
    vector<ssize_t> sizes;  //Vector containing size of each packet
    mutex tunMutex;         //Mutex to control access to queue
    bool verbose;           //Verbosity flag
public:
    MacHighQueue(TunInterface* tun, bool v);
    ~MacHighQueue();
    void reading();
    int getNum();
    ssize_t getNextSdu(char* buf);
};