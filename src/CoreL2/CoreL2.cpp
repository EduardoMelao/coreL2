/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : CoreL2.cpp
@Classification : MAC Layer
@
@Last alteration : November 21st, 2019
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : Medium Access Control (MAC) Layer main module for PoC in project H2020 5G-RANGE. 
    This module initializes the other modules of 5G-RANGE MAC System.
*/

#include <iostream>
#include <stdlib.h> //strtol
#include <thread>
using namespace std;

//Custom headers implemented
#include "ProtocolPackage/ProtocolPackage.h"
#include "ProtocolPackage/MacAddressTable/MacAddressTable.h"
#include "../CoreL1/CoreL1.h"
#include "MacController/MacController.h"

int main(int argc, char** argv){
    int* ports;                     //Array of ports that will be used on sockets
    uint8_t* macAddresses;          //Array of 5GR MAC Addresses of attached equipments
    int numberEquipments;           //Number of attached equipments
    int argumentsOffset;			//Arguments interpretation offset
    bool verbose = false;           //Verbosity flag
    char *devname = NULL;           //Tun interface name
    uint16_t maxNumberBytes;        //Maximum number of bytes in PDU
    bool flagBS;                    //Base Station flag: true if BS, false if UE

    //Print message if wrong usage of command line
    if(argc<2){
        cout<<"Usage BS: sudo ./a.out 0(BS) numberEquipments ip1 port1 UE_macAddr1 ... ipN portN UE_macAddrN MaxNBytes BS_macAddr [--v] [devname]"<<endl;
        cout<<"Usage UE: sudo ./a.out 1(UE) BS_ip BS_port BS_macAddr MaxNBytes UE_macAddr [--v] [devname]"<<endl;
        exit(1);
    }

    //Verifying Base Station flag
    flagBS = (argv[1][0] == '0');

    //Atributing value to numberEquipments
    numberEquipments = flagBS? (argv[2][0] - 48):1;

    //Defining arguments offset
    argumentsOffset = flagBS? 3:2;

    //Verbosity and devName arguments 
    if(argc==(argumentsOffset+numberEquipments*3+3)){
        if(argv[argumentsOffset+numberEquipments*3+2][0]=='-')
            verbose = true;
        else devname = argv[argumentsOffset+numberEquipments*3+2];
    }
    else if(argc==(argumentsOffset+numberEquipments*3+4)){
        verbose = true;
        devname = argv[argumentsOffset+numberEquipments*3+3];
    }

    //Creates a new L1 empty object
    CoreL1* l1 = new CoreL1(verbose);

    //Allocates ports array, get ports, MAC addresses and IP addresses from command line and adds one socket for each port
    ports = new int[numberEquipments];
    macAddresses = new uint8_t[numberEquipments];
    for(int i=0;i<numberEquipments;i++){
        ports[i] = (int) strtol(argv[argumentsOffset+i*3+1], NULL, 10);
        macAddresses[i] = (uint8_t) strtol(argv[argumentsOffset+i*3+2], NULL, 10);
        l1->addSocket(argv[argumentsOffset+i*3], ports[i]);
    }

    //Creates and initializes a MacAddressTable with static informations
    MacAddressTable* ipMacTable = new MacAddressTable(verbose);
    uint8_t addressEntry0[4] = {10,0,0,10};
    uint8_t addressEntry1[4] = {10,0,0,11};
    uint8_t addressEntry2[4] = {10,0,0,12};
    ipMacTable->addEntry(addressEntry0, 0, true);
    ipMacTable->addEntry(addressEntry1, 1, false);
    ipMacTable->addEntry(addressEntry2, 2, false);
    
    //Get maxNumberBytes from command line
    maxNumberBytes = (uint16_t) strtol(argv[argumentsOffset+numberEquipments*3], NULL, 10);

    //Create a new MacController object
    MacController equipment(numberEquipments, macAddresses, (uint16_t) maxNumberBytes, devname, ipMacTable, (int) argv[argumentsOffset+numberEquipments*3+1][0] - 48, l1, verbose);
    
    //Finnally start threads
    equipment.startThreads();

    delete [] ports;
    delete ipMacTable;
    delete l1;
    delete [] macAddresses;
}
