/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#pragma once
#include <iostream> //std::cout
#include <future>   //std::async, std::future
#include <chrono>   //std::chrono::milliseconds
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
    uint16_t maxNumberBytes;    //Maximum number of Bytes of MAC 5G-RANGE PDU
    uint8_t macAddr;            //MAC Address of equipment
    TunInterface* tunInterface;        //TunInterface object to perform L3 packet capture
    MacHighQueue* macHigh;      //Queue to receive and enqueue L3 packets
    MacAddressTable* ipMacTable;       //Table to associate IP addresses to 5G-RANGE domain MAC addresses
    condition_variable queueConditionVariable;  //Condition variable to manage access to Multiplexer Queue
    mutex queueMutex;           //Mutex to control access to Transmission Queue
    Multiplexer* mux;           //Multiplexes various SDUs to multiple destinations
    MacCQueue* macControlQueue;            //Queue to store Control PDUs
    CoreL1* l1;                 //CoreL1 object that performs sending and receiving operations in PHY level
    thread *threads;            //Threads array
    bool flagBS;                //BaseStation flag: 1 for BS; 0 for UE
    bool verbose;               //Verbosity flag

    /**
     * @brief Auxiliary function for CRC calculation
     * @param data Single byte from PDU
     * @param crc CRC history
     * @returns 2-byte CRC calculation
     */
    unsigned short auxiliaryCalculationCRC(char data, unsigned short crc);
public:
    /**
     * @brief Initializes a MacController object to manage all 5G RANGE MAC Operations
     * @param numberEquipments Number of attached equipments. Must be 1 for UEs
     * @param _maxNumberBytes Maximum number of PDU in Bytes 
     * @param _deviceNameTun Customized name for TUN Interface
     * @param _ipMacTable Static table to link IP addresses to 5G-RANGE MAC addresses
     * @param _macAddr Current MAC address
     * @param _l1 Configured CoreL1 object
     * @param _verbose Verbosity flag
     */
    MacController(int numberEquipments, uint16_t _maxNumberBytes, const char* _deviceNameTun, MacAddressTable* _ipMacTable, uint8_t _macAddr, CoreL1* _l1, bool _verbose);
    
    /**
     * @brief Destructs MacController object
     */
    ~MacController();

    /**
     * @brief Procedure that executes forever and controls TUN interface reading, adding SDUs from MAC High Queue to Multiplexer
     */
    void readTunControl();

    /**
     * @brief Procedure that executes forever and controls Control SDUs entrance in Multiplexing queue
     */
    void controlSduControl();

    /**
     * @brief Declares and starts all threads necessary for MacController
     */
    void startThreads();

    /**
     * @brief Performs PDU sending to destination identified by index
     * @param index Index of destination Transmission Queue in Multiplexer
     */
    void sendPdu(int index);

    /**
     * @brief Procedure that controls timeout and triggers PDU sending
     */
    void timeoutController();

    /**
     * @brief Procedure that performs decoding of PDUs received from L1
     * @param port Receiving socket port
     */
    void decoding(uint16_t port);

    /**
     * @brief Calculates CRC of current PDU passed as parameter
     * @param buffer Bytes of current PDU
     * @param size Size of PDU in bytes
     */
    void crcPackageCalculate(char* buffer, int size);

    /**
     * @brief Checks if CRC contained in received PDU matches calculated CRC
     * @param buffer Bytes of current PDU
     * @param size Size of PDU in bytes
     * @returns True if CRC match; False otherwise
     */
    bool crcPackageChecking(char* buffer, int size);
};
