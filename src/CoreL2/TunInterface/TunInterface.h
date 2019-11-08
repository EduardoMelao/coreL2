#pragma once
#include <net/if.h>   //IFNAMSIZ, struct ifreq
#include <linux/if_tun.h>
#include <unistd.h>     //read(), close()
#include <iostream>     //cout
#include <string.h>     //memset()
#include <sys/types.h>  //size_t
#include <fcntl.h>      //open(), O_RDWR
#include <sys/ioctl.h>  //ioctl()

class TunInterface{
private:
    int fd;
    char* dev;
    bool verbose;
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