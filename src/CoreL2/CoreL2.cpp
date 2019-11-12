/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

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
    int* ports;             //Array of ports that will be used on sockets
    int nEquipments;        //Number of attached equipments
    bool verbose = false;   //Verbosity flag
    char *devname = NULL;   //Tun interface name
    uint16_t nB;            //Maximum number of bytes in PDU

    //Print message if wrong usage of command line
    if(argc<2){
        cout<<"Usage: sudo ./a.out nEquipments ip1 port1 ... ipN portN MaxNBytes MacAddr [--v] [devname]"<<endl;
        exit(1);
    }

    //First argument: nEquipments (considering 9 as maximum)
    nEquipments = argv[1][0] - 48;  //Converting char to int

    //Verbosity and devName arguments 
    if(argc==(2+nEquipments*2+3)){
        if(argv[2+nEquipments*2+2][0]=='-')
            verbose = true;
        else devname = argv[2+nEquipments*2+2];
    }
    else if(argc==(2+nEquipments*2+4)){
        verbose = true;
        devname = argv[2+nEquipments*2+3];
    }

    //Creates a new L1 empty object
    CoreL1* l1 = new CoreL1(verbose);

    //Allocates ports array, get ports and IP addresses from command line and adds one socket for each port
    ports = new int[nEquipments];
    for(int i=0;i<nEquipments;i++){
        ports[i] = (int) strtol(argv[2+i*nEquipments+1], NULL, 10);
        l1->addSocket(argv[2+i*nEquipments], ports[i]);
    }

    //Creates and initializes a MacAddressTable with static informations
    MacAddressTable* arp = new MacAddressTable(verbose);
    uint8_t addressEntry0[4] = {10,0,0,10};
    uint8_t addressEntry1[4] = {10,0,0,11};
    uint8_t addressEntry2[4] = {10,0,0,12};
    arp->addEntry(addressEntry0, 0);
    arp->addEntry(addressEntry1, 1);
    arp->addEntry(addressEntry2, 2);
    
    //Get nB from command line
    nB = (uint16_t) strtol(argv[2+nEquipments*2], NULL, 10);

    //Create a new MacController object
    MacController equipment(nEquipments, (uint16_t) nB, devname, arp, (int) argv[2+nEquipments*2+1][0] - 48, l1, verbose);
    
    //Finnally start threads
    equipment.startThreads();

    delete ports;
    delete arp;
    delete l1;
}
