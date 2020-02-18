/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_L1_L2_INTERFACE_H
#define INCLUDED_L1_L2_INTERFACE_H

#define PORT_TO_L1 8090
#define PORT_FROM_L1 8091
#define CONTROL_MESSAGES_PORT_TO_L1 8092
#define CONTROL_MESSAGES_PORT_FROM_L1 8093

#include <iostream>
#include <vector>
#include <sys/socket.h> //socket(), AF_INET, SOCK_DGRAM
#include <arpa/inet.h>  //struct sockaddr_in
#include <string.h>     //bzero()
#include <unistd.h>     //close()
#include "../../common/lib5grange/lib5grange.h"

using namespace std;
using namespace lib5grange;

class L1L2Interface{
private:
    int socketPduFromL1;                        //File descriptor of socket used to RECEIVE PDUs from L1
    int socketPduToL1;                          //File descriptor of socket used to SEND PDUs to L1
    int socketControlMessagesFromL1;            //File descriptor of socket used to RECEIVE Control Messages from L1
    int socketControlMessagesToL1;              //File descriptor of socket used to SEND Control Messages to L1
    struct sockaddr_in serverPdusSocketAddress; //Address of server to which client will send PDUs
    struct sockaddr_in serverControlMessagesSocketAddress;  //Address of server to which client will send control messages
    bool verbose;                               //Verbosity flag

    /**
    * @brief Auxiliary function for CRC calculation
    * @param data Single byte from PDU
    * @param crc CRC history
    * @returns 2-byte CRC calculation
    */
    unsigned short auxiliaryCalculationCRC(char data, unsigned short crc);

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
     * @brief Constroys a L1L2Interface object, initializes class variables with static information and allocate sockets tocommunicate with PHY
     * @param _verbose verbosity flag
     */
    L1L2Interface(bool _verbose);

    /**
     * @brief Destroys a L1L2Interface object
     */
    ~L1L2Interface();

    /**
     * @param macPdu MAC PDU structure containing all information PHY needs
     * @param macAddress Destination MAC Address
     * @returns True if transmission was successful, false otherwise
     */
    void sendPdu(MacPDU _macPdu, uint8_t macAddress);

    /**
     * @brief Received a PDU from PHY Layer
     * @param buffer Buffer where PDU is going to be store
     * @param maximumSize Maximum size of PDU
     * @returns Received PDU size in bytes
     */
    ssize_t receivePdu(const char* buffer, size_t maximumSize);

    /**
     * @brief Sends Control Message to PHY
     * @param buffer Buffer containing message
     * @param numberBytes Size of message in bytes
     */
    void sendControlMessage(char* buffer, size_t numberBytes);

    /**
     * @brief Received Control Message from PHY
     * @param buffer Buffer where message will be stored
     * @param maximumLength Maximum message length in Bytes
     * @returns Size of message received in Bytes
     */
    ssize_t receiveControlMessage(char* buffer, size_t maximumLength);

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
#endif  //INCLUDED_L1_L2_INTERFACE_H
