/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : CoreL2.cpp
@Classification : MAC Layer
@
@Last alteration : February 18th, 2020
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : Medium Access Control (MAC) Layer main module for PoC in project H2020 5G-RANGE. 
    This module instantiates and initializes 5G-RANGE MAC System manager.
*/

#include <iostream>
using namespace std;

//Custom headers implemented
#include "ProtocolPackage/ProtocolPackage.h"
#include "SduBuffers/MacAddressTable/MacAddressTable.h"
#include "MacController/MacController.h"
#include "SystemParameters/CurrentParameters.h"

void stubCLI(MacController & macController){
    while(1){
        char caracter;
        cout<<"Press + for MacStart, / for MacStop and * for MacConfigRequest"<<endl;
        cin>>caracter;
        while(caracter!='+'&&caracter!='/'&&caracter!='*'){
            cin>>caracter;
        }

        switch(caracter){
            case '+':
            {
                macController.cliL2Interface->macStartCommand();
                break;
            }
            case '/':
            {
                macController.cliL2Interface->macStopCommand();
                break;
            }
            case '*':
            {
                macController.cliL2Interface->macConfigRequestCommand();
                break;
            }
            default:
            break;
        }
    }
}

int main(int argc, char** argv){
    bool verbose = false;           //Verbosity flag
    char *devname = NULL;           //Tun interface name

	//Verify verbose
    if(argc==2){
        if(argv[1][0]=='-') verbose = true;
        else devname = argv[1];
    }
    if(argc==3){
        verbose = true;
        devname = argv[2];
    }

    //Create a new MacController (main module) object
    MacController equipment(devname, verbose);

    //Start stub to replace CLI
    thread t1(stubCLI, ref(equipment));
    t1.detach();

    //Finally, start threads
    equipment.initialize();
}
