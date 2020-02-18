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

#include "../SduBuffers/SduBuffers.h"
#include "../ProtocolPackage/ProtocolPackage.h"
#include "../SduBuffers/MacAddressTable/MacAddressTable.h"
#include "../Multiplexer/AggregationQueue.h"
#include "../Multiplexer/Multiplexer.h"
#include "../ReceptionProtocol/ReceptionProtocol.h"
#include "../TransmissionProtocol/TransmissionProtocol.h"
#include "../ProtocolControl/ProtocolControl.h"
#include "../../common/lib5grange/lib5grange.h"
#include "../../common/libMac5gRange/libMac5gRange.h"
#include "../SystemParameters/CurrentParameters.h"
#include "../CLIL2Interface/CLIL2Interface.h"

using namespace std;

#define MAXIMUM_BUFFER_LENGTH 2048      //Maximum buffer length in bytes
#define TIMEOUT_DYNAMIC_PARAMETERS 5    //Timeout(seconds) to check for dynamic parameters alterations


//Initializing classes that will be defined in other .h files
class ProtocolControl;

/**
 * @brief Class responsible for managing all MAC 5G-RANGE operations
 */
class MacController{
private:
    //Control Variables
    MacModes currentMacMode;        //Current execution mode of MAC
    MacTxModes currentMacTxMode;    //Current execution Tx mode of MAC
    MacRxModes currentMacRxMode;    //Current execution Rx mode of MAC
    MacTunModes currentMacTunMode;  //Current execution Tun mode of MAC

    uint8_t currentMacAddress;              //MAC Address of current equipment
    const char* deviceNameTun;              //TUN device name
    TunInterface* tunInterface;             //TunInterface object to perform L3 packet capture
    MacAddressTable* ipMacTable;            //Table to associate IP addresses to 5G-RANGE domain MAC addresses
    ProtocolControl* protocolControl;       //Object to deal with enqueueing CONTROL SDUS
	thread *threads;                        //Threads array
    MacPDU macPDU;                          //Object MacPDU containing all information that will be sent to PHY
    unsigned int subframeCounter;           //Subframe counter used for RxMetrics reporting to BS.
    bool verbose;                           //Verbosity flag

public:
    SduBuffers* sduBuffers;                 //Queues to receive and enqueue Data Sdus (L3 packets) and Control SDUs
    bool flagBS;                    //BaseStation flag: 1 for BS; 0 for UE
    ReceptionProtocol* receptionProtocol;           //Object to receive packets from L1 and L3
    TransmissionProtocol* transmissionProtocol;     //Object to transmit packets to L1 and L3
    L1L2Interface* l1l2Interface;   //Object to manage interface with L1
    CurrentParameters* currentParameters;           //Object with static/default parameters read from a file
    CLIL2Interface* cliL2Interface;             //Object to configure dynamic parameters 
    
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
     * @brief Declares and starts all threads necessary for MacController
     */
    void startThreads();

    /**
     * @brief Immediately schedules SDUs for transmission
     */
    void provisionalScheduling();

    /**
     * @brief Sends Pdu contained in MUX to MacAddress passed as parameter
     * @param mux Multiplexer object containing multiplexed SDUs
     * @param macAddress Destination MAC Address
     */
    void sendPdu(Multiplexer* mux, uint8_t macAddress);

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
};
#endif  //INCLUDED_MAC_CONTROLLER_H
