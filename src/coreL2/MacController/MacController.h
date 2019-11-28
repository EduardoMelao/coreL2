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
#include "../ReceptionProtocol/ReceptionProtocol.h"
#include "../TransmissionProtocol/TransmissionProtocol.h"
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
    L1L2Interface* l1l2Interface;   //Object to manage interface with L1
    MacHighQueue* macHigh;          //Queue to receive and enqueue L3 packets
    MacAddressTable* ipMacTable;    //Table to associate IP addresses to 5G-RANGE domain MAC addresses
    condition_variable* queueConditionVariables;  //Condition variables to manage access to Multiplexer Queues
    mutex queueMutex;               //Mutex to control access to Transmission Queue
	Multiplexer* mux;               //Multiplexes various SDUs to multiple destinations
	MacCQueue* macControlQueue;     //Queue to store Control PDUs
	ReceptionProtocol* receptionProtocol;       //Object to receive packets from L1 and L3
    TransmissionProtocol* transmissionProtocol; //Object to transmit packets to L1 and L3
	thread *threads;                //Threads array
    bool flagBS;                    //BaseStation flag: 1 for BS; 0 for UE
    bool verbose;                   //Verbosity flag
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
};
#endif  //INCLUDED_MAC_CONTROLLER_H