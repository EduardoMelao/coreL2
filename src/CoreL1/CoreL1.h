#pragma once
#include <iostream>     //cout
#include <stdint.h>     //uint16_t
#include <sys/socket.h> //socket(), AF_INET, SOCK_DGRAM
#include <arpa/inet.h>  //struct sockaddr_in
#include <string.h>     //bzero()
#include <unistd.h>     //close()
using namespace std;

class CoreL1{
private:
    int *socksIn;     //Accept messages: Act as server
    int *socksOut;    //Send messages: Act as client
    struct sockaddr_in *socknames;
    const char **ipServers;   //Messages will be sent to this ip
    uint16_t *ports;  //Universal port 
    int numSockets;     //Number of actual sockets stored
    bool verbose;

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