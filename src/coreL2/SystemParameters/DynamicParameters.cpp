/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : DynamicParameters.cpp
@Classification : System Parameters - Dynamic Parameters
@
@Last alteration : Febrary 20th, 2020
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : 	This module stores dynamic information to configure BS and UE
*/

#include "DynamicParameters.h"

DynamicParameters::DynamicParameters(
    bool _verbose)  //Verbosity flag
{
    verbose = _verbose;
}

DynamicParameters::~DynamicParameters() {}

void 
DynamicParameters::fillDynamicVariables(
    uint8_t _fLutMatrix,                        //Fusion Lookup Table Matrix
    vector<allocation_cfg_t> _ulReservations,   //Uplink Reservations
    vector<uint8_t> _mcsDownlink,               //Modulation and Coding Scheme Downlink
    vector<uint8_t> _mcsUplink,                 //Modulation and Coding Scheme Uplink
    vector<uint8_t> _mimoConf,                  //Mimo Configuration
    vector<uint8_t> _mimoDiversityMultiplexing, //Mimo Diversity/Multiplexing flag
	vector<uint8_t> _mimoAntenna,               //Mimo number of antennas
    vector<uint8_t> _mimoOpenLoopClosedLoop,    //Mimo OpenLoop/ClosedLoop flag
    vector<uint8_t> _mimoPrecoding,             //Mimo Precoding index
    vector<uint8_t> _transmissionPowerControl,  //Transmission Power Control
    uint8_t _rxMetricPeriodicity)               //Rx Metrics Periodicity in number of subframes
{
	//Copy fLutMatrix
    fLutMatrix=_fLutMatrix;

    //Redefine vectors
    ulReservation = _ulReservations;
    mcsDownlink = _mcsDownlink;
    mcsUplink = _mcsUplink;
    mimoConf = _mimoConf;
    mimoDiversityMultiplexing = _mimoDiversityMultiplexing;
    mimoAntenna = _mimoAntenna;
    mimoOpenLoopClosedLoop = _mimoOpenLoopClosedLoop;
    mimoPrecoding = _mimoPrecoding;
    transmissionPowerControl = _transmissionPowerControl;
    rxMetricPeriodicity = _rxMetricPeriodicity;
}

void
DynamicParameters::fillDynamicVariables(
    allocation_cfg_t _ulReservation,        //Uplink Reservation
    uint8_t _mcsUplink,                     //Modulation and Coding Scheme Uplink
    uint8_t _mimoConf,                      //Mimo configuration
    uint8_t _mimoDiversityMultiplexing,     //Mimo Diversity/Multiplexing flag
    uint8_t _mimoAntenna,                   //Mimo number of antennas
	uint8_t _mimoOpenLoopClosedLoop,        //Mimo OpenLoop/ClosedLoop flag
    uint8_t _mimoPrecoding,                 //Mimo Precoding index
    uint8_t _transmissionPowerControl,      //Transmission Power Control
    uint8_t _rxMetricPeriodicity)           //Rx Metrics periodicity in number of  
{
    //Push back information on vectors
    ulReservation.push_back(_ulReservation);
    mcsUplink.push_back(_mcsUplink);
    mimoConf.push_back(_mimoConf);
    mimoDiversityMultiplexing.push_back(_mimoDiversityMultiplexing);
    mimoAntenna.push_back(_mimoAntenna);
    mimoOpenLoopClosedLoop.push_back(_mimoOpenLoopClosedLoop);
    mimoPrecoding.push_back(_mimoPrecoding);
    transmissionPowerControl.push_back(_transmissionPowerControl);
    rxMetricPeriodicity = _rxMetricPeriodicity;
}

void 
DynamicParameters::serialize(
    uint8_t targetUeId,         //Target UE Identification
    vector<uint8_t> & bytes)    //Vector where serialized bytes will be stored
{
    bytes.clear();  //Clear vector

    uint8_t auxiliary;      //Auxiliary variable to help shift binaries

    //Gets UE index and verify
    int index = getIndex(targetUeId);
    if(index==-1){
        cout<<"[DynamicParameters] Error in serialization: MAC Addres not found."<<endl;
        exit(1);
    }
	//Least significant 4 bits: mcsUplink[index]
	//Most significant 4 bits: rxMetrixPeriodicity
	auxiliary = (mcsUplink[index]&15)|((rxMetricPeriodicity&15)<<4);
	push_bytes(bytes, auxiliary);

    push_bytes(bytes, ulReservation[index].target_ue_id);
    push_bytes(bytes, ulReservation[index].first_rb);
    push_bytes(bytes, ulReservation[index].number_of_rb);

	auxiliary = (mimoPrecoding[index]&15)|((mimoOpenLoopClosedLoop[index]&1)<<4)|((mimoAntenna[index]&1)<<5)|((mimoDiversityMultiplexing[index]&1)<<6)|((mimoConf[index]&1)<<7);
	push_bytes(bytes, auxiliary);

	push_bytes(bytes, transmissionPowerControl[index]);

	if(verbose) cout<<"[CLIL2Interface] Serialization successful with "<<bytes.size()<<" bytes of information."<<endl;
}

void
DynamicParameters::deserialize(
    vector<uint8_t> & bytes)    //Vector where serialized bytes are stored
{
    uint8_t auxiliary;          //Auxiliary variable to store temporary information

    pop_bytes(auxiliary, bytes);
    transmissionPowerControl.push_back(auxiliary);

    pop_bytes(auxiliary, bytes);
    mimoPrecoding.push_back(auxiliary&15);                  //First 4 bits
    mimoOpenLoopClosedLoop.push_back((auxiliary>>4)&1);     //5th bit
    mimoAntenna.push_back((auxiliary>>5)&1);                //6th bit
    mimoDiversityMultiplexing.push_back((auxiliary>>6)&1);  //7th bit
    mimoConf.push_back((auxiliary>>7)&1);                   //8th bit

    ulReservation.resize(1);
    pop_bytes(ulReservation[0].number_of_rb, bytes);
    pop_bytes(ulReservation[0].first_rb, bytes);
    pop_bytes(ulReservation[0].target_ue_id, bytes);

    pop_bytes(auxiliary, bytes);
    rxMetricPeriodicity = ((auxiliary>>4)&15);  //Last 4 bits
    mcsUplink.push_back(auxiliary&15);          //First 4 bits
}

void 
DynamicParameters::setFLutMatrix(
    uint8_t _fLutMatrix)       //Fusion Spectrum Analysis Lookup Table
{
    fLutMatrix = _fLutMatrix;     //Copy value
}

void 
DynamicParameters::setUlReservation(
    allocation_cfg_t _ulReservation)    //Spectrum allocation for Uplink
{
    int index = getIndex(_ulReservation.target_ue_id);
    if(index==-1){
        cout<<"[DynamicParameters] Error setting UlReservation: MAC Address not found."<<endl;
        exit(1);
    }

    if(_ulReservation.first_rb!=ulReservation[index].first_rb || _ulReservation.number_of_rb!=ulReservation[index].number_of_rb){
        ulReservation[index]=_ulReservation;    //Copy values
    }
}

void 
DynamicParameters::setMcsDownlink(
    uint8_t macAddress,             //UE Mac Address
    uint8_t _mcsDownlink)           //Modulation and Coding Scheme for Downlink
{
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error setting MCS Downlink: MAC Address not found."<<endl;
        exit(1);
    }
    
    if(mcsDownlink[index]!=_mcsDownlink){
        mcsDownlink[index] = _mcsDownlink;  //Assign new value
    }
}

void 
DynamicParameters::setMcsUplink(
    uint8_t macAddress,                 //UE Mac Address
    uint8_t _mcsUplink)     //Modulation and Coding Scheme for Uplink
{
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error setting MCS Uplink: MAC Address not found."<<endl;
        exit(1);
    }

    if(mcsUplink[index]!=_mcsUplink){
        mcsUplink[index] = _mcsUplink;  //Assign new value
    }
}

void 
DynamicParameters::setMimo(
    uint8_t macAddress,                     //UE Mac Address
    uint8_t _mimoConf,                      //MIMO configuration
    uint8_t _mimoDiversityMultiplexing,     //MIMO Diversity or Multiplexing
    uint8_t _mimoAntenna,                   //MIMO antenna scheme: 2x2 or 4x4
    uint8_t _mimoOpenLoopClosedLoop,        //MIMO open loop or closed loop
    uint8_t _mimoPrecoding)                 //MIMO codeblock configuration for DL OR UL
{
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error setting MIMO: MAC Address not found."<<endl;
        exit(1);
    }
    
    if(mimoConf[index]!=_mimoConf || mimoDiversityMultiplexing[index]!=_mimoDiversityMultiplexing || mimoAntenna[index]!=_mimoAntenna || 
        mimoOpenLoopClosedLoop[index]!=_mimoOpenLoopClosedLoop || mimoPrecoding[index]!=_mimoPrecoding)
    {
        //Assign new value(s)
        mimoConf[index] = _mimoConf;
        mimoDiversityMultiplexing[index] = _mimoDiversityMultiplexing;
        mimoAntenna[index] = _mimoAntenna;
        mimoOpenLoopClosedLoop[index] = _mimoOpenLoopClosedLoop;
        mimoPrecoding[index] = _mimoPrecoding;
    }
}

void 
DynamicParameters::setTPC(
    uint8_t macAddress,                 //UE Mac Address
    uint8_t _trasmissionPowerControl)   //Transmission Power Control value
{
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error setting TPC: MAC Address not found."<<endl;
        exit(1);
    }
    
    if(transmissionPowerControl[index]!=_trasmissionPowerControl){
        transmissionPowerControl[index] = _trasmissionPowerControl;    //Assign new values
    }
}

void 
DynamicParameters::setRxMetricPeriodicity(
    uint8_t _rxMetricPeriodicity)       //Rx Metrics Periodicity in number of subframes
{    
    if(rxMetricPeriodicity!=_rxMetricPeriodicity){
        rxMetricPeriodicity = _rxMetricPeriodicity; //Assign new values
    }
}


uint8_t 
DynamicParameters::getFLUTMatrix()
{
    return fLutMatrix;
}

allocation_cfg_t 
DynamicParameters::getUlReservation(
    uint8_t macAddress) //UE MAC Address
{
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error getting UlReservation: macAddress not found."<<endl;
        exit(1);
    }
    return ulReservation[index];
}

void 
DynamicParameters::getUlReservations(
    vector<allocation_cfg_t> & _ulReservations)     //Vector where UL Reservations will be stored
{
    _ulReservations.resize(ulReservation.size());   //Resize container vector
    for(int i=0;i<ulReservation.size();i++)
        _ulReservations[i]=ulReservation[i];
}

uint8_t 
DynamicParameters::getMcsDownlink(
    uint8_t macAddress) //UE MAC Address
{
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error getting MCS Downlink: macAddress not found."<<endl;
        exit(1);
    }
    return mcsDownlink[index];
}

uint8_t 
DynamicParameters::getMcsUplink(
    uint8_t macAddress) //UE MAC Address
{
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error getting MCS Uplink: macAddress not found."<<endl;
        exit(1);
    }
    return mcsUplink[index];
}

uint8_t 
DynamicParameters::getMimoConf(
    uint8_t macAddress) //UE MAC Address
{
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error getting MIMO Configuration: macAddress not found."<<endl;
        exit(1);
    }
    return mimoConf[index];
}


uint8_t 
DynamicParameters::getMimoDiversityMultiplexing(
    uint8_t macAddress) //UE MAC Address
{
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error getting MIMO Diversity/Multiplexing flag: macAddress not found."<<endl;
        exit(1);
    }
    return mimoDiversityMultiplexing[index];
}

uint8_t 
DynamicParameters::getMimoAntenna(
    uint8_t macAddress) //UE MAC Address
{
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error getting MIMO number of Antennas: macAddress not found."<<endl;
        exit(1);
    }
    return mimoAntenna[index];
}

uint8_t 
DynamicParameters::getMimoOpenLoopClosedLoop(
    uint8_t macAddress) //UE MAC Address
{
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error getting MIMO OpenLoop/ClosedLoop flag: macAddress not found."<<endl;
        exit(1);
    }
    return mimoOpenLoopClosedLoop[index];
}

uint8_t 
DynamicParameters::getMimoPrecoding(
    uint8_t macAddress) //UE MAC Address
{
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error getting MIMO Precoding index: macAddress not found."<<endl;
        exit(1);
    }
    return mimoPrecoding[index];
}

uint8_t 
DynamicParameters::getTPC(
    uint8_t macAddress)//UE MAC Address
{
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error getting Transmission Power Control: macAddress not found."<<endl;
        exit(1);
    }
    return transmissionPowerControl[index];
}

uint8_t 
DynamicParameters::getRxMetricsPeriodicity(){
    return rxMetricPeriodicity;
}

int
DynamicParameters::getIndex(
    uint8_t macAddress)     //UE MAC Address
{
	//Index of BS is always zero
	if(macAddress==0)
		return 0;

	//Look for index
    for(int i=0;i<ulReservation.size();i++)
        if(macAddress==ulReservation[i].target_ue_id)
            return i;
    return -1;
}


