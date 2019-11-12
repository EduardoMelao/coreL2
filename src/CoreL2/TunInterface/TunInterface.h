/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#pragma once
#include <net/if.h>   //IFNAMSIZ, struct ifreq
#include <linux/if_tun.h>
#include <unistd.h>     //read(), close()
#include <iostream>     //cout
#include <string.h>     //memset()
#include <sys/types.h>  //size_t
#include <fcntl.h>      //open(), O_RDWR
#include <sys/ioctl.h>  //ioctl()

/**
 * @brief Class to alloc, save the descriptor and manage operations of TUN interface
 */
class TunInterface{
private:
    int fd;         //File descriptor of the interface
    char* dev;      //[optional] Name of the interface
    bool verbose;   //Verbosity flag
public:
    TunInterface();
    TunInterface(bool v);
    TunInterface(const char* devName);
    TunInterface(const char* devName, bool v);
    ~TunInterface();
    bool allocTunInterface();
    ssize_t readTunInterface(char* buf, size_t n_bytes);
    bool writeTunInterface(char* buf, size_t n);
};