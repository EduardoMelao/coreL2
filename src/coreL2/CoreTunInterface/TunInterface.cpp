/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : TunInterface.cpp
@Classification : TUN Interface
@
@Last alteration : November 28th, 2019
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module creates, allocates and manages reading and writing to TUN Interface. 
*/

#include "TunInterface.h"
using namespace std;    

TunInterface::TunInterface(){
    TunInterface("tun0", false);
}

TunInterface::TunInterface(
    bool _verbose)      //Verbosity flag
{
    TunInterface("tun0", _verbose);
}

TunInterface::TunInterface(
    const char* _deviceName)        //Interface name
{
    TunInterface(_deviceName,false);
}

TunInterface::TunInterface(
    const char* _deviceName,        //Interface name
    bool _verbose)              //Verbosity flag
{
    verbose = _verbose;
    deviceName = new char[IFNAMSIZ+1];
    memset(deviceName,0,IFNAMSIZ+1);
    if(_deviceName!=NULL) strncpy(deviceName,_deviceName,sizeof(deviceName)-1);
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
    struct ifreq interfaceRequirement;
    memset(&interfaceRequirement, 0, sizeof(interfaceRequirement));
    interfaceRequirement.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(interfaceRequirement.ifr_name, deviceName, IFNAMSIZ);

    //Calls system in/out control to set interface active
    ioctl(fileDescriptor, TUNSETIFF, (void *) &interfaceRequirement);
    strncpy(deviceName,interfaceRequirement.ifr_name, IFNAMSIZ);

    //Forces interface to me initialized as "UP"
    char cmd[100];
    sprintf(cmd, "ifconfig %s up", interfaceRequirement.ifr_ifrn.ifrn_name);
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
