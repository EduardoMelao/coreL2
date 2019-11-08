#include "MacAddressTable.h"

using namespace std;

MacAddressTable::MacAddressTable(){
    MacAddressTable(false);
}

MacAddressTable::MacAddressTable(bool _verbose){
    this->verbose = _verbose;
    numRegs = 0;
    if(verbose) cout<<"[MacAddressTable] Created Mac Address Table"<<endl;
}

MacAddressTable::~MacAddressTable(){
    while(numRegs>0){
        deleteEntry(0);
    }
}

//Returns number of current registers on table.
int MacAddressTable::getNumRegs(){
    return numRegs;
}

//Prints ARP Table: ID, IP Address and MAC Address.
void MacAddressTable::printMacTable(){
    cout<<"ID \t IP \t\t MAC"<<endl;
    for(int i=0;i<numRegs;i++){
        cout<<i<<"\t"<<(int)ipAddrs[i][0]<<"."<<(int)ipAddrs[i][1]<<"."<<(int)ipAddrs[i][2]<<"."<<(int)ipAddrs[i][3]<<"."<<"\t"<<(int)macAddrs[i]<<endl;
    }
}

//Adds Entry to table.
void MacAddressTable::addEntry(uint8_t* ipAddress, uint8_t macAddress){
    uint8_t** _ipAddrs = new uint8_t*[numRegs+1];
    uint8_t* _macAddrs = new uint8_t[numRegs+1];
    for(int i=0;i<numRegs;i++){
        _ipAddrs[i] = ipAddrs[i];
        _macAddrs[i] = macAddrs[i];
    }
    _ipAddrs[numRegs] = ipAddress;
    _macAddrs[numRegs] = macAddress;
    delete[] ipAddrs;
    delete[] macAddrs;
    this->ipAddrs = _ipAddrs;
    this->macAddrs = _macAddrs;
    if(verbose) cout<<"[MacAddressTable] Entry added"<<endl;
    numRegs++;
}

//Deletes the entry which ID is passed as parameter.
void MacAddressTable::deleteEntry(int id){
    if(id>(numRegs-1)){
        if(verbose) cout<<"[MacAddressTable] Invalid ID"<<endl;
        return;
    }
    uint8_t** _ipAddrs = new uint8_t*[numRegs-1];
    uint8_t* _macAddrs = new uint8_t[numRegs-1];
    for(int i=0;i<id;i++){
        _ipAddrs[i] = ipAddrs[i];
        _macAddrs[i] = macAddrs[i];
    }
    for(int i=id;i<(numRegs-1);i++){
        _ipAddrs[i] = ipAddrs[i+1];
        _macAddrs[i] = macAddrs[i+1];
    }
    delete[] ipAddrs[id];
    delete[] ipAddrs;
    delete[] macAddrs;
    this->ipAddrs = _ipAddrs;
    this->macAddrs = _macAddrs;
    if(verbose) cout<<"[MacAddressTable] Entry successfully delete"<<endl;
    numRegs--;
}

//Get MAC Address from table. Returns -1 if entry is not found.
uint8_t MacAddressTable::getMacAddress(uint8_t* ipAddr){
    bool flag;
    for(int i=0;i<numRegs;i++){
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

//Get MAC Address from table. Returns -1 if entry is not found.
uint8_t MacAddressTable::getMacAddress(int id){
    if(id>=numRegs) return -1;
    return macAddrs[id];
}

//Get IP Address from table. Returns 0 if entry is not found.
uint8_t* MacAddressTable::getIpAddress(uint8_t macAddr){
    for(int i=0;i<numRegs;i++){
        if(macAddrs[i]==macAddr)
            return ipAddrs[i];
    }
    return NULL;
}

//Get IP Address from table. Returns 0 if entry is not found.
uint8_t* MacAddressTable::getIpAddress(int id){
    if(id>=numRegs) return 0;
    return ipAddrs[id];
}