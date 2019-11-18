/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "MacAddressTable.h"

using namespace std;

MacAddressTable::MacAddressTable(){
    MacAddressTable(false);
}

MacAddressTable::MacAddressTable(
    bool _verbose)  //Verbosity flag
{
    this->verbose = _verbose;
    numRegisters = 0;
    if(verbose) cout<<"[MacAddressTable] Created Mac Address Table"<<endl;
}

MacAddressTable::~MacAddressTable(){
    while(numRegisters>0){
        deleteEntry(0);
    }
}

int 
MacAddressTable::getNumRegisters(){
    return numRegisters;
}

void 
MacAddressTable::printMacTable(){
    cout<<"ID \t IP \t\t MAC"<<endl;
    for(int i=0;i<numRegisters;i++){
        cout<<i<<"\t"<<(int)ipAddrs[i][0]<<"."<<(int)ipAddrs[i][1]<<"."<<(int)ipAddrs[i][2]<<"."<<(int)ipAddrs[i][3]<<"."<<"\t"<<(int)macAddrs[i]<<endl;
    }
}

void 
MacAddressTable::addEntry(
    uint8_t* ipAddress,     //Entry IP Address
    uint8_t macAddress)     //Entre 5GR MAC Address
{
    //Relocate arrays
    uint8_t** _ipAddrs = new uint8_t*[numRegisters+1];
    uint8_t* _macAddrs = new uint8_t[numRegisters+1];

    //Copy old information
    for(int i=0;i<numRegisters;i++){
        _ipAddrs[i] = ipAddrs[i];
        _macAddrs[i] = macAddrs[i];
    }

    //Add new information
    _ipAddrs[numRegisters] = ipAddress;
    _macAddrs[numRegisters] = macAddress;

    //Delete old arrays
    delete[] ipAddrs;
    delete[] macAddrs;

    //Renew class arrays
    this->ipAddrs = _ipAddrs;
    this->macAddrs = _macAddrs;
    if(verbose) cout<<"[MacAddressTable] Entry added"<<endl;

    //Increment number of registers
    numRegisters++;
}

void 
MacAddressTable::deleteEntry(
    int id)     //Identification of the entry
{
    //Verify ID. ID is sequential
    if(id>(numRegisters-1)){
        if(verbose) cout<<"[MacAddressTable] Invalid ID"<<endl;
        return;
    }

    //Relocate arrays
    uint8_t** _ipAddrs = new uint8_t*[numRegisters-1];
    uint8_t* _macAddrs = new uint8_t[numRegisters-1];

    //Copy information
    for(int i=0;i<id;i++){
        _ipAddrs[i] = ipAddrs[i];
        _macAddrs[i] = macAddrs[i];
    }
    for(int i=id;i<(numRegisters-1);i++){
        _ipAddrs[i] = ipAddrs[i+1];
        _macAddrs[i] = macAddrs[i+1];
    }

    //Delete old arrays
    delete[] ipAddrs[id];
    delete[] ipAddrs;
    delete[] macAddrs;

    //Renew class arrays
    this->ipAddrs = _ipAddrs;
    this->macAddrs = _macAddrs;
    if(verbose) cout<<"[MacAddressTable] Entry successfully delete"<<endl;

    //Decrement number of registers
    numRegisters--;
}

uint8_t 
MacAddressTable::getMacAddress(
    uint8_t* ipAddr)    //Entry IP Address
{
    bool flag;      //Flag to verify if entry was found
    for(int i=0;i<numRegisters;i++){
        flag = true;
        for(int j=0;j<4;j++){
            if(ipAddrs[i][j]!=ipAddr[j])
                flag = false;
        }
        if(flag)
            return macAddrs[i];
    }
    return -1;
}

uint8_t MacAddressTable::getMacAddress(
    int id)     //Entry identification
{
    if(id>=numRegisters) return -1;
    return macAddrs[id];
}

uint8_t* MacAddressTable::getIpAddress(
    uint8_t macAddr)    //Entry 5GR Mac Address
{
    for(int i=0;i<numRegisters;i++){
        if(macAddrs[i]==macAddr)
            return ipAddrs[i];
    }
    return NULL;
}

uint8_t* MacAddressTable::getIpAddress(
    int id)     //Entry Identification
{
    if(id>=numRegisters) return 0;
    return ipAddrs[id];
}