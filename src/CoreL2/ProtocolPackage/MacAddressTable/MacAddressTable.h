#pragma once
#include <stdint.h> //uint8_t
#include <iostream> //cout

class MacAddressTable{
private:
    int numRegs;    //Number of current register in the table
    uint8_t** ipAddrs;    //Array of IP strings
    uint8_t* macAddrs;  //Array of MAC Addresses
    bool verbose;  
public:
    MacAddressTable();
    MacAddressTable(bool verbose);
    ~MacAddressTable();
    int getNumRegs();   
    void printMacTable();   
    void addEntry(uint8_t* ipAddress, uint8_t macAddress); 
    void deleteEntry(int id);   
    uint8_t getMacAddress(uint8_t* ipAddress);
    uint8_t getMacAddress(int id);
    uint8_t* getIpAddress(uint8_t macAddress);
    uint8_t* getIpAddress(int id);
};