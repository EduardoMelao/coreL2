/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef MAC_HIGH_QUEUE_H
#define MAC_HIGH_QUEUE_H

#include <vector>
#include <mutex>
#include "../ReceptionProtocol/ReceptionProtocol.h"
#include "../../common/libMac5gRange/libMac5gRange.h"

#define MAXIMUM_BUFFER_LENGTH 2048    //Maximum buffer size
#define DST_OFFSET 16

using namespace std;

/**
 * @brief Queue to store packets received from Linux IP layer
 */
class MacHighQueue{
private:
    ReceptionProtocol* reception;   //Object to receive packets from L3
    vector<char*> queue;            //Vector of L3 packets
    vector<ssize_t> sizes;          //Vector containing size of each packet
    mutex tunMutex;                 //Mutex to control access to queue
    bool verbose;                   //Verbosity flag

public:
    /**
     * @brief Constructs an empty MacHighQueue with a TUN descriptor
     * @param _reception Object to receive packets from L3
     * @param _verbose Verbosity flag 
     */
    MacHighQueue(ReceptionProtocol* _reception, bool _verbose);
    
    /**
     * @brief Destroys MacHighQueue
     */
    ~MacHighQueue();
    
    /**
     * @brief Procedure that executes forever, receiving packets from L3 and storing them in the queue
     * @param currentMacMode Actual MAC Mode to control enqueueing while system is in another modes, e.g. RECONFIG_MODE or STOP_MODE
     * @param currentMacTunMode Actual MAC Tun Mode to signal to system if it is in an active mode, e.g. TUN_DISABLED
     */
    void reading(MacModes & currentMacMode, MacTunModes & currentMacTunMode);
    
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
#endif  //MAC_HIGH_QUEUE_H