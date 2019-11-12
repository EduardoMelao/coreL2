/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#pragma once
#include <iostream> //std::cout
#include <future>   //std::async, std::future
#include <chrono>   //std::chrono::milisseconds
#include <mutex>    //std::mutex
#include <condition_variable>   //std::condition_variable

#include "../../CoreL1/CoreL1.h"
#include "../TunInterface/MacHighQueue.h"
#include "../ProtocolPackage/ProtocolPackage.h"
#include "../ProtocolPackage/MacAddressTable/MacAddressTable.h"
#include "../Multiplexer/TransmissionQueue.h"
#include "../Multiplexer/Multiplexer.h"
#include "../ProtocolControl/MacCQueue.h"
#include "MacCtHeader.h"

using namespace std;

#define MAXSDUS 20      //Maximum number of SDUs that can be enqueued for transmission
#define MAXLINE 2048    //Maximum buffer length in bytes
#define SRC_OFFSET 12   //IP packet source address offset in bytes 
#define DST_OFFSET 16   //IP packet destination address offset in bytes
#define TIMEOUT 10      //Timeout to send PDU if there is information to transmit

/**
 * @brief Class responsible for managing all MAC 5G-RANGE operations
 */
class MacController{
private:
    int attachedEquipments;     //Number of equipments attached. It must be 1 for UEs.
    uint16_t nB;                //Maximum number of Bytes of MAC 5G-RANGE PDU
    uint8_t macAddr;            //MAC Address of equipment
    TunInterface* tunIf;        //TunInterface object to perform L3 packet capture
    MacHighQueue* macHigh;      //Queue to receive and enqueue L3 packets
    MacAddressTable* arp;       //Table to associate IP addresses to 5G-RANGE domain MAC addresses
    condition_variable queueCv; //Condition variable to manage access to Multiplexer Queue
    mutex queueMutex;           //Mutex to control access to Transmission Queue
    Multiplexer* mux;           //Multiplexes various SDUs to multiple destinations
    MacCQueue* macc;            //Queue to store Control PDUs
    CoreL1* l1;                 //CoreL1 object that performs sending and receiving operations in PHY level
    thread *threads;            //Threads array
    bool bs;                    //BaseStation flag: 1 for BS; 0 for UE
    bool verbose;               //Verbosity flag
    unsigned short auxCalcCRC(char data, unsigned short crc);
public:
    MacController(int nEquipments, uint16_t _nB, const char* _devNameTun, MacAddressTable* _arp, uint8_t _macAddr, CoreL1* _l1, bool v);
    ~MacController();
    void readTunCtl();
    void controlSduCtl();
    void startThreads();
    void sendPdu(int index);
    void timeoutController();
    void decoding(uint16_t port);
    void crcPackageCalculate(char* buf, int size);
    bool crcPackageChecking(char* buf, int size);
};
