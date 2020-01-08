/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : ProtocolData.cpp
@Classification : Protocol Data
@
@Last alteration : January 8th, 2020
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module controls MACD SDUS enqueueing to transmission.
*/

#include "ProtocolData.h"

ProtocolData::ProtocolData(
    MacController* _macController, 		//Object that contains all information about mutexes, condition variables and the queue to transmission
    MacHighQueue* _macHigh,				//Queue with MACD SDUs to be transmitted
    bool _verbose)                      //Verbosity flag
{
    macHigh = _macHigh;
    macController = _macController;
    verbose = _verbose;
}

ProtocolData::~ProtocolData() {}

void 
ProtocolData::enqueueDataSdus(){
    int macSendingPDU;                      //This auxiliary variable will store MAC Address if queue is full of SDUs
    char bufferData[MAXIMUM_BUFFER_LENGTH]; //Buffer to store Data Bytes
    ssize_t numberBytesRead = 0;            //Size of MACD SDU read in Bytes
    
    //Infinite loop
    while(1){

        //Test if MacHigh Queue is not empty, i.e. there are SDUs to enqueue
        if(macHigh->getNumberPackets()){

            //Fulfill bufferData with zeros 
            bzero(bufferData, MAXIMUM_BUFFER_LENGTH);

            //Gets next SDU from MACHigh Queue
            numberBytesRead = macHigh->getNextSdu(bufferData);

            //If multiplexer queue is empty, notify condition variable to trigger timeout timer
            if(!macController->staticParameters->flagBS){    //If UE, test if its (unique) queue to BS is empty, then notify condition variables
                if(macController->mux->emptyPdu(0))
                    macController->queueConditionVariables[0].notify_all();
            }
            else{
                for(int i=0;i<macController->staticParameters->numberUEs;i++){
                    if(macController->staticParameters->ulReservations[i].target_ue_id==macController->mux->getMacAddress(bufferData)){
                        if(macController->mux->emptyPdu(macController->staticParameters->ulReservations[i].target_ue_id)){
                            macController->queueConditionVariables[i].notify_all();
                        }
                        else break;
                    }
                }
            }

            //Locks mutex to write in Multiplexer queue
            lock_guard<mutex> lk(macController->queueMutex);
            
            //Adds SDU to multiplexer
            macSendingPDU = macController->mux->addSdu(bufferData, numberBytesRead);

            //If the SDU was added successfully, continues the loop
            if(macSendingPDU==-1)
                continue;

            //Else, macSendingPDU contains the Transmission Queue destination MAC to perform PDU sending. 
            //So, perform PDU sending
            macController->sendPdu(macSendingPDU);

            //Now, it is possible to add SDU to queue
            macController->mux->addSdu(bufferData,numberBytesRead);
        }
    }
}

void
ProtocolData::decodeDataSdus(
    char* buffer,                   //Buffer containg Data SDU to decode
    size_t numberDecodingBytes)     //Size of Data SDU in bytes
{   
    if(verbose) cout<<"[ProtocolData] Data SDU received. Forwarding to L3."<<endl; 
    macController->transmissionProtocol->sendPackageToL3(buffer, numberDecodingBytes);
}