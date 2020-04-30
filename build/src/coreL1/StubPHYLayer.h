/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_CORE_L1_H
#define INCLUDED_CORE_L1_H

#include <iostream>     //cout
#include <stdint.h>     //uint16_t
#include <sys/socket.h> //socket(), AF_INET, SOCK_DGRAM
#include <arpa/inet.h>  //struct sockaddr_in
#include <string.h>     //bzero()
#include <vector>
#include <unistd.h>     //close()
#include <thread>       //thread
#include <mqueue.h>     //POSIX Message Queues
#include <errno.h>      //errno
#include "../common/lib5grange/lib5grange.h"
#include "../common/libMac5gRange/libMac5gRange.h"

using namespace std;
using namespace lib5grange;

/**
 * @brief This class simulates the physical layer with UDP sockets. 
 */
class CoreL1{
private:
    int *socketsIn;                         //Array of socket descriptors used to RECEIVE messages
    int *socketsOut;                        //Array of sockets descriptors used to SEND messages
    struct sockaddr_in *socketNames;        //Array of socket address structs
    const char **ipServers;                 //Array of IP addresses to which messages will be sent
    uint16_t *ports;                        //Array of ports used to define IN and OUT sockets 
    uint8_t currentMacAddress;              //MAC address of this equipment
    uint8_t *macAddresses;                  //Array of MAC addresses of each destination
    int numberSockets;                      //Number of actual sockets stored

    l1_l2_interface_t l1l2Interface;        //Object containing all message queues to change messages with L2

    int subFrameCounter;                    //Counter to trigger RX Metrics sending to MAC
    uint8_t rxMetricsPeriodicity;           //Periodicity to send Rx metrics to MAC, in number of Subframes
    bool phyActive;                         //Flag to control PHY activation and deactivation
    bool verbose;                           //Verbosity flag

    /**
     * @brief Creates a new socket to serve as sender of messages
     * @param port Socket port
     * @param serverReceiverOfMessage Struct used to send message later
     * @param serverIp Ip address of server
     * @returns Socket file descriptor used to send message later
     */
    int createClientSocketToSendMessages(short port, struct sockaddr_in *serverReceiverOfMessage, const char* serverIp);

    /**
     * @brief Creates a new socket to serve as receiver of messages
     * @param port Socket port
     * @returns Socket file descriptor
     */
    int createServerSocketToReceiveMessages(short port);

public:

    /**
     * @brief Initializes a new instance of CoreL1 with 0 sockets
     * @param _macAddress MAC Address of this equipment
     * @param _verbose Verbosity flag
     */
    CoreL1(uint8_t _macAddress, bool _verbose);

    /**
     * @brief Destructor of CoreL1 object
     */
    ~CoreL1();

    /**
     * @brief Adds new socket information in CoreL1. 
     * Declarates a client socket to send information to destination and a server socket to receive information from destination.
     * @param ip Destination IP address
     * @param port Socket port
     * @param macAddress Destination MAC Address 
     */
    void addSocket(const char* ip, uint16_t port, uint8_t macAddress);    

    /**
     * @brief Send PDU to socket identified by port
     * @param buffer Information buffer
     * @param size Size of information in bytes
     * @param port Socket port to identify which socket to send information
     * @returns True if transmission was successful; False if it was not
     */
    bool sendPdus(const char* buffer, size_t size, uint16_t port);

    /**
     * @brief Receive PDU from socket considering there's just one socket added
     * @param buffer Information buffer to put the information received
     * @param maxSiz Maximum size of buffer
     * @returns Length in bytes of information received
     */   
    ssize_t receivePdus(const char* buffer, size_t maxSiz); 

    /**
     * @brief Receive PDU from socket identified by port
     * @param buffer Information buffer to put the information received
     * @param maxSiz Maximum size of buffer
     * @param port Socket port to identify which socket to receive information
     * @returns Length in bytes of information received
     */
    ssize_t receivePdus(const char* buffer, size_t maxSiz, uint16_t port);

    /**
     * @brief Get index of socket to send/receive information
     * @param port Socket port
     * @returns Socket index or -1 if socket was not found
     */
    int getSocketIndex(uint16_t port);

    /**
     * @brief Get index of socket to send/receive information
     * @param macAddress Socket refering destination MAC address
     * @returns Socket index or -1 if socket was not found
     */
    int getSocketIndex(uint8_t macAddress);

    /**
     * @brief Encoding function: executes forever and receives Data packets from L2 and send them to destination socket
     * @param numberPdus Number of PDUs for transmission
     */
    void encoding(uint8_t numberPdus);

    /**
     * @brief Decoding function: executes forever and forward received data packets to L2
     * @param macAddress Address of equipment which the procedure will listen to
     */
    void decoding(uint8_t macAddress);

    /**
     * @brief Sends Control Message to L2
     * @param buffer Buffer containing message
     * @param numberBytes Size of message in bytes
     */
    void sendInterlayerMessage(char* buffer, size_t numberBytes);

    /**
     * @brief Receives Control Messages from L2
     * @returns Size of message received in Bytes
     */
    void receiveInterlayerMessage();

    /**
     * @brief Declares and starts all threads necessary for CoreL1
     */
    void startThreads();

    /**
     * @brief Sends PHYTx.Indication periodically to MAC to get Transport Block for transmission
     */
    void sendTxIndication();
};
#endif //INCLUDED_CORE_L1_H
