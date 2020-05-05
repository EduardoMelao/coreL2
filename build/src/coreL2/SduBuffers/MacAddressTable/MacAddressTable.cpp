/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacAddressTable.cpp
@Classification : MAC Address Table
@
@Last alteration : May 5th, 2020
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

MacAddressTable::MacAddressTable(){
    MacAddressTable(false);
}

MacAddressTable::MacAddressTable(
    bool _verbose)  //Verbosity flag
{
    this->verbose = _verbose;
    if(verbose) cout<<"[MacAddressTable] Created Mac Address Table"<<endl;
}

MacAddressTable::~MacAddressTable() { 
    while(macAddresses.size()>0)
        deleteEntry(0);
}

int 
MacAddressTable::getNumberRegisters(){
    return macAddresses.size();
}

void 
MacAddressTable::printMacTable(){
    cout<<"ID \t IP \t\t MAC"<<endl;
    for(int i=0;i<macAddresses.size();i++){
        cout<<i<<"\t"<<(int)ipAddresses[i][0]<<"."<<(int)ipAddresses[i][1]<<"."<<(int)ipAddresses[i][2]<<"."<<(int)ipAddresses[i][3]<<"."<<"\t"<<(int)macAddresses[i]<<endl;
    }
}

void 
MacAddressTable::addEntry(
    uint8_t* ipAddress,     //Entry IP Address
    uint8_t macAddress)     //Entry 5GR MAC Address)
{
    uint8_t* newIpAddress = new uint8_t[4];  //Ip Address entry to be added

    //Resize mac Addresses vector and copy IP Address
    for(int i=0;i<4;i++)
        newIpAddress[i]=ipAddress[i];

    //Push back information
    macAddresses.push_back(macAddress);
    ipAddresses.push_back(newIpAddress);
}

void 
MacAddressTable::deleteEntry(
    int id)     //Identification of the entry
{
    if(id<macAddresses.size()){
        macAddresses.erase(macAddresses.begin()+id);
        delete [] ipAddresses[id];
        ipAddresses.erase(ipAddresses.begin()+id);
        if(verbose) cout<<"[MacAddressTable] Entry successfully deleted"<<endl;
    }
}

uint8_t 
MacAddressTable::getMacAddress(
    uint8_t* ipAddr)    //Entry IP Address
{
    bool flag;      //Flag to verify if entry was found
    for(int i=0;i<macAddresses.size();i++){
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

uint8_t* MacAddressTable::getIpAddress(
    uint8_t macAddr)    //Entry 5GR Mac Address
{
    for(int i=0;i<ipAddresses.size();i++){
        if(macAddresses[i]==macAddr)
            return ipAddresses[i];
    }
    return NULL;
}