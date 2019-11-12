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
    int *socksIn;                   //Array of sockets used to RECEIVE messages
    int *socksOut;                  //Array of sockets used to SEND messages
    struct sockaddr_in *socknames;  //Array of socket addesses structs
    const char **ipServers;         //Array of IP addresses to which messages will be sent
    uint16_t *ports;                //Array of ports used to define IN and OUT sockets 
    int numSockets;                 //Number of actual sockets stored
    bool verbose;                   //Verbosity flag

public:
    CoreL1(bool v);
    CoreL1(const char* ip, uint16_t port);
    CoreL1(const char* ip, uint16_t port, bool v);
    ~CoreL1();
    void addSocket(const char* ip, uint16_t port);      
    bool sendPdu(const char* buf, size_t n);  
    bool sendPdu(const char* buf, size_t n, uint16_t port);   
    ssize_t receivePdu(const char* buf, size_t maxSiz); 
    ssize_t receivePdu(const char* buf, size_t maxSiz, uint16_t port);       
    int getSockIn(uint16_t port);   
    int getSockOut(uint16_t port);
    uint16_t* getPorts();  
};