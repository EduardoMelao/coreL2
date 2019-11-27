/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacAddressTable.cpp
@Classification : MAC Address Table
@
@Last alteration : November 27th, 2019
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This is a support module to associate Linux L3 IP addresses to MAC 5G-Range MAC addresses. 
*/

#include "MacAddressTable.h"

using namespace std;

MacAddressTable::MacAddressTable(){
    MacAddressTable(false);
}

MacAddressTable::MacAddressTable(
    bool _verbose)  //Verbosity flag
{
    this->verbose = _verbose;
    numberRegisters = 0;
    if(verbose) cout<<"[MacAddressTable] Created Mac Address Table"<<endl;
}

MacAddressTable::~MacAddressTable(){
    while(numberRegisters>0){
        deleteEntry(0);
    }
}

int 
MacAddressTable::getNumberRegisters(){
    return numberRegisters;
}

void 
MacAddressTable::printMacTable(){
    cout<<"ID \t IP \t\t MAC"<<endl;
    for(int i=0;i<numberRegisters;i++){
        cout<<i<<"\t"<<(int)ipAddresses[i][0]<<"."<<(int)ipAddresses[i][1]<<"."<<(int)ipAddresses[i][2]<<"."<<(int)ipAddresses[i][3]<<"."<<"\t"<<(int)macAddresses[i]<<endl;
    }
}

void 
MacAddressTable::addEntry(
    uint8_t* ipAddress,     //Entry IP Address
    uint8_t macAddress,     //Entry 5GR MAC Address
    bool flagBS)            //Entry flag indicating if it is a Base Station or not
{
    //Relocate arrays
    uint8_t** _ipAddresses = new uint8_t*[numberRegisters+1];
    uint8_t* _macAddresses = new uint8_t[numberRegisters+1];
    bool* _flagsBS = new bool[numberRegisters+1];

    //Copy old information
    for(int i=0;i<numberRegisters;i++){
        _ipAddresses[i] = ipAddresses[i];
        _macAddresses[i] = macAddresses[i];
        _flagsBS[i] = flagsBS[i];
    }

    //Add new information
    _ipAddresses[numberRegisters] = ipAddress;
    _macAddresses[numberRegisters] = macAddress;
    _flagsBS[numberRegisters] = flagBS;

    //Delete old arrays, if they exist
    if(numberRegisters){
        delete[] ipAddresses;
        delete[] macAddresses;
        delete[] flagsBS;
    }

    //Renew class arrays
    this->ipAddresses = _ipAddresses;
    this->macAddresses = _macAddresses;
    this->flagsBS = _flagsBS;
    if(verbose) cout<<"[MacAddressTable] Entry added"<<endl;

    //Increment number of registers
    numberRegisters++;
}

void 
MacAddressTable::deleteEntry(
    int id)     //Identification of the entry
{
    //Verify ID. ID is sequential
    if(id>(numberRegisters-1)){
        if(verbose) cout<<"[MacAddressTable] Invalid ID"<<endl;
        return;
    }

    //Relocate arrays
    uint8_t** _ipAddresses = new uint8_t*[numberRegisters-1];
    uint8_t* _macAddresses = new uint8_t[numberRegisters-1];

    //Copy information
    for(int i=0;i<id;i++){
        _ipAddresses[i] = ipAddresses[i];
        _macAddresses[i] = macAddresses[i];
    }
    for(int i=id;i<(numberRegisters-1);i++){
        _ipAddresses[i] = ipAddresses[i+1];
        _macAddresses[i] = macAddresses[i+1];
    }

    //Delete old arrays
    delete[] ipAddresses[id];
    delete[] ipAddresses;
    delete[] macAddresses;

    //Renew class arrays
    this->ipAddresses = _ipAddresses;
    this->macAddresses = _macAddresses;
    if(verbose) cout<<"[MacAddressTable] Entry successfully delete"<<endl;

    //Decrement number of registers
    numberRegisters--;
}

uint8_t 
MacAddressTable::getMacAddress(
    uint8_t* ipAddr)    //Entry IP Address
{
    bool flag;      //Flag to verify if entry was found
    for(int i=0;i<numberRegisters;i++){
        flag = true;
        for(int j=0;j<4;j++){
            if(ipAddresses[i][j]!=ipAddr[j])
                flag = false;
        }
        if(flag)
            return macAddresses[i];
    }
    return -1;
}

uint8_t MacAddressTable::getMacAddress(
    int id)     //Entry identification
{
    if(id>=numberRegisters) return -1;
    return macAddresses[id];
}

uint8_t* MacAddressTable::getIpAddress(
    uint8_t macAddr)    //Entry 5GR Mac Address
{
    for(int i=0;i<numberRegisters;i++){
        if(macAddresses[i]==macAddr)
            return ipAddresses[i];
    }
    return NULL;
}

uint8_t* MacAddressTable::getIpAddress(
    int id)     //Entry Identification
{
    if(id>=numberRegisters) return 0;
    return ipAddresses[id];
}

bool MacAddressTable::getFlagBS(
    uint8_t mac)
{
    for(int i=0;i<numberRegisters;i++){
        if(macAddresses[i]==mac)
            return flagsBS[i];
    }
    if(verbose) cout<<"[MacAddressTable] Entry not found for flagBS."<<endl;
    return false;
}