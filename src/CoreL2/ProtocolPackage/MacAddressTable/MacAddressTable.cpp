/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "MacAddressTable.h"

using namespace std;

/**
 * @brief Constructs empty table with no verbosity
 */
MacAddressTable::MacAddressTable(){
    MacAddressTable(false);
}

/**
 * @brief Constructs empty table with verbosity passed as parameter
 * @param _verbose Verbosity flag
 */
MacAddressTable::MacAddressTable(bool _verbose){
    this->verbose = _verbose;
    numRegs = 0;
    if(verbose) cout<<"[MacAddressTable] Created Mac Address Table"<<endl;
}

/**
 * @brief Destroys MacAddressTable
 */
MacAddressTable::~MacAddressTable(){
    while(numRegs>0){
        deleteEntry(0);
    }
}

/**
 * @brief Gets the total number of registers in table
 * @returns Number of current registers on table
 */
int 
MacAddressTable::getNumRegs(){
    return numRegs;
}

/**
 * @brief Prints MacAddressTable Table: ID, IP Address and MAC Address
 */
void 
MacAddressTable::printMacTable(){
    cout<<"ID \t IP \t\t MAC"<<endl;
    for(int i=0;i<numRegs;i++){
        cout<<i<<"\t"<<(int)ipAddrs[i][0]<<"."<<(int)ipAddrs[i][1]<<"."<<(int)ipAddrs[i][2]<<"."<<(int)ipAddrs[i][3]<<"."<<"\t"<<(int)macAddrs[i]<<endl;
    }
}

/**
 * @brief Adds a new entry to the table
 * @param ipAddress IP Address
 * @param macAddress Corresponding MAC Address
 */
void 
MacAddressTable::addEntry(uint8_t* ipAddress, uint8_t macAddress){
    //Relocate arrays
    uint8_t** _ipAddrs = new uint8_t*[numRegs+1];
    uint8_t* _macAddrs = new uint8_t[numRegs+1];

    //Copy old information
    for(int i=0;i<numRegs;i++){
        _ipAddrs[i] = ipAddrs[i];
        _macAddrs[i] = macAddrs[i];
    }

    //Add new information
    _ipAddrs[numRegs] = ipAddress;
    _macAddrs[numRegs] = macAddress;

    //Delete old arrays
    delete[] ipAddrs;
    delete[] macAddrs;

    //Renew class arrays
    this->ipAddrs = _ipAddrs;
    this->macAddrs = _macAddrs;
    if(verbose) cout<<"[MacAddressTable] Entry added"<<endl;

    //Increment number of registers
    numRegs++;
}

/**
 * @brief Deletes the entry which ID is passed as parameter
 * @param id Identification of register
 */
void 
MacAddressTable::deleteEntry(int id){
    //Verify ID. ID is sequential
    if(id>(numRegs-1)){
        if(verbose) cout<<"[MacAddressTable] Invalid ID"<<endl;
        return;
    }

    //Relocate arrays
    uint8_t** _ipAddrs = new uint8_t*[numRegs-1];
    uint8_t* _macAddrs = new uint8_t[numRegs-1];

    //Copy information
    for(int i=0;i<id;i++){
        _ipAddrs[i] = ipAddrs[i];
        _macAddrs[i] = macAddrs[i];
    }
    for(int i=id;i<(numRegs-1);i++){
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
    numRegs--;
}

/**
 * @brief Gets MAC Address from table given IP Address
 * @param ipAddr IP Address
 * @returns Corresponding MAC Address; -1 if entry is not found
 */
uint8_t 
MacAddressTable::getMacAddress(uint8_t* ipAddr){
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

/**
 * @brief Gets MAC Address from table given ID
 * @param id Entry identification
 * @returns Corresponding MAC Address; -1 if entry is not found
 */
uint8_t MacAddressTable::getMacAddress(int id){
    if(id>=numRegs) return -1;
    return macAddrs[id];
}

/**
 * @brief Gets IP Address from table given MAC Address
 * @param macAddr MAC Address
 * @returns Corresponding IP Address; 0 if entry is not found
 */
uint8_t* MacAddressTable::getIpAddress(uint8_t macAddr){
    for(int i=0;i<numRegs;i++){
        if(macAddrs[i]==macAddr)
            return ipAddrs[i];
    }
    return NULL;
}

/**
 * @brief Gets IP Address from table given ID
 * @param id Entry identification
 * @returns Corresponding IP Address; 0 if entry is not found
 */
uint8_t* MacAddressTable::getIpAddress(int id){
    if(id>=numRegs) return 0;
    return ipAddrs[id];
}