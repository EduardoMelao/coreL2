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

#define MAXSDUS 20
#define MAXLINE 2048
#define SRC_OFFSET 12
#define DST_OFFSET 16

class MacController{
private:
    int attachedEquipments;
    uint16_t nB;
    uint8_t macAddr;
    TunInterface* tunIf;
    MacHighQueue* macHigh;
    bool verbose;
    MacAddressTable* arp;
    condition_variable queueCv;
    mutex queueMutex;
    Multiplexer* mux;
    MacCQueue* macc;
    CoreL1* l1;
    thread *threads;
    bool bs;
    unsigned short auxCalcCRC(char data, unsigned short crc);
public:
    MacController(int nEquipments, uint16_t _nB, const char* _devNameTun, MacAddressTable* _arp, uint8_t _macAddr, CoreL1* _l1, bool v);
    ~MacController();
    void readTunCtl();
    void controlSduCtl();
    void startThreads();
    void sendPdu(int index);
    void encoding();
    void decoding(uint16_t port);
    void crcPackageCalculate(char* buf, int size);
    bool crcPackageChecking(char* buf, int size);
};
