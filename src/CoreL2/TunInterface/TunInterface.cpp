/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "TunInterface.h"
using namespace std;    

TunInterface::TunInterface(){
    TunInterface("tun0", false);
}

TunInterface::TunInterface(bool _verbose){
    TunInterface("tun0", _verbose);
}

TunInterface::TunInterface(const char* devName){
    TunInterface(devName,false);
}

TunInterface::TunInterface(const char* devName, bool v){
    verbose = v;
    deviceName = new char[IFNAMSIZ+1];
    memset(deviceName,0,IFNAMSIZ+1);
    if(devName!=NULL) strncpy(deviceName,devName,sizeof(deviceName)-1);
}

TunInterface::~TunInterface(){
    delete[] deviceName;
    close(fileDescriptor);
}

bool 
TunInterface::allocTunInterface(){
    //Test if dev name is valid
    if(deviceName==NULL){
        if(verbose) cout << "[TunInterface] Error creating interface: dev = NULL." << endl;
        return false;
    }

    //Open file descriptor
    fileDescriptor = open("/dev/net/tun", O_RDWR);

    //Creates and sets interface requirement struct
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, deviceName, IFNAMSIZ);

    //Calls system in/out control to set interface active
    ioctl(fileDescriptor, TUNSETIFF, (void *) &ifr);
    strncpy(deviceName,ifr.ifr_name, IFNAMSIZ);

    //Forces interface to me initialized as "UP"
    char cmd[100];
    sprintf(cmd, "ifconfig %s up", ifr.ifr_ifrn.ifrn_name);
    system(cmd);
    if(verbose) cout << "[TunInterface] Tun interface allocated successfully." << endl;
    return true;
}

ssize_t 
TunInterface::readTunInterface(
    char* buffer,           //Buffer to store packet read
    size_t numberBytes)     //Number of bytes read
{
    return read(fileDescriptor, buffer, numberBytes);
}

bool 
TunInterface::writeTunInterface(
    char* buffer,           //Buffer containing L3 packet
    size_t numberBytes)     //Number of bytes to write
{
    ssize_t returnValue = write(fileDescriptor, buffer, numberBytes);
    if(returnValue==-1){
        if(verbose) cout<<"[TunInterface] Could not write to Tun Interface."<<endl;
        return false;
    }
    return true;
}
