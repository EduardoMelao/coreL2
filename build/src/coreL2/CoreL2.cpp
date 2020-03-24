/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : CoreL2.cpp
@Classification : MAC Layer
@
@Last alteration : March 24th, 2020
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
#include "MacController/MacController.h"

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

    //Retrieve number of cores 
    unsigned int numberCores = thread::hardware_concurrency();

    //Start stub to replace CLI
    thread t0(stubCLI, ref(equipment));

    //Create CPUSET
    cpu_set_t cpuSet;

    //Assign core 0 to StubCLI thread
    CPU_ZERO(&cpuSet);
    CPU_SET(0,&cpuSet);
    int errorCheck = pthread_setaffinity_np(t0.native_handle(), sizeof(cpuSet), &cpuSet);
    if(errorCheck!=0){
        if(verbose) cout<<"[CoreL2] Error assigning thread StubCLI to CPU 0."<<endl;
    }
    else
        if(verbose) cout<<"[CoreL2] Thread StubCLI assigned to CPU 0."<<endl;
    
    //Detach StubCLI thread
    t0.detach();

    //Create thread for MacController::manager()
    thread t1(&MacController::initialize, &equipment);

    //Assign core 1%numberCores to MacController::manager()
    CPU_ZERO(&cpuSet);
    CPU_SET(1%numberCores,&cpuSet);
    errorCheck = pthread_setaffinity_np(t1.native_handle(), sizeof(cpuSet), &cpuSet);
    if(errorCheck!=0){
        if(verbose) cout<<"[CoreL2] Error assigning thread MacController::manager() to CPU 1."<<endl;
    }
    else
        if(verbose) cout<<"[CoreL2] Thread MacController::manager() assigned to CPU 1."<<endl;
    
    //Finally, join second thread
    t1.join();

    return 0;
}