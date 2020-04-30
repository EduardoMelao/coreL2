/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : ProtocolControl.cpp
@Classification : Protocol Control
@
@Last alteration : April 30th, 2020
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
    rxMetricsReceived = false;
    verbose = _verbose;
}

ProtocolControl::~ProtocolControl() {
    delete[] rxMetrics;
}

void 
ProtocolControl::decodeControlSdus(
    char* buffer,                   //Buffer containing Control SDU
    size_t numberDecodingBytes,     //Size of Control SDU in Bytes
    uint8_t macAddress)             //Source MAC Address
{
    //If it is BS, it can receive ACKs or Rx Metrics
    if(macController->currentParameters->isBaseStation()){
        switch(buffer[0]){
            case '1':   //RxMetrics: snr_avg and rankIndicator
            {
                //Verify index
                int index = macController->currentParameters->getIndex(macAddress);
                if(index == -1){
                    if(verbose) cout<<"[ProtocolControl] Error decoding RxMetrics."<<endl;
                    exit(1);
                }

                //Decode Bytes
                vector<uint8_t> rxMetricsBytes;         //Serialized Rx Metrics bytes
                rxMetricsBytes.resize(numberDecodingBytes-1);
                rxMetricsBytes.assign(&(buffer[1]),&(buffer[1])+(numberDecodingBytes-1));

                //Deserialize Bytes
                rxMetrics[index].snr_avg_ri_deserialize(rxMetricsBytes);

                if(verbose){
                    cout<<"[ProtocolControl] RxMetrics from UE "<<(int) macAddress<<" received: ";
                    cout<<"RI: "<<(int)rxMetrics[index].rankIndicator<<endl;
                }

                //Calculate average MCS
                int averageMCS = 0;
                if(rxMetricsReceived){
                    for(int i=0;i<132;i++)
                        averageMCS += LinkAdaptation::getSnrConvertToMcs(rxMetrics[index].snr[i]);
                }
                averageMCS += LinkAdaptation::getSnrConvertToMcs(rxMetrics[index].snr_avg)*macController->currentParameters->getUlReservation(macAddress).number_of_rb;
                averageMCS = averageMCS/(macController->currentParameters->getUlReservation(macAddress).number_of_rb + (rxMetricsReceived? 133:0));

                //Assert WbSNR was not received
                rxMetricsReceived = false;

                if(verbose) cout<<" MCS avg: "<<(int)averageMCS<<endl;

                //Calculate new DLMCS
                macController->cliL2Interface->dynamicParameters->setMcsDownlink(macAddress, averageMCS);

                //If new MCS is different from old, enter RECONFIG mode
                if(macController->cliL2Interface->dynamicParameters->getMcsDownlink(macAddress)!=macController->currentParameters->getMcsDownlink(macAddress)){
                    //Changes current MAC mode to RECONFIG
                    macController->currentParameters->setMacMode(RECONFIG_MODE);

                    if(verbose) cout<<"\n\n[MacController] ___________ System entering RECONFIG mode by System parameters alteration. ___________\n"<<endl;
                }

                break;
            }
            case '2':   //RxMetrics: snr per RB and Spectrum Sensing Report
            {
                //Verify index
                int index = macController->currentParameters->getIndex(macAddress);
                if(index == -1){
                    if(verbose) cout<<"[ProtocolControl] Error decoding RxMetrics."<<endl;
                    exit(1);
                }

                //Decode Bytes
                vector<uint8_t> rxMetricsBytes;         //Serialized Rx Metrics bytes
                rxMetricsBytes.resize(numberDecodingBytes-1);
                rxMetricsBytes.assign(&(buffer[1]),&(buffer[1])+(numberDecodingBytes-1));

                //Deserialize Bytes
                rxMetrics[index].snr_ssr_deserialize(rxMetricsBytes);

                if(verbose){
                    cout<<"[ProtocolControl] RxMetrics from UE "<<(int) macAddress<<" received: ";
                    cout<<"Flut: "<<(int)rxMetrics[index].ssReport<<endl;
                }
                
                //Change flag value
                rxMetricsReceived = true;

                //Perform Fusion calculation
                macController->cosora->fusionAlgorithm(rxMetrics[index].ssReport);

                break;
            }
            case '3':   //It is an "ACK"
            {
                if(verbose) cout<<"[ProtocolControl] Received ACK from UE."<<endl;
                break;
            }
            default:
                break;
        }
    }
    else{    //UE needs to set its Dynamic Parameters and return ACK to BS
        managerDynamicParameters((uint8_t*) buffer, numberDecodingBytes);
        if(verbose) cout<<"[ProtocolControl] UE Configured correctly. Returning ACK to BS..."<<endl;

        // ACK
        char ackBuffer = '3';

        macController->sduBuffers->enqueueControlSdu((uint8_t*) &ackBuffer, 3, 0);
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
ProtocolControl::receiveInterlayerMessages()
{
    char buffer[MQ_MAX_MSG_SIZE]; //Buffer where message will be stored
    string message;                     //String containing message converted from char*
    uint8_t sourceMacAddress;           //Source MAC Address

    //Control message stream
    while(macController->currentParameters->getMacMode()!=STOP_MODE){

        if(macController->currentParameters->getMacMode()==IDLE_MODE||macController->currentParameters->getMacMode()==START_MODE){
            //Change system Rx mode to ACTIVE_MODE_RX
            macController->currentParameters->setMacRxMode(ACTIVE_MODE_RX); 

            //Clear buffer and message and receive next control message
            bzero(buffer, MQ_MAX_MSG_SIZE);
            message.clear();
            ssize_t messageSize = macController->l1l2Interface->receiveControlMessage(buffer, MQ_MAX_MSG_SIZE);

            //If it returns 0 or less, no information was received
            if(messageSize<=0)
                continue;

            vector<uint8_t> messageParametersBytes;     //Bytes of serialized message parameters

            switch(buffer[0]){
                case 'A':    //Treat PHYConfig.Response message
                    //System will stand in IDLE mode
                    macController->currentParameters->setMacMode(IDLE_MODE);

                    cout<<"\n\n[ProtocolControl] ___________ System entering IDLE mode. ___________\n"<<endl;
                break;
                case 'B':    //Treat PHYStop.Response message
                    macController->currentParameters->setMacMode(STOP_MODE);                                 //Change mode

                    cout<<"\n\n[MacController] ___________ System entering STOP mode. ___________\n"<<endl;
                break;
                case 'C':    //Treat BSSubframeRX.Start message
                    if(verbose) cout<<"[ProtocolControl] Receiving PDU from L1..."<<endl;
                    sourceMacAddress = macController->decoding();
                break;
                case 'D':    //Treat UESubframeRX.Start message
                    if(messageSize>1){      //It means that RX metrics were received
                        UESubframeRx_Start messageParametersUE;     //Define struct for UE parameters

                        //Copy buffer to vector
                        messageParametersBytes.resize(messageSize-1);
                        messageParametersBytes.assign(&(buffer[1]), &(buffer[1])+(messageSize-1));

                        //Deserialize message
                        messageParametersUE.deserialize(messageParametersBytes);
                        if(verbose) cout<<"[ProtocolControl] Received UESubframeRx.Start with Rx Metrics message. Receiving PDU from L1..."<<endl;

                        //Perform Spctrum Sensing Report calculation
                        uint8_t ssReport = Cosora::calculateSpectrumSensingValue(messageParametersUE.ssm);     //SSM->SSReport calculation

                        //Assign new values and enqueue a control SDU to BS with updated information
                        for(int i=0;i<132;i++)
                            rxMetrics->snr[i] = messageParametersUE.snr[i];
                        rxMetrics->ssReport = ssReport;

                        //Send Report to BS
                        rxMetricsReport(false);
                    }

                    if(verbose) cout<<"[ProtocolControl] Receiving PDU from L1..."<<endl;
                    macController->decoding();
                break;
                case 'F':    //Treat PHYTx.Indication message
                    macController->scheduling();
                break;
                case 'E':    //Treat SubframeRX.End message
                    if(verbose) cout<<"[ProtocolControl] Received SubframeRx.End from PHY."<<endl;
                default: 
                    //Change MAC Rx Mode to DISABLED_MODE_RX
                    macController->currentParameters->setMacRxMode(DISABLED_MODE_RX);
                break;
            }
        }
        //Change MAC Rx Mode to DISABLED_MODE_RX
        macController->currentParameters->setMacRxMode(DISABLED_MODE_RX);
    }

    if(verbose) cout<<"[ProtocolControl] Entering STOP_MODE."<<endl;    
    //Change MAC Rx Mode to DISABLED_MODE_RX before stopping System
    macController->currentParameters->setMacRxMode(DISABLED_MODE_RX);
}

void 
ProtocolControl::managerDynamicParameters(
    uint8_t* bytesDynamicParameters,    //Serialized bytes from CLIL2Interface object
    size_t numberBytes)                 //Number of bytes of serialized information
{
    vector<uint8_t> serializedBytes;        //Vector to be used for deserialization

    for(int i=0;i<numberBytes;i++)
        serializedBytes.push_back(bytesDynamicParameters[i]);   //Copy information form array to vector

    //Deserialize bytes referring to ULMCS, ULReservation, MIMO, TPC and RxMetricsPeriodicity 
    macController->cliL2Interface->dynamicParameters->deserialize(serializedBytes);

    //Change state to RECONFIG_MODE to apply changes in parameters
    macController->currentParameters->setMacMode(RECONFIG_MODE);

    cout<<"\n\n[ProtocolControl] ___________ System entering RECONFIG mode by MACC SDU received by UE. ___________\n"<<endl;
}

void 
ProtocolControl::rxMetricsReport(
    bool snrRiOrSnrSsm)                 //True if SNR_AVG and RI; False if SNR per RB and SSM
{     
    //This procedure executes only on UE
    vector<uint8_t> rxMetricsBytes;     //Array of bytes where RX Metrics will be stored

    //Serialize Rx Metrics in the first position (because it is an UE)
    if(snrRiOrSnrSsm){
        rxMetrics[0].snr_avg_ri_serialize(rxMetricsBytes);
        rxMetricsBytes.insert(rxMetricsBytes.begin(), '1');     //Add Rx Metrics MACC SDU code to the beggining of the SDU
    }
    else{
        rxMetrics[0].snr_ssr_serialize(rxMetricsBytes);
        rxMetricsBytes.insert(rxMetricsBytes.begin(), '2');     //Add Rx Metrics MACC SDU code to the beggining of the SDU
    }

    if(verbose) cout<<"[MacController] RxMetrics report with size "<<rxMetricsBytes.size()<<" enqueued to BS."<<endl;

    //Enqueue MACC SDU
    macController->sduBuffers->enqueueControlSdu(&(rxMetricsBytes[0]), rxMetricsBytes.size(), 0);
}
