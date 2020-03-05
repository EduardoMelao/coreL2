/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : Scheduler.cpp
@Classification : Scheduler
@
@Last alteration : March 5th, 2020
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module is responsible for Scheduling SDUs into MAC PDUs
            to be transmitted in the next subframe.
*/

#include "Scheduler.h"

Scheduler::Scheduler(
    CurrentParameters* _currentParameters,  //System Current execution parameters
    SduBuffers* _sduBuffers,                //Buffers responsible to store Control and Data SDUs
    bool _verbose)                          //Verbosity flag
{
    //Assign values do class variables
    currentParameters = _currentParameters;
    sduBuffers = _sduBuffers;
    verbose = _verbose;
}

Scheduler::~Scheduler() {}

vector<int>
Scheduler::selectUEs()
{
    vector<int> indexUEs;   //Array of indexes of UEs, each one referring to Current Parameters arrays UE position

    //Verify all UEs for data for transmission
    for(int i=0;i<currentParameters->getNumberUEs();i++){
        if(sduBuffers->bufferStatusInformation(currentParameters->getMacAddress(i)))
            indexUEs.push_back(i);
    }

    return indexUEs;
}

void
Scheduler::scheduleRequest(
    MacPDU** macPDUs)   //Array of MAC PDUs where scheduler will store Multiplexed SDUs
{
    //Define variables
    vector<int> indexUEs;   //Array of indexes of UEs, each one referring to Current Parameters arrays UE position
    int numberTotalRBs = 0; //Total Number of RBs for transmission
    uint8_t fusionLutValue; //Fusion Lookup Table value: array of 4 least significat bits

    //Check buffer status information & select UEs for scheduling
    indexUEs = selectUEs();
    if(verbose) cout<<"[Scheduler] Selected "<<indexUEs.size()<<" UEs for next transmission."<<endl;

    //Read Fusion Lookup Tables and obtain Total Number of RBs for transmission (NumTRBs)
    fusionLutValue = currentParameters->getFLUTMatrix();
    for(int i=0;i<4;i++){       //For each bit
        if((fusionLutValue>>i)==1){ //If channel is IDLE
            numberTotalRBs+=33;         //Adds 33 RBs to Total number of RBs available
        }
    }

    /*  #TODO:
        1. inicializar macPDUs
    */
    
    //For each UE (each PDU?)
    for(int i=0;i<indexUEs.size();i++){
        /*  #TODO:
        2. Inicializar um MAC PDU para cada macPDU[i] 
        3. calcular numero RBs para cada UE;
        4. Preencher informacao de alocacao em cada PDU com num RBs calculado, mimo, tudo que tiver
        5. lib5grange::get_bit_capacity(const size_t & numID,const allocation_cfg_t & allocation,const mimo_cfg_t & mimo,const qammod_t & mod) e colocar num array
        6. Criar mux Multiplexer() (AggregationQueue)
        7. verificar sdus de controle, depois de dados e ir adicionando;
        8. Preencher informacao de alocacao em cada PDU com num RBs calculado
    */
    }
}