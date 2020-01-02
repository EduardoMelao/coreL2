/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : CoreL1.cpp
@Classification : Core L1 [STUB]
@
@Last alteration : December 13th, 2019
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : Stub PHY Layer main module for tests about MAC Layer.
*/

#include <iostream>
#include <thread>
using namespace std;

#define PORT_TO_L2 8091
#define PORT_FROM_L2 8090

#include "StubPHYLayer.h"

int main(int argc, char** argv){
    int numberEquipments;           //Number of attached equipments
    int counter = 0;                //Counter to help assigning threads
    bool verbose = false;
    if(argc<4){
        cout<<"Usage: ./a.out numberEquipments ip1 port1 mac1 ... ipN portN macN --v"<<endl;
        exit(1);
    }

    numberEquipments = argv[1][0]-48;

    verbose = (argc==2+numberEquipments*3+1);
    CoreL1* l1 = new CoreL1(verbose);   //Create object CoreL1 with no verbose

    for(int i=0;i<numberEquipments;i++)
        l1->addSocket(argv[2+3*i], strtol(argv[2+3*i+1], NULL, 10), argv[2+3*i+2][0]-48);

    l1->startThreads();     //Start all threads

    delete l1;
}