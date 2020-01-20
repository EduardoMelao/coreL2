/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : ProtocolControl.cpp
@Classification : Protocol Control
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

@Description : This module controls MACC SDUS enqueueing to transmission.
*/

#include "ProtocolControl.h"

ProtocolControl::ProtocolControl(
		MacController* _macController, 		//Object that contains all information about mutexes, condition variables and the queue to transmission
        bool _verbose)                      //Verbosity flag
{
    macController = _macController;
    macControlqueue = new MacCQueue();
    verbose = _verbose;
}

ProtocolControl::~ProtocolControl() {
    delete macControlqueue;
}

void
ProtocolControl::enqueueControlSdus(){
    int macSendingPDU;                      //Auxiliary variable to store MAC Address if queue is full of SDUs to transmit
    char bufControl[MAXIMUM_BUFFER_LENGTH]; //Buffer to store Control Bytes

    ssize_t numberBytesRead = 0;    //Size of MACC SDU read in Bytes

    //Inifinite loop
    while (1){
        //If multiplexing queue is empty, notify condition variable to trigger timeout timer
        for(int i=0;i<macController->attachedEquipments;i++)
            if(macController->mux->emptyPdu(macController->macAddressEquipments[i])) 
                macController->queueConditionVariables[i].notify_all();

        //Fulfill bufferControl with zeros        
        bzero(bufControl, MAXIMUM_BUFFER_LENGTH);

        //Test if it is BS or UE and decides which Control SDU to get
        numberBytesRead = macController->flagBS? macControlqueue->getControlSduCSI(bufControl): macControlqueue->getControlSduULMCS(bufControl);
        {   
            //Locks mutex to write in multiplexer queue
            lock_guard<mutex> lk(macController->queueMutex);
            
            //Send control PDU to all attached equipments
            for(int i=0;i<macController->attachedEquipments;i++){
                //Adds SDU to multiplexer
                macSendingPDU = macController->mux->addSdu(bufControl, numberBytesRead, 0, macController->macAddressEquipments[i]);

                //If the SDU was added successfully, continues the loop
                if(macSendingPDU==-1)
                    continue;

                //Else, macSendingPDU contains the Transmission Queue MAC Address to perform PDU sending. 
                //So, perform PDU sending
                macController->sendPdu(macSendingPDU);

                //Now, it is possible to add SDU to queue
                macController->mux->addSdu(bufControl,numberBytesRead, 0,  macController->macAddressEquipments[i]);
            }
        }
    }
}

void 
ProtocolControl::decodeControlSdus(
    char* buffer,                   //Buffer containing Control SDU
    size_t numberDecodingBytes)     //Size of Control SDU in Bytes
{
    //PROVISIONAL: PRINT CONTROL SDU
    if(verbose){
        cout<<"[ProtocolControl] Control SDU received: ";
        for(int i=0;i<numberDecodingBytes;i++)
            cout<<buffer[i];
        cout<<endl;
    }

    if(!(macController->flagBS)){    //UE needs to return ACK to BS
        if(verbose) cout<<"[ProtocolControl] Returns ACK to BS..."<<endl;
        // ACK
        if(macController->mux->emptyPdu(macController->macAddressEquipments[0]))
        	macController->queueConditionVariables[0].notify_all();     //index 0: UE has only BS as equipment

        char ackBuffer[MAXIMUM_BUFFER_LENGTH];
        bzero(ackBuffer, MAXIMUM_BUFFER_LENGTH);

        size_t numberAckBytes = macControlqueue->getAck(ackBuffer);
        lock_guard<mutex> lk(macController->queueMutex);
        int macSendingPDU = macController->mux->addSdu(ackBuffer, numberAckBytes, 0,0);

        //If addSdu returns -1, SDU was added successfully
        if(macSendingPDU==-1) return;

        //Else, queue is full. Need to send PDU
        macController->sendPdu(macSendingPDU);

        macController->mux->addSdu(ackBuffer, numberAckBytes, 0,0);
    }    
}

void
ProtocolControl::sendInterlayerMessages(
    char* buffer,           //Buffer containing message
    size_t numberBytes)     //Size of message in Bytes
{
    macController->l1l2Interface->sendControlMessage(buffer, numberBytes);
}

void
ProtocolControl::receiveInterlayerMessages(){
    char buffer[MAXIMUM_BUFFER_LENGTH]; //Buffer where message will be stored
    string message;                     //String containing message converted from char*
    ssize_t messageSize = macController->l1l2Interface->receiveControlMessage(buffer, MAXIMUM_BUFFER_LENGTH);

    //Control message stream
    while(messageSize>0){

        //Manually convert char* to string ////////////////// PROVISIONAL: CONSIDERING message is transmitted alone (no parameters with it)
        for(int i=0;i<messageSize;i++)
            message+=buffer[i];

        if(message=="SubframeRx.Start"){
            if(verbose) cout<<"[StubPHYLayer] Received SubframeRx.Start message."<<endl;
            macController->decoding();
        }
        else if(message=="SubframeRx.End"){
            if(verbose) cout<<"[StubPHYLayer] Received SubframeRx.End message."<<endl;
        }

        //Clear buffer and message and receive next control message
        bzero(buffer, MAXIMUM_BUFFER_LENGTH);
        message.clear();
        messageSize = macController->l1l2Interface->receiveControlMessage(buffer, MAXIMUM_BUFFER_LENGTH);
    }
}