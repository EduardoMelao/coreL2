/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_MAC_CONTROLLER_H
#define INCLUDED_MAC_CONTROLLER_H

#include <iostream> //std::cout
#include <future>   //std::async, std::future
#include <chrono>   //std::chrono::milliseconds
#include <mutex>    //std::mutex
#include <condition_variable>   //std::condition_variable

#include "../../coreL1/CoreL1.h"
#include "../ProtocolPackage/MacHighQueue.h"
#include "../ProtocolPackage/ProtocolPackage.h"
#include "../Multiplexer/MacAddressTable/MacAddressTable.h"
#include "../Multiplexer/TransmissionQueue.h"
#include "../Multiplexer/Multiplexer.h"
#include "../ProtocolControl/MacCQueue.h"
#include "../L1L2Interface/L1L2Interface.h"
#include "../ProtocolControl/MacCtHeader.h"

using namespace std;

#define MAXSDUS 20      //Maximum number of SDUs that can be enqueued for transmission
#define MAXLINE 2048    //Maximum buffer length in bytes
#define SRC_OFFSET 12   //IP packet source address offset in bytes 
#define DST_OFFSET 16   //IP packet destination address offset in bytes
#define TIMEOUT 1      	//Timeout(nanoseconds) to send PDU if there is information to transmit

/**
 * @brief Class responsible for managing all MAC 5G-RANGE operations
 */
class MacController{
private:
    int attachedEquipments;         //Number of equipments attached. It must be 1 for UEs.
    uint8_t* macAddressEquipents;   //Attached equipments 5GR MAC Address
    uint16_t maxNumberBytes;        //Maximum number of Bytes of MAC 5G-RANGE PDU
    uint8_t macAddress;             //MAC Address of equipment
    TunInterface* tunInterface;     //TunInterface object to perform L3 packet capture
    MacHighQueue* macHigh;          //Queue to receive and enqueue L3 packets
    MacAddressTable* ipMacTable;    //Table to associate IP addresses to 5G-RANGE domain MAC addresses
    condition_variable* queueConditionVariables;  //Condition variables to manage access to Multiplexer Queues
    mutex queueMutex;               //Mutex to control access to Transmission Queue
	Multiplexer* mux;               //Multiplexes various SDUs to multiple destinations
	MacCQueue* macControlQueue;     //Queue to store Control PDUs
	L1L2Interface* l1l2Interface;   //Deals with communication between MAC and PHY
	thread *threads;                //Threads array
    bool flagBS;                    //BaseStation flag: 1 for BS; 0 for UE
    bool verbose;                   //Verbosity flag

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
     * @param _macAddressEquipments MAC Address of each attached equipment
     * @param _maxNumberBytes Maximum number of PDU in Bytes 
     * @param _deviceNameTun Customized name for TUN Interface
     * @param _ipMacTable Static table to link IP addresses to 5G-RANGE MAC addresses
     * @param _macAddress Current MAC address
     * @param _l1 Configured CoreL1 object
     * @param _verbose Verbosity flag
     */
    MacController(int numberEquipments, uint8_t* _macAddressessEquipments, uint16_t _maxNumberBytes, 
        const char* _deviceNameTun, MacAddressTable* _ipMacTable, uint8_t _macAddress, CoreL1* _l1, bool _verbose);
    
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
     * @brief Performs PDU sending to destination identified by macAddress
     * @param macAddress MAC Address of destination
     */
    void sendPdu(uint8_t macAddress);

    /**
     * @brief Procedure that controls timeout and triggers PDU sending
     * @param index Index of MAC Addresses of equipments and Condition Variables Arrays
     */
    void timeoutController(int index);

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
#endif  //INCLUDED_MAC_CONTROLLER_H
