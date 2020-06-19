/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : DynamicParameters.cpp
@Classification : System Parameters - Dynamic Parameters
@
@Last alteration : June 16th, 2020
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
    vector<mimo_cfg_t> _mimo,                   //Mimo configurations
    vector<uint8_t> _transmissionPowerControl,  //Transmission Power Control
    uint8_t _rxMetricPeriodicity)               //Rx Metrics Periodicity in number of subframes
{
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);

	//Copy fLutMatrix
    fLutMatrix=_fLutMatrix;

    //Redefine vectors
    ulReservation = _ulReservations;
    mcsDownlink = _mcsDownlink;
    mcsUplink = _mcsUplink;
    mimo = _mimo;
    transmissionPowerControl = _transmissionPowerControl;
    rxMetricPeriodicity = _rxMetricPeriodicity;
}

void
DynamicParameters::fillDynamicVariables(
    allocation_cfg_t _ulReservation,        //Uplink Reservation
    uint8_t _mcsUplink,                     //Modulation and Coding Scheme Uplink
    mimo_cfg_t _mimo,                       //Mimo configuration
    uint8_t _transmissionPowerControl,      //Transmission Power Control
    uint8_t _rxMetricPeriodicity)           //Rx Metrics periodicity in number of  
{
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);

    //Push back information on vectors
    ulReservation.push_back(_ulReservation);
    mcsUplink.push_back(_mcsUplink);
    mimo.push_back(_mimo);
    transmissionPowerControl.push_back(_transmissionPowerControl);
    rxMetricPeriodicity = _rxMetricPeriodicity;
}

void 
DynamicParameters::serialize(
    uint8_t targetUeId,         //Target UE Identification
    vector<uint8_t> & bytes)    //Vector where serialized bytes will be stored
{
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);

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

    mimo[index].serialize(bytes);

	push_bytes(bytes, transmissionPowerControl[index]);

	if(verbose) cout<<"[CLIL2Interface] Serialization successful with "<<bytes.size()<<" bytes of information."<<endl;
}

void
DynamicParameters::deserialize(
    vector<uint8_t> & bytes)    //Vector where serialized bytes are stored
{
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);

    //Clear all vectors before deserializing
    transmissionPowerControl.clear();
    mimo.clear();
    ulReservation.clear();
    mcsUplink.clear();

    uint8_t auxiliary;          //Auxiliary variable to store temporary information

    pop_bytes(auxiliary, bytes);
    transmissionPowerControl.push_back(auxiliary);

    mimo.resize(1);
    mimo[0].deserialize(bytes);

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
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);

    fLutMatrix = _fLutMatrix;     //Copy value
}

void 
DynamicParameters::setUlReservation(
    allocation_cfg_t _ulReservation)    //Spectrum allocation for Uplink
{
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);
    
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
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);
    
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
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);
    
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
    mimo_cfg_t _mimo)                       //MIMO configuration
{
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);
    
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error setting MIMO: MAC Address not found."<<endl;
        exit(1);
    }
    
    if(mimo[index].scheme!=_mimo.scheme || mimo[index].num_tx_antenas!=_mimo.num_tx_antenas || mimo[index].precoding_mtx!=_mimo.precoding_mtx)
    {
        //Assign new value(s)
        mimo[index]=_mimo;
    }
}

void 
DynamicParameters::setTPC(
    uint8_t macAddress,                 //UE Mac Address
    uint8_t _trasmissionPowerControl)   //Transmission Power Control value
{
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);
    
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
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);
    
    if(rxMetricPeriodicity!=_rxMetricPeriodicity){
        rxMetricPeriodicity = _rxMetricPeriodicity; //Assign new values
    }
}


uint8_t 
DynamicParameters::getFLUTMatrix()
{
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);
    
    return fLutMatrix;
}

allocation_cfg_t 
DynamicParameters::getUlReservation(
    uint8_t macAddress) //UE MAC Address
{
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);
    
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
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);
    
    _ulReservations.resize(ulReservation.size());   //Resize container vector
    for(int i=0;i<ulReservation.size();i++)
        _ulReservations[i]=ulReservation[i];
}

uint8_t 
DynamicParameters::getMcsDownlink(
    uint8_t macAddress) //UE MAC Address
{
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);
    
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
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);
    
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error getting MCS Uplink: macAddress not found."<<endl;
        exit(1);
    }
    return mcsUplink[index];
}

mimo_cfg_t 
DynamicParameters::getMimo(
    uint8_t macAddress) //UE MAC Address
{
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);
    
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error getting MIMO Configuration: macAddress not found."<<endl;
        exit(1);
    }
    return mimo[index];
}

uint8_t 
DynamicParameters::getTPC(
    uint8_t macAddress)//UE MAC Address
{
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);
    
    int index = getIndex(macAddress);
    if(index==-1){
        cout<<"[DynamicParameters] Error getting Transmission Power Control: macAddress not found."<<endl;
        exit(1);
    }
    return transmissionPowerControl[index];
}

uint8_t 
DynamicParameters::getRxMetricsPeriodicity(){
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);
    
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


