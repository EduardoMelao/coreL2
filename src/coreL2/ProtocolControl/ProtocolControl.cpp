/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : ProtocolControl.cpp
@Classification : Protocol Control
@
@Last alteration : January 9th, 2019
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
    verbose = _verbose;
}

ProtocolControl::~ProtocolControl() { }

void 
ProtocolControl::enqueueControlSdus(
    uint8_t* controlSdu,    //MAC Control SDU
    size_t numberBytes,     //Size of MACC SDU in Bytes
    uint8_t macAddress)     //Destination MAC Address
{
    //Find index to identify the queue referring to this UE
    int index;  //Index of destination UE in queues
    if(!macController->flagBS)      //If it is UE, then it only has BS "attached"
        index=0;
    else{
        for(index=0;index<macController->staticParameters->numberUEs;index++)
            if(macAddress == macController->staticParameters->ulReservations[index].target_ue_id)
                break;

        if(index==macController->staticParameters->numberUEs){
            if(verbose) cout<<"[ProtocolControl] Did not find MAC Address to send MACC SDU."<<endl;
            exit(1);
        }
    }

    if(macController->mux->emptyPdu(macAddress))
        	macController->queueConditionVariables[index].notify_all();     //index 0: UE has only BS as equipment

    char sduBuffer[MAXIMUM_BUFFER_LENGTH];   //Buffer to store SDU for futher transmission

    for(int i=0;i<numberBytes;i++)
        sduBuffer[i] = controlSdu[i];
    
    lock_guard<mutex> lk(macController->queueMutex);

    int macSendingPDU = macController->mux->addSdu(sduBuffer, numberBytes, 0, macAddress);

    //If addSdu returns -1, SDU was added successfully
    if(macSendingPDU==-1) return;

    //Else, queue is full. Need to send PDU
    macController->sendPdu(macSendingPDU);

    macController->mux->addSdu(sduBuffer, numberBytes, 0, macAddress);
}

void 
ProtocolControl::decodeControlSdus(
    char* buffer,                   //Buffer containing Control SDU
    size_t numberDecodingBytes)     //Size of Control SDU in Bytes
{
    if(macController->flagBS){
        if(numberDecodingBytes==3){     //It is probably an "ACK"
            string receivedString;      //String to be compared to "ACK"
            for(int i=0;i<numberDecodingBytes;i++)
                receivedString += buffer[i];
            if(receivedString=="ACK"){
                if(verbose) cout<<"[ProtocolControl] Received ACK from UE."<<endl;
                macController->dynamicParameters->setModified(false);
            }
        }
    }
    else{    //UE needs to set its Dynamic Parameters and return ACK to BS
        macController->managerDynamicParameters((uint8_t*) buffer, numberDecodingBytes);
        if(verbose) cout<<"[ProtocolControl] UE Configured correctly. Returning ACK to BS..."<<endl;
        // ACK
        if(macController->mux->emptyPdu(0))
        	macController->queueConditionVariables[0].notify_all();     //index 0: UE has only BS as equipment

        char ackBuffer[3] = {'A', 'C', 'K'};
        lock_guard<mutex> lk(macController->queueMutex);
        int macSendingPDU = macController->mux->addSdu(ackBuffer, 3, 0,0);

        //If addSdu returns -1, SDU was added successfully
        if(macSendingPDU==-1) return;

        //Else, queue is full. Need to send PDU
        macController->sendPdu(macSendingPDU);

        macController->mux->addSdu(ackBuffer, 3, 0,0);
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
        
        //Manually convert char* to string ////////////////// PROVISIONAL: CONSIDERING ONLY SubframeRx.Start messages
    	int subFrameStartSize = 18;
        for(int i=0;i<subFrameStartSize;i++)
            message+=buffer[i];

    	vector<uint8_t> messageParametersBytes;

        if(message=="BSSubframeRx.Start"){
        	BSSubframeRx_Start messageParametersBS;
        	for(int i=subFrameStartSize;i<messageSize;i++)
        		messageParametersBytes.push_back(message[i]);
        	messageParametersBS.deserialize(messageParametersBytes);
        	if(verbose) cout<<"[ProtocolControl] Received BSSubframeRx.Start message. Receiving PDU from L1..."<<endl;

			macController->dynamicParameters->setMcsUplink(LinkAdaptation::getSinrConvertToCqiUplink(messageParametersBS.sinr));
            macController->decoding();
        }
        if(message=="UESubframeRx.Start"){
			UESubframeRx_Start messageParametersUE;
			for(int i=subFrameStartSize;i<messageSize;i++)
				messageParametersBytes.push_back(message[i]);
			messageParametersUE.deserialize(messageParametersBytes);
			if(verbose) cout<<"[ProtocolControl] Received UESubframeRx.Start message. Receiving PDU from L1..."<<endl;

			macController->dynamicParameters->setMcsUplink(LinkAdaptation::getSinrConvertToCqiUplink(messageParametersUE.sinr));
            macController->decoding();
		}

        //Clear buffer and message and receive next control message
        bzero(buffer, MAXIMUM_BUFFER_LENGTH);
        message.clear();
        messageSize = macController->l1l2Interface->receiveControlMessage(buffer, MAXIMUM_BUFFER_LENGTH);
    }
}
