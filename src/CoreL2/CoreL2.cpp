/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : CoreL2.cpp
@Classification : MAC Layer
@
@Last alteration : November 19th, 2019
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
    int numberEquipments;           //Number of attached equipments
    bool verbose = false;           //Verbosity flag
    char *devname = NULL;           //Tun interface name
    uint16_t maxNumberBytes;        //Maximum number of bytes in PDU

    //Print message if wrong usage of command line
    if(argc<2){
        cout<<"Usage: sudo ./a.out numberEquipments ip1 port1 ... ipN portN MaxNBytes MacAddr [--v] [devname]"<<endl;
        exit(1);
    }

    //First argument: numberEquipments (considering 9 as maximum)
    numberEquipments = argv[1][0] - 48;  //Converting char to int

    //Verbosity and devName arguments 
    if(argc==(2+numberEquipments*2+3)){
        if(argv[2+numberEquipments*2+2][0]=='-')
            verbose = true;
        else devname = argv[2+numberEquipments*2+2];
    }
    else if(argc==(2+numberEquipments*2+4)){
        verbose = true;
        devname = argv[2+numberEquipments*2+3];
    }

    //Creates a new L1 empty object
    CoreL1* l1 = new CoreL1(verbose);

    //Allocates ports array, get ports and IP addresses from command line and adds one socket for each port
    ports = new int[numberEquipments];
    for(int i=0;i<numberEquipments;i++){
        ports[i] = (int) strtol(argv[2+i*numberEquipments+1], NULL, 10);
        l1->addSocket(argv[2+i*numberEquipments], ports[i]);
    }

    //Creates and initializes a MacAddressTable with static informations
    MacAddressTable* ipMacTable = new MacAddressTable(verbose);
    uint8_t addressEntry0[4] = {10,0,0,10};
    uint8_t addressEntry1[4] = {10,0,0,11};
    uint8_t addressEntry2[4] = {10,0,0,12};
    ipMacTable->addEntry(addressEntry0, 0);
    ipMacTable->addEntry(addressEntry1, 1);
    ipMacTable->addEntry(addressEntry2, 2);
    
    //Get maxNumberBytes from command line
    maxNumberBytes = (uint16_t) strtol(argv[2+numberEquipments*2], NULL, 10);

    //Create a new MacController object
    MacController equipment(numberEquipments, (uint16_t) maxNumberBytes, devname, ipMacTable, (int) argv[2+numberEquipments*2+1][0] - 48, l1, verbose);
    
    //Finnally start threads
    equipment.startThreads();

    delete ports;
    delete ipMacTable;
    delete l1;
}
