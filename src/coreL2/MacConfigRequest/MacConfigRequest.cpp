/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacConfigRequest.cpp
@Classification : MAC Configuration Request
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

@Description : 	This module uses dynamic information to configure BS and UE
*/

#include "MacConfigRequest.h"

MacConfigRequest::MacConfigRequest(
		bool _verbose)		//Verbosity flag
{
    verbose = _verbose;
}


MacConfigRequest::~MacConfigRequest(){}

void MacConfigRequest::deserialize(
    vector<uint8_t> & bytes)    //Bytes containing serialized values
{
    uint8_t auxiliary;          //Auxiliary variable to store temporary information

    pop_bytes(transmissionPowerControl, bytes);

    pop_bytes(auxiliary, bytes);
    mimoPrecoding = auxiliary&15;                   //First 4 bits
    mimoOpenLoopClosedLoop = (auxiliary>>4)&1;      //5th bit
    mimoAntenna = (auxiliary>>5)&1;                 //6th bit
    mimoDiversityMultiplexing = (auxiliary>>6)&1;   //7th bit
    mimoConf = (auxiliary>>7)&1;                    //8th bit

    pop_bytes(auxiliary, bytes);
    mcsDownlink = auxiliary&15;     //First 4 bits
    mcsUplink = (auxiliary>>4)&15;  //Next 4 bits

    ulReservations.resize(1);
    pop_bytes(ulReservations[0].number_of_rb, bytes);
    pop_bytes(ulReservations[0].first_rb, bytes);
    pop_bytes(ulReservations[0].target_ue_id, bytes);

    pop_bytes(auxiliary, bytes);
    rxMetricPeriodicity = (auxiliary>>4)&15;    //Last 4 bits
    fLutMatrix[16] = auxiliary&15;              //First 4 bits

    for(int i=15;i>=0;i--)
        pop_bytes(fLutMatrix[i], bytes);
}

void
MacConfigRequest::fillDynamicVariables(
    uint8_t* _fLutMatrix,                       //Bitmap from Fusion Spectrum Analysis
    vector<allocation_cfg_t> _ulReservations,   //Spectrum allocation from Uplink
    uint8_t _mcsDownlink,                       //Modulation and Coding Scheme for Downlink
    uint8_t _mcsUplink,                         //Modulation and Coding Scheme for Uplink
    uint8_t _mimoConf,                          //SISO(0) oe MIMO(1) flag
    uint8_t _mimoDiversityMultiplexing,         //Diversity(0) or Multiplexing(1) flag
	uint8_t _mimoAntenna,                       //MIMO 2x2(0) or 4x4(1) antenna scheme
    uint8_t _mimoOpenLoopClosedLoop,            //MIMO 0 = Open Loop; 1 = Closed Loop
    uint8_t _mimoPrecoding,                     //MIMO codeblock configuration for DL and UL
    uint8_t _transmissionPowerControl,          //Transmission Power Control
    uint8_t _rxMetricPeriodicity)               //CSI period for CQI, PMI and SSM provided by PHY
{
    setFLutMatrix(_fLutMatrix);
    setUlReservations(_ulReservations);
    setMcsDownlink(_mcsDownlink);
    setMcsUplink(_mcsUplink);
    setMimo(_mimoConf, _mimoDiversityMultiplexing, _mimoAntenna, _mimoOpenLoopClosedLoop, _mimoPrecoding);
    setTPC(_transmissionPowerControl);
    setRxMetricPeriodicity(_rxMetricPeriodicity);
}

void
MacConfigRequest::fillDynamicVariables(
    vector<allocation_cfg_t> _ulReservation,    //Spectrum allocation from Uplink
    uint8_t _mcsUplink,                         //Modulation and Coding Scheme for Uplink
    uint8_t _mimoConf,                          //SISO(0) oe MIMO(1) flag
    uint8_t _mimoDiversityMultiplexing,         //Diversity(0) or Multiplexing(1) flag
	uint8_t _mimoAntenna,                       //MIMO 2x2(0) or 4x4(1) antenna scheme
    uint8_t _mimoOpenLoopClosedLoop,            //MIMO 0 = Open Loop; 1 = Closed Loop
    uint8_t _mimoPrecoding,                     //MIMO codeblock configuration for DL and UL
    uint8_t _transmissionPowerControl,          //Transmission Power Control
    uint8_t _rxMetricPeriodicity)               //CSI period for CQI, PMI and SSM provided by PHY
{
    setUlReservations(_ulReservation);
    setMcsUplink(_mcsUplink);
    setMimo(_mimoConf, _mimoDiversityMultiplexing, _mimoAntenna, _mimoOpenLoopClosedLoop, _mimoPrecoding);
    setTPC(_transmissionPowerControl);
    setRxMetricPeriodicity(_rxMetricPeriodicity);
}

void 
MacConfigRequest::setFLutMatrix(
    uint8_t* _fLutMatrix)       //Fusion Spectrum Analysis Lookup Table
{
    lock_guard<mutex> lk(dynamicParametersMutex);   //Lock mutex until alterations are finished
    {
        for(int i=0;i<17;i++)
            fLutMatrix[i] = _fLutMatrix[i];     //Copy array values
        modified ++;                            //Change counter after modifying values
    }
}

void 
MacConfigRequest::setUlReservations(
    vector<allocation_cfg_t> _ulReservations)    //Spectrum allocation for Uplink
{
    lock_guard<mutex>  lk(dynamicParametersMutex);  //Lock mutex until alterations are finished
    {
        ulReservations.clear();
        for(int i=0;i<_ulReservations.size();i++)
            ulReservations.push_back(_ulReservations[i]);   //Copy values
        modified ++;                                        //Change counter after modifying values
    }
}

void 
MacConfigRequest::setMcsDownlink(
    uint8_t _mcsDownlink)           //Modulation and Coding Scheme for Downlink
{
    lock_guard<mutex>  lk(dynamicParametersMutex);  //Lock mutex until alterations are finished
    {
        if(mcsDownlink!=_mcsDownlink){
            mcsDownlink = _mcsDownlink;     //Assign new value
            modified ++;                    //Change counter after modifying value
        }
    }
}

void 
MacConfigRequest::setMcsUplink(
    uint8_t _mcsUplink)     //Modulation and Coding Scheme for Uplink
{
    lock_guard<mutex>  lk(dynamicParametersMutex);  //Lock mutex until alterations are finished
    {
        if(mcsUplink!=_mcsUplink){
            mcsUplink = _mcsUplink;     //Assign new value
            modified ++;                //Change counter after modifying value
        }
    }
}

void 
MacConfigRequest::setMimo(
    uint8_t _mimoConf,                      //MIMO configuration
    uint8_t _mimoDiversityMultiplexing,     //MIMO Diversity or Multiplexing
    uint8_t _mimoAntenna,                   //MIMO antenna scheme: 2x2 or 4x4
    uint8_t _mimoOpenLoopClosedLoop,        //MIMO open loop or closed loop
    uint8_t _mimoPrecoding)                  //MIMO codeblock configuration for DL OR UL
{
    lock_guard<mutex>  lk(dynamicParametersMutex);  //Lock mutex until alterations are finished
    {
        //Assign new values
        mimoConf = _mimoConf;
        mimoDiversityMultiplexing = _mimoDiversityMultiplexing;
        mimoAntenna = _mimoAntenna;
        mimoOpenLoopClosedLoop = _mimoOpenLoopClosedLoop;
        mimoPrecoding = _mimoPrecoding;
        
        modified ++;        //Change counter after modifying value
    }
}

void 
MacConfigRequest::setTPC(
    uint8_t _trasmissionPowerControl)
{
    lock_guard<mutex>  lk(dynamicParametersMutex);  //Lock mutex until alterations are finished
    {
        transmissionPowerControl = _trasmissionPowerControl;    //Assign new values
        modified ++;        //Change counter after modifying value
    }
}

void 
MacConfigRequest::setRxMetricPeriodicity(
    uint8_t _rxMetricPeriodicity)
{
    lock_guard<mutex>  lk(dynamicParametersMutex);  //Lock mutex until alterations are finished
    {
        rxMetricPeriodicity = _rxMetricPeriodicity;     //Assign new values
        modified ++;        //Change counter after modifying value
    }
}

void
MacConfigRequest::setModified(
    uint8_t _modified)  //New modified counter value
{
    lock_guard<mutex>  lk(dynamicParametersMutex);  //Lock mutex until alterations are finished
    {
        modified = _modified;   //Assign new value
    }
}

void 
MacConfigRequest::serialize(
    uint8_t targetUeId,         //Target UE Identification
    vector<uint8_t> & bytes)    //Vector where serialized bytes will be stored
{
    bytes.clear();  //Clear vector
    
    int counter;            //Counter to help loop controls
    uint8_t auxiliary;      //Auxiliary variable to help shift binaries

	for(counter=0;counter<16;counter++)
		push_bytes(bytes, fLutMatrix[counter]);

	//Least significant 4 bits: fLutMatrix[16]
	//Most significant 4 bits: rxMetrixPeriodicity
	auxiliary = (fLutMatrix[16]&15)|((rxMetricPeriodicity&15)<<4);
	push_bytes(bytes, auxiliary);

	for(counter=0;counter<ulReservations.size();counter++){
		if(ulReservations[counter].target_ue_id==targetUeId){
			push_bytes(bytes, ulReservations[counter].target_ue_id);
			push_bytes(bytes, ulReservations[counter].first_rb);
			push_bytes(bytes, ulReservations[counter].number_of_rb);
			break;
		}
	}
	if(counter==ulReservations.size()){
		if(verbose) cout<<"[MacConfigRequest] Target UE not found to serialize"<<endl;
		exit(1);
	}

	//Least significant 4 bits: mcsDownlink
	//Most significant 4 bits: mcsUplink
	auxiliary = (mcsDownlink&15)|((mcsUplink&15)<<4);
	push_bytes(bytes, auxiliary);

	auxiliary = (mimoPrecoding&15)|((mimoOpenLoopClosedLoop&1)<<4)|((mimoAntenna&1)<<5)|((mimoDiversityMultiplexing&1)<<6)|((mimoConf&1)<<7);
	push_bytes(bytes, auxiliary);

	push_bytes(bytes, transmissionPowerControl);

	if(verbose) cout<<"[MacConfigRequest] Serialization successful with "<<bytes.size()<<" bytes of information."<<endl;
}

uint8_t 
MacConfigRequest::getModified(){
        return modified;
}
