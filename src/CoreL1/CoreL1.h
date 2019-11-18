/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#pragma once
#include <iostream>     //cout
#include <stdint.h>     //uint16_t
#include <sys/socket.h> //socket(), AF_INET, SOCK_DGRAM
#include <arpa/inet.h>  //struct sockaddr_in
#include <string.h>     //bzero()
#include <unistd.h>     //close()
using namespace std;

/**
 * @brief This class simulates the physical layer with UDP sockets. 
 */
class CoreL1{
private:
    int *socksIn;                   //Array of socket descriptors used to RECEIVE messages
    int *socksOut;                  //Array of sockets descriptors used to SEND messages
    struct sockaddr_in *socknames;  //Array of socket addess structs
    const char **ipServers;         //Array of IP addresses to which messages will be sent
    uint16_t *ports;                //Array of ports used to define IN and OUT sockets 
    int numSockets;                 //Number of actual sockets stored
    bool verbose;                   //Verbosity flag

public:

    /**
     * @brief Initializes a new instance of CoreL1 with 0 sockets
     * @param _verbose Verbosity flag
     */
    CoreL1(bool _verbose);

    /**
     * @brief Initializes a new instance of CoreL1 with 1 socket which informations were passed as parameters and no verbose
     * @param ip Destination IP address: L1 will send packets to this IP
     * @param port Socket port
     */
    CoreL1(const char* ip, uint16_t port);

    /**
     * @brief Initializes a new instance of CoreL1 with 1 socket which informations were passed as parameters
     * @param ip Destination IP address: L1 will send packets to this IP
     * @param port Socket port
     * @param _verbose Verbosity flag
     */
    CoreL1(const char* ip, uint16_t port, bool _verbose);

    /**
     * @brief Destructor of CoreL1 object
     */
    ~CoreL1();

    /**
     * @brief Adds new socket information in CoreL1. 
     * Declarates a client socket to send information to destination and a server socket to receive information from destination.
     * @param ip Destination IP address
     * @param port Socket port
     */
    void addSocket(const char* ip, uint16_t port);    
    
    /**
     * @brief Send PDU to socket considering there's just one socket added
     * @param buffer Information buffer
     * @param size Size of information in bytes
     * @returns If transmission was successfull
     */ 
    bool sendPdu(const char* buffer, size_t size);  

    /**
     * @brief Send PDU to socket identified by port
     * @param buffer Information buffer
     * @param size Size of information in bytes
     * @param port Socket port to identify which socket to send information
     * @returns True if transmission was successfull; False if it was not
     */
    bool sendPdu(const char* buffer, size_t size, uint16_t port);

    /**
     * @brief Receive PDU from socket considering there's just one socket added
     * @param buffer Information buffer to put the information received
     * @param maxSiz Maximum size of buffer
     * @returns Length in bytes of information received
     */   
    ssize_t receivePdu(const char* buffer, size_t maxSiz); 

    /**
     * @brief Receive PDU from socket identified by port
     * @param buffer Information buffer to put the information received
     * @param maxSiz Maximum size of buffer
     * @param port Socket port to identify which socket to receive information
     * @returns Length in bytes of information received
     */
    ssize_t receivePdu(const char* buffer, size_t maxSiz, uint16_t port);

    /**
     * @brief Get index of socket to receive information
     * @param port Socket port
     * @returns Socket index or -1 if socket was not found
     */
    int getSockIn(uint16_t port);

    /**
     * @brief Get index of socket to send information
     * @param port Socket port
     * @returns Socket index or -1 if socket was not found
     */
    int getSockOut(uint16_t port);

    /**
     * @brief Get ports currently added to CoreL1 object
     * @returns Array of ports
     */
    uint16_t* getPorts();  
};