#include <iostream>
#include <stdlib.h> //strtol
#include <thread>
using namespace std;

//Custom headers implemented
#include "ProtocolPackage/ProtocolPackage.h"
#include "ProtocolPackage/MacAddressTable/MacAddressTable.h"
#include "../CoreL1/CoreL1.h"
#include "MacController/MacController.h"

//Definitions about the socket UDP that simulates L1

int main(int argc, char** argv){
    int* ports;
    int nEquipments;
    bool verbose = false;
    char *devname = NULL;
    uint16_t nB;

    if(argc<2){
        cout<<"Usage: sudo ./a.out nEquipments ip1 port1 ... ipN portN MaxNBytes MacAddr [--v] [devname]"<<endl;
        exit(1);
    }

    nEquipments = argv[1][0] - 48;  //Converting char to int
    if(argc==(2+nEquipments*2+3)){
        if(argv[2+nEquipments*2+2][0]=='-')
            verbose = true;
        else devname = argv[2+nEquipments*2+2];
    }
    else if(argc==(2+nEquipments*2+4)){
        verbose = true;
        devname = argv[2+nEquipments*2+3];
    }

    CoreL1* l1 = new CoreL1(verbose);
    ports = new int[nEquipments];
    for(int i=0;i<nEquipments;i++){
        ports[i] = (int) strtol(argv[2+i*nEquipments+1], NULL, 10);
        l1->addSocket(argv[2+i*nEquipments], ports[i]);
    }

    MacAddressTable* arp = new MacAddressTable(verbose);
    uint8_t addressEntry0[4] = {10,0,0,10};
    uint8_t addressEntry1[4] = {10,0,0,11};
    uint8_t addressEntry2[4] = {10,0,0,12};
    arp->addEntry(addressEntry0, 0);
    arp->addEntry(addressEntry1, 1);
    arp->addEntry(addressEntry2, 2);
    
    nB = (uint16_t) strtol(argv[2+nEquipments*2], NULL, 10);
    MacController equipment(nEquipments, (uint16_t) nB, devname, arp, (int) argv[2+nEquipments*2+1][0] - 48, l1, verbose);
    
    equipment.startThreads();

    delete ports;
    delete arp;
    delete l1;
}
