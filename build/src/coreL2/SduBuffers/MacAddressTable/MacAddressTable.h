/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_MAC_ADDRESS_TABLE_H
#define INCLUDED_MAC_ADDRESS_TABLE_H

#include <stdint.h> //uint8_t
#include <iostream> //cout

/**
 * @brief Table of correlation of IP Addresses and 5G-RANGE MAC Addresses
 */
class MacAddressTable{
private:
    int numberRegisters;        //Number of current registers in the table
    uint8_t** ipAddresses;      //Array of IP strings
    uint8_t* macAddresses;      //Array of MAC Addresses
    bool verbose;               //Verbosity flag

public:
    /**
     * @brief Constructs empty table with no verbosity
     */
    MacAddressTable();

    /**
     * @brief Constructs empty table with verbosity passed as parameter
     * @param _verbose Verbosity flag
     */
    MacAddressTable(bool verbose);

    /**
     * @brief Destroys MacAddressTable
     */
    ~MacAddressTable();

    /**
     * @brief Gets the total number of registers in table
     * @returns Number of current registers on table
     */
    int getNumberRegisters();  
    
    /**
     * @brief Prints MacAddressTable Table: ID, IP Address and MAC Address
     */ 
    void printMacTable();
    
    /**
     * @brief Adds a new entry to the table
     * @param ipAddress IP Address
     * @param macAddress Corresponding MAC Address
     */   
    void addEntry(uint8_t* ipAddress, uint8_t macAddress); 

    /**
     * @brief Deletes the entry which ID is passed as parameter
     * @param id Identification of register
     */
    void deleteEntry(int id); 
    
    /**
     * @brief Gets MAC Address from table, given IP Address
     * @param ipAddr IP Address
     * @returns Corresponding MAC Address; -1 if entry is not found
     */  
    uint8_t getMacAddress(uint8_t* ipAddress);

    /**
     * @brief Gets IP Address from table given, MAC Address
     * @param macAddr MAC Address
     * @returns Corresponding IP Address; 0 if entry is not found
     */
    uint8_t* getIpAddress(uint8_t macAddress);
};
#endif  //INCLUDED_MAC_ADDRESS_TABLE_H