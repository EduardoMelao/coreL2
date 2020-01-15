/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : ProtocolControl.cpp
@Classification : Protocol Control
@
@Last alteration : January 14th, 2019
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
        index = macController->getIndex(macAddress);

        if(index==-1){
            if(verbose) cout<<"[ProtocolControl] Did not find MAC Address to send MACC SDU."<<endl;
            exit(1);
        }
    }

    if(macController->mux->emptyPdu(macAddress))
        	macController->queueConditionVariables[index].notify_all();     //index 0: UE has only BS as equipment

    char sduBuffer[MAXIMUM_BUFFER_LENGTH];   //Buffer to store SDU for futher transmission

    //Copy MACC SDU
    for(int i=0;i<numberBytes;i++)
        sduBuffer[i] = controlSdu[i];
    
    //Lock Mutex
    lock_guard<mutex> lk(macController->queueMutex);

    //Try to add SDU to sending queue
    int macSendingPDU = macController->mux->addSdu(sduBuffer, numberBytes, 0, macAddress);

    //If addSdu returns -1, SDU was added successfully
    if(macSendingPDU==-1) return;

    //Else, queue is full. Need to send PDU
    macController->sendPdu(macSendingPDU);

    //Then, adds Sdu
    macController->mux->addSdu(sduBuffer, numberBytes, 0, macAddress);
}

void 
ProtocolControl::decodeControlSdus(
    char* buffer,                   //Buffer containing Control SDU
    size_t numberDecodingBytes,     //Size of Control SDU in Bytes
    uint8_t macAddress)             //Source MAC Address
{
    //If it is BS, it can receive ACKs or Rx Metrics
    if(macController->flagBS){
        if(numberDecodingBytes==3){     //It is an "ACK"
            string receivedString;      //String to be compared to "ACK"

            //Convert array received to string
            for(int i=0;i<numberDecodingBytes;i++)
                receivedString += buffer[i];
            
            //Compare Strings
            if(receivedString=="ACK"){
                if(verbose) cout<<"[ProtocolControl] Received ACK from UE."<<endl;
                if(macController->macConfigRequest->dynamicParameters->getModified()==1){
                    macController->macConfigRequest->dynamicParameters->setModified(0);
                }
                else if(verbose) cout<<"[ProtocolControl] There were values changed before receiving ACK."<<endl;
            }
        }
        else{   //RxMetrics
            //Verify index
            int index = macController->getIndex(macAddress);
            if(index == -1){
                if(verbose) cout<<"[ProtocolControl] Error decoding RxMetrics."<<endl;
                exit(1);
            }

            //Decode Bytes
            vector<uint8_t> rxMetricsBytes;         //Serialized Rx Metrics bytes
            for(int i=0;i<numberDecodingBytes;i++)
                rxMetricsBytes.push_back(buffer[i]);

            //Deserialize Bytes
            macController->rxMetrics[index].deserialize(rxMetricsBytes);

            //Calculate new DLMCS
            macController->macConfigRequest->dynamicParameters->setMcsDownlink(macAddress,AdaptiveModulationCoding::getCqiConvertToMcs(macController->rxMetrics[index].cqiReport));

            if(verbose){
                cout<<"[ProtocolControl] RxMetrics from UE "<<(int) macAddress<<" received.";
                cout<<"RBS idle: "<<Cosora::spectrumSensingConvertToRBIdle(macController->rxMetrics[index].ssReport)<<endl;
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

        macController->mux->addSdu(ackBuffer, 3, 0, 0);
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
    uint8_t cqi;                        //Channel Quality information based on SINR measurement from PHY
    uint8_t sourceMacAddress;           //Source MAC Address

    //Receive Control message
    ssize_t messageSize = macController->l1l2Interface->receiveControlMessage(buffer, MAXIMUM_BUFFER_LENGTH);

    //Control message stream
    while(messageSize>0){
        
        //Manually convert char* to string ////////////////// PROVISIONAL: CONSIDERING ONLY SubframeRx.Start messages
    	int subFrameStartSize = 18;
        for(int i=0;i<subFrameStartSize;i++)
            message+=buffer[i];

    	vector<uint8_t> messageParametersBytes;     //Bytes of serialized message parameters

        if(message=="BSSubframeRx.Start"){
        	BSSubframeRx_Start messageParametersBS;     //Define struct for BS paremeters

            //Copy buffer to vector
        	for(int i=subFrameStartSize;i<messageSize;i++)
        		messageParametersBytes.push_back(buffer[i]);
            
            //Deserialize message
        	messageParametersBS.deserialize(messageParametersBytes);

        	if(verbose) cout<<"[ProtocolControl] Received BSSubframeRx.Start message. Receiving PDU from L1..."<<endl;
            
            //Perform channel quality information calculation and uplink MCS calculation
            cqi = LinkAdaptation::getSinrConvertToCqi(messageParametersBS.sinr);

            //Receive source MAC Address from decoding function
            sourceMacAddress = macController->decoding();

            //Calculates new UL MCS and sets it
            macController->macConfigRequest->dynamicParameters->setMcsUplink(sourceMacAddress, AdaptiveModulationCoding::getCqiConvertToMcs(cqi));
        }
        if(message=="UESubframeRx.Start"){
			UESubframeRx_Start messageParametersUE;     //Define struct for UE parameters

            //Copy buffer to vector
			for(int i=subFrameStartSize;i<messageSize;i++)
				messageParametersBytes.push_back(buffer[i]);

            //Deserialize message
			messageParametersUE.deserialize(messageParametersBytes);
			if(verbose) cout<<"[ProtocolControl] Received UESubframeRx.Start message. Receiving PDU from L1..."<<endl;

            //Perform RXMetrics calculation
            macController->rxMetrics->accessControl.lock();     //Lock mutex to prevent access conflict
            macController->rxMetrics->cqiReport = LinkAdaptation::getSinrConvertToCqi(messageParametersUE.sinr);    //SINR->CQI calculation
            Cosora::calculateSpectrumSensingValue(messageParametersUE.ssm, macController->rxMetrics->ssReport);     //SSM->SSReport calculation
            macController->rxMetrics->pmi = messageParametersUE.pmi;
            macController->rxMetrics->ri = messageParametersUE.ri;
            macController->rxMetrics->accessControl.unlock();   //Unlock Mutex
            macController->decoding();
		}

        //Clear buffer and message and receive next control message
        bzero(buffer, MAXIMUM_BUFFER_LENGTH);
        message.clear();
        messageSize = macController->l1l2Interface->receiveControlMessage(buffer, MAXIMUM_BUFFER_LENGTH);
    }
}
