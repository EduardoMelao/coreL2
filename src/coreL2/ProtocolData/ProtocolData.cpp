/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : ProtocolData.cpp
@Classification : Protocol Data
@
@Last alteration : November 29th, 2019
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
		MacHighQueue* _macHigh)				//Queue with MACD SDUs to be transmitted
{
    macHigh = _macHigh;
    macController = _macController;
}

ProtocolData::~ProtocolData() {}

void 
ProtocolData::enqueueDataSdus(){
    int macSendingPDU;              //This auxiliar variable will store MAC Address if queue is full of SDUs
    char bufferData[MAXLINE];       //Buffer to store Data Bytes
    ssize_t numberBytesRead = 0;    //Size of MACD SDU read
    
    //Infinite loop
    while(1){

        //Test if MacHigh Queue is not empty, i.e. there are SDUs to enqueue
        if(macHigh->getNumberPackets()){

            //Fulfill bufferData with zeros 
            bzero(bufferData, MAXLINE);

            //Gets next SDU from MACHigh Queue
            numberBytesRead = macHigh->getNextSdu(bufferData);

            //If multiplexer queue is empty, notify condition variable to trigger timeout timer
            for(int i=0;i<macController->attachedEquipments;i++){
                if(macController->macAddressEquipments[i]==macController->mux->getMacAddress(bufferData)){
                    if(macController->mux->emptyPdu(macController->macAddressEquipments[i])){
                        macController->queueConditionVariables[i].notify_all();
                    }
                    else break;
                }
            }

            {   
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
}
