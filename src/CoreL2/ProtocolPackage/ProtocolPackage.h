#pragma once
#include <stdint.h> //uint8_t
#include <iostream> //cout
using namespace std;

#include "../Multiplexer/TransmissionQueue.h"
class TransmissionQueue;

class ProtocolPackage{
private:
    uint8_t srcAddr;    //Source MAC Address (4 bits)
    uint8_t dstAddr;    //Destination MAC Address (4 bits)
    uint8_t num;        //Number of MAC SDUs (8 bits)
    uint16_t *sizes;    //Sizes of each MAC SDU (15 bits each)
    uint8_t *dcs;       //Data(1)/Control(0) flag of each SDU (1 bit each)
    size_t size;        //PDU length
    unsigned short crc; //Cyclic Redundance Check (16 bits)
    unsigned short auxCalcCRC(char data, unsigned short _crc);
    bool verbose;
public:
    ProtocolPackage(uint8_t sa, uint8_t da, uint8_t n, uint16_t* _sizes, uint8_t* _dcs, char* buf);
    ProtocolPackage(uint8_t sa, uint8_t da, uint8_t n, uint16_t* _sizes, uint8_t* _dcs, char* buf, bool v);
    ProtocolPackage(char* pdu, size_t _size);
    ProtocolPackage(char* pdu, size_t _size, bool v);
    ~ProtocolPackage();
    void insertMacHeader();
    void removeMacHeader();
    TransmissionQueue* getMultiplexedSDUs();
    ssize_t getPduSize();
    uint8_t getDstMac();
    char* buffer;       //PDU buffer
};
