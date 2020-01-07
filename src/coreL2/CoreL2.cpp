/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : CoreL2.cpp
@Classification : MAC Layer
@
@Last alteration : January 7th, 2019
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
#include "Multiplexer/MacAddressTable/MacAddressTable.h"
#include "MacController/MacController.h"
#include "StaticDefaultParameters/StaticDefaultParameters.h"

int main(int argc, char** argv){
    uint8_t* macAddresses;          //Array of 5GR MAC Addresses of attached equipments
    int numberEquipments;           //Number of attached equipments
    int argumentsOffset;			//Arguments interpretation offset
    bool verbose = false;           //Verbosity flag
    char *devname = NULL;           //Tun interface name
    uint16_t maxNumberBytes;        //Maximum number of bytes in PDU
    bool flagBS;                    //Base Station flag: true if BS, false if UE

	//Verify verbose
    verbose = false;
    if(argc==2){
        if(argv[1][0]=='-') verbose = true;
        else devname = argv[1];
    }
    if(argc==3){
        verbose = true;
        devname = argv[2];
    }

    //Load static information on BS/UE and attributing value to numberEquipments
    StaticDefaultParameters* staticParameters = new StaticDefaultParameters(verbose);
    numberEquipments = staticParameters->numberUEs;
    flagBS = staticParameters->flagBS;

    macAddresses = new uint8_t[numberEquipments];

    if(flagBS){
        for(int i=0;i<numberEquipments;i++)
            macAddresses[i] = staticParameters->ulReservations[i].target_ue_id;
    }
    else macAddresses[0] = 0;   //BS is UE's "attached equipment"

    //Creates and initializes a MacAddressTable with static informations (HARDCODE)
    MacAddressTable* ipMacTable = new MacAddressTable(verbose);
    uint8_t addressEntry0[4] = {10,0,0,10};
    uint8_t addressEntry1[4] = {10,0,0,11};
    uint8_t addressEntry2[4] = {10,0,0,12};
    ipMacTable->addEntry(addressEntry0, 0, true);
    ipMacTable->addEntry(addressEntry1, 1, false);
    ipMacTable->addEntry(addressEntry2, 2, false);
    
    //Get maxNumberBytes from command line
    maxNumberBytes = staticParameters->mtu;

    //Create a new MacController object
    MacController equipment(numberEquipments, macAddresses, (uint16_t) maxNumberBytes, devname, ipMacTable, staticParameters, verbose);

    //Finally, start threads
    equipment.startThreads();

    delete ipMacTable;
    delete [] macAddresses;
    delete staticParameters;
}
