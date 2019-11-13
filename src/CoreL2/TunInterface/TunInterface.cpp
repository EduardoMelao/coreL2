/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "TunInterface.h"
using namespace std;    

/**
 * @brief Creates interface with name "tun0" and no verbosity
 */
TunInterface::TunInterface(){
    TunInterface("tun0", false);
}

/**
 * @brief Creates interface with name "tun0"
 * @param v Verbosity flag
 */
TunInterface::TunInterface(bool v){
    TunInterface("tun0", v);
}

/**
 * @brief Creates interface with no verbosity
 * @param devName Interface name
 */
TunInterface::TunInterface(const char* devName){
    TunInterface(devName,false);
}

/**
 * @brief Creates interface
 * @param devName Interface name
 * @param v Verbosity flag
 */
TunInterface::TunInterface(const char* devName, bool v){
    verbose = v;
    dev = new char[IFNAMSIZ+1];
    memset(dev,0,IFNAMSIZ+1);
    if(devName!=NULL) strncpy(dev,devName,sizeof(dev)-1);
}

/**
 * @brief Destroys TUN interface
 */
TunInterface::~TunInterface(){
    delete[] dev;
    close(fd);
}

/**
 * @brief Allocates TUN interface
 * @returns true if successful, false otherwise
 */
bool 
TunInterface::allocTunInterface(){
    //Test if dev name is valid
    if(dev==NULL){
        if(verbose) cout << "[TunInterface] Error creating interface: dev = NULL." << endl;
        return false;
    }

    //Open file descriptor
    fd = open("/dev/net/tun", O_RDWR);

    //Creates and sets interface requirement struct
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    //Calls system in/out control to set interface active
    ioctl(fd, TUNSETIFF, (void *) &ifr);
    strncpy(dev,ifr.ifr_name, IFNAMSIZ);

    //Forces interface to me initialized as "UP"
    char cmd[100];
    sprintf(cmd, "ifconfig %s up", ifr.ifr_ifrn.ifrn_name);
    system(cmd);
    if(verbose) cout << "[TunInterface] Tun interface allocated successfully." << endl;
    return true;
}

/**
 * @brief Performs reading of packets in the interface; Blocks if there is no packet to read
 * @param buf Buffer to store packet read
 * @param n_bytes Maximum number of bytes to read
 * @returns Number of bytes read; 0 for EOF; -1 for errors
 */
ssize_t 
TunInterface::readTunInterface(char* buf, size_t n_bytes){
    return read(fd, buf, n_bytes);
}

/**
 * @brief Performs writing in TUN interface to take packet back to Linux system
 * @param buf Buffer containing L3 packet
 * @param n Number of bytes to write
 * @returns true if writing was successfull, false otherwise
 */
bool 
TunInterface::writeTunInterface(char* buf, size_t n){
    ssize_t returnValue = write(fd, buf, n);
    if(returnValue==-1){
        if(verbose) cout<<"[TunInterface] Could not write to Tun Interface."<<endl;
        return false;
    }
    return true;
}
