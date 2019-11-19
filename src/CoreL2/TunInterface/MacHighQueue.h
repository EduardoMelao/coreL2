/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#pragma once
#include <vector>
#include <mutex>
#include "TunInterface.h"

#define MAXLINE 2048    //Maximum buffer size
#define DST_OFFSET 16

using namespace std;

/**
 * @brief Queue to store packets received from Linux IP layer
 */
class MacHighQueue{
private:
    TunInterface* tunInterface;     //Tun Interface object
    vector<char*> queue;            //Vector of L3 packets
    vector<ssize_t> sizes;          //Vector containing size of each packet
    mutex tunMutex;                 //Mutex to control access to queue
    bool verbose;                   //Verbosity flag
public:
    
    /**
     * @brief Constructs an empty MacHighQueue with a TUN descriptor
     * @param tun TUN Interface object
     * @param v Verbosity flag 
     */
    MacHighQueue(TunInterface* tun, bool _verbose);
    
    /**
     * @brief Destroys MacHighQueue
     */
    ~MacHighQueue();
    
    /**
     * @brief Proceeding that executes forever, receiving packets from L3 and storing them in the queue
     */
    void reading();
    
    /**
     * @brief Gets number of packets that are currently enqueued
     * @returns Number of packets enqueued
     */
    int getNumberPackets();
    
    /**
     * @brief Gets next SDU on queue for treatment
     * @param buffer Buffer were SDU will be stored
     * @returns Size of SDU
     */
    ssize_t getNextSdu(char* buffer);
};