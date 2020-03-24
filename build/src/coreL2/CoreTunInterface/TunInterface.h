/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_TUN_INTERFACE_H
#define INCLUDED_TUN_INTERFACE_H

#include <net/if.h>   //IFNAMSIZ, struct ifreq
#include <linux/if_tun.h>
#include <unistd.h>     //read(), close()
#include <iostream>     //cout
#include <string.h>     //memset()
#include <sys/types.h>  //size_t
#include <fcntl.h>      //open(), O_RDWR
#include <sys/ioctl.h>  //ioctl()
#include <sys/time.h>   //struct timeval
#include <sys/types.h>  //select()

#define TIMEOUT_SELECT 1    //select() procedure timeout in nanoseconds  

/**
 * @brief Class to alloc, save the descriptor and manage operations of TUN interface
 */
class TunInterface{
private:
    int fileDescriptor;         //File descriptor of the interface
    uint16_t mtu;               //Maximum Transmission Unit for the interface
    char* deviceName;           //[optional] Name of the interface
    fd_set fileDescriptorSet;   //File descriptor Set
    bool verbose;               //Verbosity flag

public:
    /**
     * @brief Creates interface with name "tun0" and no verbosity
     */
    TunInterface();
    
    /**
     * @brief Creates interface with name "tun0"
     * @param _verbose Verbosity flag
     */
    TunInterface(bool _verbose);
    
    /**
     * @brief Creates interface with no verbosity
     * @param deviceName Interface name
     */
    TunInterface(const char* deviceName);
   
    /**
     * @brief Creates interface
     * @param deviceName Interface name
     * @param _mtu Interface mtu
     * @param _verbose Verbosity flag
     */
    TunInterface(const char* deviceName, uint16_t _mtu, bool _verbose);
    
    /**
     * @brief Destroys TUN interface
     */
    ~TunInterface();
        
    /**
     * @brief Allocates TUN interface
     * @returns true if successful, false otherwise
     */
    bool allocTunInterface();
    
    /**
     * @brief Performs reading of packets in the interface; Blocks if there is no packet to read
     * @param buffer Buffer to store packet read
     * @param numberBytes Maximum number of bytes to read
     * @returns Number of bytes read; 0 for EOF; -1 for errors
     */
    ssize_t readTunInterface(char* buffer, size_t numberBytes);
 
    /**
     * @brief Performs writing in TUN interface to take packet back to Linux system
     * @param buffer Buffer containing L3 packet
     * @param numberBytes Number of bytes to write
     * @returns true if writing was successful, false otherwise
     */   
    bool writeTunInterface(char* buffer, size_t numberBytes);

    /**
     * @brief Performs select() procedure on TUN interface and returns its state
     * @returns -1 for errors; 0 for TIMEOUT; >0 if interface has data to be read
     */
    bool isTunInterfaceReady();
};
#endif  //INCLUDED_TUN_INTERFACE_H