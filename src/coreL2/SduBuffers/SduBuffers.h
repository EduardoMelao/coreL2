/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef SDU_BUFFERS_H
#define SDU_BUFFERS_H

#include <vector>                                       //Auxiliar library to manage arrays
#include <mutex>                                        //Mutex Semaphores to prevent different threads trying to write the same variable
#include "MacAddressTable/MacAddressTable.h"            //IP <-> MAC mapping
#include "../ReceptionProtocol/ReceptionProtocol.h"     //Reception Protocol to manage reception of L3 packets from TUN
#include "../SystemParameters/CurrentParameters.h"      //System Current Parameters contains number of UEs and their IDs (MAC addresses)
#include "../../common/libMac5gRange/libMac5gRange.h"   //MAC Common library contains system's states and substates

#define MAXIMUM_BUFFER_LENGTH 2048  //Maximum buffer size
#define DST_OFFSET 16               //Destination address offset in IP Packet [bytes]    

using namespace std;

/**
 * @brief Queues to store packets received from Linux IP layer and control packets (MACC SDUs) with control information to the other side
 */
class SduBuffers{
private:
    //System variables
    ReceptionProtocol* reception;           //Object to receive packets from L3
    MacAddressTable* ipMacTable;            //Table of correlation of IP Addresses and MAC5GR Addresses
    CurrentParameters* currentParameters;    //Object with the parameters that are currently being used by the system
    bool verbose;                           //Verbosity flag

    //Buffers
    vector<vector<char*>> dataSduQueue;     //Vector of L3 packets - MACD SDUs for each destination
    vector<vector<char*>> controlSduQueue;  //Vector of MACC SDUs for each destination
    vector<vector<ssize_t>> dataSizes;      //Vector containing size of MACD SDU for each destination
    vector<vector<ssize_t>> controlSizes;   //Vector containing size of MACC SDU for each destination

    //Mutexes
    mutex dataMutex;                        //Mutex to control access to data queue
    mutex controlMutex;                     //Mutex to control access to control queue

public:
    /**
     * @brief Constructs empty buffers for destinations and defines a ReceptionProtocol with a TUN descriptor
     * @param _reception Object to receive packets from L3
     * @param _currentParameters Parameters that are currently being used by the system
     * @param _ipMacTable Table of correlation of IP Addresses and MAC5GR Addresses
     * @param _verbose Verbosity flag 
     */
    SduBuffers(ReceptionProtocol* _reception, CurrentParameters* _currentParameters, MacAddressTable* _ipMacTable, bool _verbose);
    
    /**
     * @brief Destroys SduBuffers
     */
    ~SduBuffers();
    
    /**
     * @brief Procedure that executes forever, receiving packets from L3 and storing them in the data queue
     * @param currentMacMode Actual MAC Mode to control enqueueing while system is in another modes, e.g. RECONFIG_MODE or STOP_MODE
     * @param currentMacTunMode Actual MAC Tun Mode to signal to system if it is in an active mode, e.g. TUN_DISABLED
     */
    void enqueueingDataSdus(MacModes & currentMacMode, MacTunModes & currentMacTunMode);

    /**
     * @brief Given the SDU, open it and look for its IP in Mac Address Table
     * @param dataSDU SDU containing IP bytes
     * @returns Destination MAC Address
     */
    uint8_t getMacAddress(char* dataSdu);
    
    /**
     * @brief Adds a MACC SDU to buffer for transmission
     * @param controlSdu MAC Control SDU
     * @param numberBytes Soze of MACC SDU in Bytes
     * @param macAddress Destination MAC Address
     */
    void enqueueControlSdu( uint8_t* controlSdu, size_t numberBytes, uint8_t macAddress);

    /**
     * @brief Gets number of data packets that are currently enqueued
     * @param macAddress Destination MAC Address
     * @returns Number of data packets enqueued; -1 for errors
     */
    int getNumberDataSdus(uint8_t macAddress);

    /**
     * @brief Gets number of control packets that are currently enqueued
     * @param macAddress Destination MAC Address
     * @returns Number of control packets enqueued; -1 for errors
     */
    int getNumberControlSdus(uint8_t macAddress);

    /**
     * @brief Informs the Scheduler about the state of SDU buffers
     * @param macAddress Destination MAC Address
     * @returns TRUE if there are packets to schedule; FALSE otherwise
     */
    bool bufferStatusInformation(uint8_t macAddress);
    
    /**
     * @brief Gets next data SDU on queue for treatment
     * @param macAddress Destination MAC Address
     * @param buffer Buffer where data SDU is stored
     * @returns Size of data SDU; -1 for errors
     */
    ssize_t getNextDataSdu(uint8_t macAddress, char* buffer);

    /**
     * @brief Gets next control SDU on queue for treatment
     * @param macAddress Destination MAC Address
     * @param buffer Buffer where control SDU is stored
     * @returns Size of control SDU; -1 for errors
     */
    ssize_t getNextControlSdu(uint8_t macAddress, char* buffer);
    
    /**
     * @brief Gets next data SDU on queue for treatment
     * @param macAddress Destination MAC Address
     * @returns Size of data SDU; -1 for errors
     */
    ssize_t getNextDataSduSize(uint8_t macAddress);

    /**
     * @brief Gets next control SDU on queue for treatment
     * @param macAddress Destination MAC Address
     * @returns Size of control SDU; -1 for errors
     */
    ssize_t getNextControlSduSize(uint8_t macAddress);
};
#endif  //SDU_BUFFERS_H