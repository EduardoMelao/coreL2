/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_MAC_CONTROLLER_H
#define INCLUDED_MAC_CONTROLLER_H

#include <iostream>             //std::cout
#include <future>               //std::async, std::future
#include <chrono>               //std::chrono::milliseconds
#include <mutex>                //std::mutex
#include <condition_variable>   //std::condition_variable
#include <atomic>               //std::atomic 

enum MacModes {STANDBY_MODE, CONFIG_MODE, START_MODE, IDLE_MODE, RECONFIG_MODE, STOP_MODE};

#include "../ProtocolData/MacHighQueue.h"
#include "../ProtocolPackage/ProtocolPackage.h"
#include "../Multiplexer/MacAddressTable/MacAddressTable.h"
#include "../Multiplexer/TransmissionQueue.h"
#include "../Multiplexer/Multiplexer.h"
#include "../ReceptionProtocol/ReceptionProtocol.h"
#include "../TransmissionProtocol/TransmissionProtocol.h"
#include "../ProtocolControl/MacCtHeader.h"
#include "../ProtocolData/ProtocolData.h"
#include "../ProtocolControl/ProtocolControl.h"
#include "../../common/lib5grange/lib5grange.h"
#include "../../common/libMac5gRange/libMac5gRange.h"
#include "../SystemParameters/StaticDefaultParameters.h"
#include "../MacConfigRequest/MacConfigRequest.h"

using namespace std;

#define MAXSDUS 20                      //Maximum number of SDUs that can be enqueued for transmission
#define MAXIMUM_BUFFER_LENGTH 2048      //Maximum buffer length in bytes
#define SRC_OFFSET 12                   //IP packet source address offset in bytes 
#define DST_OFFSET 16                   //IP packet destination address offset in bytes
#define TIMEOUT_DYNAMIC_PARAMETERS 5    //Timeout(seconds) to check for dynamic parameters alterations


//Initializing classes that will be defined in other .h files
class ProtocolData;		
class ProtocolControl;

/**
 * @brief Class responsible for managing all MAC 5G-RANGE operations
 */
class MacController{
private:
    atomic<MacModes> currentMacMode;    //Current execution mode of MAC
    uint8_t currentMacAddress;          //MAC Address of current equipment
    const char* deviceNameTun;          //TUN device name
    TunInterface* tunInterface;         //TunInterface object to perform L3 packet capture
    MacHighQueue* macHigh;              //Queue to receive and enqueue L3 packets
    MacAddressTable* ipMacTable;        //Table to associate IP addresses to 5G-RANGE domain MAC addresses
	ProtocolData* protocolData;         //Object to deal with enqueueing DATA SDUS
    ProtocolControl* protocolControl;   //Object to deal with enqueueing CONTROL SDUS
	thread *threads;                    //Threads array
    MacPDU macPDU;                      //Object MacPDU containing all information that will be sent to PHY
    unsigned int subframeCounter;       //Subframe counter used for RxMetrics reporting to BS.
    bool verbose;                       //Verbosity flag

public:
    condition_variable* queueConditionVariables;    //Condition variables to manage access to Multiplexer Queues
    mutex queueMutex;               //Mutex to control access to Transmission Queue
	Multiplexer* mux;               //Multiplexes various SDUs to multiple destinations
    bool flagBS;                    //BaseStation flag: 1 for BS; 0 for UE
    ReceptionProtocol* receptionProtocol;           //Object to receive packets from L1 and L3
    TransmissionProtocol* transmissionProtocol;     //Object to transmit packets to L1 and L3
    L1L2Interface* l1l2Interface;   //Object to manage interface with L1
    StaticDefaultParameters* staticParameters;      //Object with static/default parameters read from a file
    MacConfigRequest* macConfigRequest;             //Object to configure dynamic parameters 
    RxMetrics* rxMetrics;                           //Array of Reception Metrics for each UE
    
    /**
     * @brief Initializes a MacController object to manage all 5G RANGE MAC Operations
     * @param _deviceNameTun Customized name for TUN Interface
     * @param _verbose Verbosity flag
     */
    MacController(const char* _deviceNameTun, bool _verbose);
    
    /**
     * @brief Destructs MacController object
     */
    ~MacController();

    /**
     * @brief Initializes MAC System in STANDBY_MODE
     */
    void initialize();

    /**
     * @brief Main thread of MAC, controls all system modes
     */
    void manager();

    /**
     * @brief Called by RECONFIG_MODE, updates static parameters with dynamic parameters modified by CLI.
     */
    void recordDynamicParameters();

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
     * @param macAddress Source MAC Address from which packet will be received
     * @returns Source MAC Address
     */
    uint8_t decoding();

    /**
     * @brief Sets MAC PDU object with static information
     * @param numberBytes Number of Data bytes to send
     * @param macAddress User equipment MAC Address (if it is sending or if is destination)
     */
    void setMacPduStaticInformation(size_t numberBytes, uint8_t macAddress);

    /**
     * @brief [UE] Receives bytes referring to Dynamic Parameters coming by MACC SDU and updates class with new information
     * @param bytesDynamicParameters Serialized bytes from MacConfigRequest object
     * @param size Number of bytes of serialized information
     */ 
    void managerDynamicParameters(uint8_t* bytesDynamicParameters, size_t numberBytes);

    /**
     * @brief (Only UE) Periodically sends RxMetrics Report to BS
     */
    void rxMetricsReport();

    /**
     * @brief Gets index referent to macAddress on all class arrays
     * @param macAddress MAC Address 
     */
    int getIndex(uint8_t macAddress);
};
#endif  //INCLUDED_MAC_CONTROLLER_H
