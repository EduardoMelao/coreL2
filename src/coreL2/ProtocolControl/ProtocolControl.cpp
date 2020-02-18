/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : ProtocolControl.cpp
@Classification : Protocol Control
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

@Description : This module controls MACC SDUS enqueueing to transmission.
    Also, it controls sending/decoding interlayer messages.
*/

#include "ProtocolControl.h"

ProtocolControl::ProtocolControl(
    MacController* _macController,      //MacController Object with all System modules
    bool _verbose)                      //Verbosity flag
{
    macController = _macController;
    rxMetrics = new RxMetrics[macController->currentParameters->getNumberUEs()];
    verbose = _verbose;
}

ProtocolControl::~ProtocolControl() { }

void 
ProtocolControl::decodeControlSdus(
    MacModes & currentMacMode,      //Current MAC execution mode
    char* buffer,                   //Buffer containing Control SDU
    size_t numberDecodingBytes,     //Size of Control SDU in Bytes
    uint8_t macAddress)             //Source MAC Address
{
    //If it is BS, it can receive ACKs or Rx Metrics
    if(macController->currentParameters->isBaseStation()){
        if(numberDecodingBytes==3){     //It is an "ACK"
            string receivedString;      //String to be compared to "ACK"

            //Convert array received to string
            for(int i=0;i<numberDecodingBytes;i++)
                receivedString += buffer[i];
            
            //Compare Strings
            if(receivedString=="ACK"){
                if(verbose) cout<<"[ProtocolControl] Received ACK from UE."<<endl;
            }
        }
        else{   //RxMetrics received from UE
            //Verify index
            int index = macController->currentParameters->getIndex(macAddress);
            if(index == -1){
                if(verbose) cout<<"[ProtocolControl] Error decoding RxMetrics."<<endl;
                exit(1);
            }

            //Decode Bytes
            vector<uint8_t> rxMetricsBytes;         //Serialized Rx Metrics bytes
            for(int i=0;i<numberDecodingBytes;i++)
                rxMetricsBytes.push_back(buffer[i]);

            //Deserialize Bytes
            rxMetrics[index].deserialize(rxMetricsBytes);

            //Calculate new DLMCS
            macController->cliL2Interface->dynamicParameters->setMcsDownlink(macAddress,AdaptiveModulationCoding::getCqiConvertToMcs(rxMetrics[index].cqiReport));

            if(verbose){
                cout<<"[ProtocolControl] RxMetrics from UE "<<(int) macAddress<<" received.";
                cout<<"RBS idle: "<<Cosora::spectrumSensingConvertToRBIdle(rxMetrics[index].ssReport)<<endl;
            }

            //Changes current MAC mode to RECONFIG
            currentMacMode = RECONFIG_MODE;

            if(verbose) cout<<"\n\n[MacController] ___________ System entering RECONFIG mode by System parameters alteration. ___________\n"<<endl;

        }   
    }
    else{    //UE needs to set its Dynamic Parameters and return ACK to BS
        managerDynamicParameters(currentMacMode, (uint8_t*) buffer, numberDecodingBytes);
        if(verbose) cout<<"[ProtocolControl] UE Configured correctly. Returning ACK to BS..."<<endl;

        // ACK

        char ackBuffer[3] = {'A', 'C', 'K'};

        macController->sduBuffers->enqueueControlSdu((uint8_t*) ackBuffer, 3, 0);
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
ProtocolControl::receiveInterlayerMessages(
    MacModes & currentMacMode,          //Current MAC execution mode
    MacRxModes & currentMacRxMode)      //Current MAC execution Rx mode
{
    char buffer[MAXIMUM_BUFFER_LENGTH]; //Buffer where message will be stored
    string message;                     //String containing message converted from char*
    uint8_t cqi;                        //Channel Quality information based on SINR measurement from PHY
    uint8_t sourceMacAddress;           //Source MAC Address

    //Control message stream
    while(currentMacMode!=STOP_MODE){

        if(currentMacMode==IDLE_MODE){
            //Change system Rx mode to ACTIVE_MODE_RX
            currentMacRxMode = ACTIVE_MODE_RX; 

            //Receive Control message
            ssize_t messageSize = macController->l1l2Interface->receiveControlMessage(buffer, MAXIMUM_BUFFER_LENGTH);

            //If it returns 0 or less, no information was received
            if(messageSize<=0)
                continue;
            
            //Manually convert char* to string //#TODO: CONSIDERING ONLY SubframeRx.Start messages! Implement other types of messages (necessary?)
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
                macController->cliL2Interface->dynamicParameters->setMcsUplink(sourceMacAddress, AdaptiveModulationCoding::getCqiConvertToMcs(cqi));

                //Changes current MAC mode to RECONFIG
                currentMacMode = RECONFIG_MODE;

                if(verbose) cout<<"\n\n[MacController] ___________ System entering RECONFIG mode by System parameters alteration. ___________\n"<<endl;

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
                rxMetrics->accessControl.lock();     //Lock mutex to prevent access conflict
                rxMetrics->cqiReport = LinkAdaptation::getSinrConvertToCqi(messageParametersUE.sinr);    //SINR->CQI calculation
                Cosora::calculateSpectrumSensingValue(messageParametersUE.ssm, rxMetrics->ssReport);     //SSM->SSReport calculation
                rxMetrics->pmi = messageParametersUE.pmi;
                rxMetrics->ri = messageParametersUE.ri;
                rxMetrics->accessControl.unlock();   //Unlock Mutex
                macController->decoding();
            }

            //Clear buffer and message and receive next control message
            bzero(buffer, MAXIMUM_BUFFER_LENGTH);
            message.clear();
            messageSize = macController->l1l2Interface->receiveControlMessage(buffer, MAXIMUM_BUFFER_LENGTH);
        }
        else{
            //Change MAC Rx Mode to DISABLED_MODE_RX
            currentMacRxMode = DISABLED_MODE_RX;
        }
    }

    if(verbose) cout<<"[ProtocolControl] Entering STOP_MODE."<<endl;    
    //Change MAC Rx Mode to DISABLED_MODE_RX before stopping System
    currentMacRxMode = DISABLED_MODE_RX;
}



void 
ProtocolControl::managerDynamicParameters(
    MacModes& currentMacMode,           //Current MAC execution mode
    uint8_t* bytesDynamicParameters,    //Serialized bytes from CLIL2Interface object
    size_t numberBytes)                 //Number of bytes of serialized information
{
    vector<uint8_t> serializedBytes;        //Vector to be used for deserialization

    for(int i=0;i<numberBytes;i++)
        serializedBytes.push_back(bytesDynamicParameters[i]);   //Copy information form array to vector

    //Deserialize bytes referring to ULMCS, ULReservation, MIMO, TPC and RxMetricsPeriodicity 
    macController->cliL2Interface->dynamicParameters->deserialize(serializedBytes);

    //Change state to RECONFIG_MODE to apply changes in parameters
    currentMacMode = RECONFIG_MODE;

    cout<<"\n\n[MacController] ___________ System entering RECONFIG mode by MACC SDU received by UE. ___________\n"<<endl;
}

void 
ProtocolControl::rxMetricsReport(){  //This thread executes only on UE
    vector<uint8_t> rxMetricsBytes;     //Array of bytes where RX Metrics will be stored

    //Serialize Rx Metrics in the first position (because it is an UE)
    rxMetrics[0].serialize(rxMetricsBytes);
    
    if(verbose) cout<<"[MacController] RxMetrics report with size "<<rxMetricsBytes.size()<<" enqueued to BS."<<endl;

    //Enqueue MACC SDU
    macController->sduBuffers->enqueueControlSdu(&(rxMetricsBytes[0]), rxMetricsBytes.size(), 0);
	
}