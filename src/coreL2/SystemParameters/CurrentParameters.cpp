/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : CurrentParameters.cpp
@Classification : System Parameters - Current Parameters
@
@Last alteration : January 21st, 2019
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : 	This module reads Default configuration from file when system starts
				and stores this information in Class variables.
*/

#include "CurrentParameters.h"

CurrentParameters::CurrentParameters(
		bool _verbose)		//Verbosity flag
{
	verbose = _verbose;
}

CurrentParameters::~CurrentParameters() {}

void
CurrentParameters::readTxtStaticParameters(){
	string readBuffer;		//Buffer that will be used to read file

	defaultConfigurationsFile.open("Default.txt");

	//Gets FlagBS
	getline(defaultConfigurationsFile, readBuffer);
	flagBS = (readBuffer[0]=='1');

	//Gets Number of UEs (only BS)
	if(flagBS){
		getline(defaultConfigurationsFile, readBuffer);
		numberUEs = stoi(readBuffer);
		readBuffer.clear();
	}
	else numberUEs = 1;			//Consider UE has only one equipment: BS

	//Gets numerology
	getline(defaultConfigurationsFile, readBuffer);
	numerology = stoi(readBuffer);
	readBuffer.clear();

	//Gets OFDM GFDM option
	getline(defaultConfigurationsFile, readBuffer);
	ofdm_gfdm = stoi(readBuffer);
	readBuffer.clear();

	//Gets FUSION LUT matrix default value (only BS)
	if(flagBS){
		uint8_t defaultValue;
		getline(defaultConfigurationsFile, readBuffer);
		defaultValue = stoi(readBuffer);
		readBuffer.clear();
		for(int i=0;i<17;i++)
			fLutMatrix[i] = (defaultValue==0) ? 0:255;
	}

	//Gets Reception Metrics Periodicity
	getline(defaultConfigurationsFile, readBuffer);
	rxMetricPeriodicity = stoi(readBuffer);
	readBuffer.clear();

	//Gets Maximum Transmission Unity in Bytes
	getline(defaultConfigurationsFile, readBuffer);
	mtu = stol(readBuffer);
	readBuffer.clear();

	//Gets IP Timeout
	getline(defaultConfigurationsFile, readBuffer);
	ipTimeout = stoi(readBuffer);
	readBuffer.clear();

	//The following are only for BS
	if(flagBS){
		//Gets Spectrum Sensing Report waiting Timeout
		getline(defaultConfigurationsFile, readBuffer);
		ssreportWaitTimeout = stoi(readBuffer);
		readBuffer.clear();

		//Gets Acknowledgement waiting Timeout
		getline(defaultConfigurationsFile, readBuffer);
		ackWaitTimeout = stoi(readBuffer);
		readBuffer.clear();
	}

	//Resize vectors
	ulReservation.resize(numberUEs);
	if(flagBS) mcsDownlink.resize(numberUEs);
	mcsUplink.resize(numberUEs);
	mimoConf.resize(numberUEs);
	mimoDiversityMultiplexing.resize(numberUEs);
	mimoAntenna.resize(numberUEs);
	mimoOpenLoopClosedLoop.resize(numberUEs);
	mimoPrecoding.resize(numberUEs);
	transmissionPowerControl.resize(numberUEs);

	//Loop to read the parameters numberUEs times
	for(int i=0;i<numberUEs;i++){	
		//Gets UPLinkReservations
		getline(defaultConfigurationsFile, readBuffer);
		ulReservation[i].target_ue_id = stoi(readBuffer);
		readBuffer.clear();
		getline(defaultConfigurationsFile, readBuffer);
		ulReservation[i].first_rb = stoi(readBuffer);
		readBuffer.clear();
		getline(defaultConfigurationsFile, readBuffer);
		ulReservation[i].number_of_rb = stoi(readBuffer);
		readBuffer.clear();

		//Gets MCS Downlink (only BS)
		if(flagBS){
			getline(defaultConfigurationsFile, readBuffer);
			mcsDownlink[i] = stoi(readBuffer);
			readBuffer.clear();
		}

		//Gets MCS Uplink
		getline(defaultConfigurationsFile, readBuffer);
		mcsUplink[i] = stoi(readBuffer);
		readBuffer.clear();

		//Gets MIMO configuration
		getline(defaultConfigurationsFile, readBuffer);
		mimoConf[i] = stoi(readBuffer);
		readBuffer.clear();
		getline(defaultConfigurationsFile, readBuffer);
		mimoDiversityMultiplexing[i] = stoi(readBuffer);
		readBuffer.clear();
		getline(defaultConfigurationsFile, readBuffer);
		mimoAntenna[i] = stoi(readBuffer);
		readBuffer.clear();
		getline(defaultConfigurationsFile, readBuffer);
		mimoOpenLoopClosedLoop[i] = stoi(readBuffer);
		readBuffer.clear();
		getline(defaultConfigurationsFile, readBuffer);
		mimoPrecoding[i] = stoi(readBuffer);
		readBuffer.clear();

		//Gets Transmission Power Control Information
		getline(defaultConfigurationsFile, readBuffer);
		transmissionPowerControl[i] = stoi(readBuffer);
		readBuffer.clear();
	}

	defaultConfigurationsFile.close();

	if(verbose) cout<<"[CurrentParameters] Reading Default information from file successful."<<endl;
}

void 
CurrentParameters::loadDynamicParametersDefaultInformation(
	DynamicParameters* dynamicParameters)	//DynamicParameters object with dynamic information to be filled
{
	if(flagBS)
		dynamicParameters->fillDynamicVariables(fLutMatrix, ulReservation, mcsDownlink, mcsUplink, mimoConf, mimoDiversityMultiplexing, mimoAntenna,
												mimoOpenLoopClosedLoop, mimoPrecoding, transmissionPowerControl, rxMetricPeriodicity);
	else
		dynamicParameters->fillDynamicVariables(ulReservation[0], mcsUplink[0], mimoConf[0], mimoDiversityMultiplexing[0], mimoAntenna[0],
												mimoOpenLoopClosedLoop[0], mimoPrecoding[0], transmissionPowerControl[0], rxMetricPeriodicity);
	if(verbose) cout<<"[CurrentParameters] DynamicParameters filled correctly."<<endl;
}

bool
CurrentParameters::isBaseStation(){
	return flagBS;
}

uint8_t
CurrentParameters::getNumberUEs(){
	return numberUEs;
}

uint8_t 
CurrentParameters::getNumerology(){
	return numerology;
}

bool 
CurrentParameters::isGFDM(){
	return (ofdm_gfdm==1);
}

uint16_t
CurrentParameters::getIpTimeout(){
	return ipTimeout;
}

uint8_t 
CurrentParameters::getSSReportWaitTimeout(){
	return ssreportWaitTimeout;
}

uint8_t 
CurrentParameters::getACKWaitTimeout(){
	return ackWaitTimeout;
}

uint8_t
CurrentParameters::getCurrentMacAddress(){
	return flagBS? 0: ulReservation[0].target_ue_id;
}

uint8_t
CurrentParameters::getMacAddress(
	int index)		//Index corresponding to MAC Address position in class arrays
{
	return ulReservation[index].target_ue_id;
}

void 
CurrentParameters::setSystemParameters(
	DynamicParameters* dynamicParameters)	//Pointer to DynamicParameters object, which stores parameters modified
{
	//(Only BS) Sets Fusion LookUp Table
	if(flagBS){
		dynamicParameters->getFLUTMatrix(fLutMatrix);
	}

	//For each UE, loads MCS Downlink and Uplink
	for(int i=0;i<numberUEs;i++){
		mcsUplink[i] = dynamicParameters->getMcsUplink(ulReservation[i].target_ue_id);
		if(flagBS) mcsDownlink[i] = dynamicParameters->getMcsDownlink(ulReservation[i].target_ue_id);
	}
}

void 
CurrentParameters::setCLIParameters(
	DynamicParameters* dynamicParameters)	//Pointer to DynamicParameters object, which stores parameters modified
{
	//Sets all Uplink Reservations
	dynamicParameters->getUlReservations(ulReservation);

	//Sets Rx Metrics Periodicity
	rxMetricPeriodicity = dynamicParameters->getRxMetricsPeriodicity();

	//Sets MIMO configuration and TPC configuration
	for(int i=0;i<ulReservation.size();i++){
		mimoConf[getIndex(ulReservation[i].target_ue_id)] = dynamicParameters->getMimoConf(ulReservation[i].target_ue_id);
		mimoDiversityMultiplexing[getIndex(ulReservation[i].target_ue_id)] = dynamicParameters->getMimoDiversityMultiplexing(ulReservation[i].target_ue_id);
		mimoAntenna[getIndex(ulReservation[i].target_ue_id)] = dynamicParameters->getMimoAntenna(ulReservation[i].target_ue_id);
		mimoOpenLoopClosedLoop[getIndex(ulReservation[i].target_ue_id)] = dynamicParameters->getMimoOpenLoopClosedLoop(ulReservation[i].target_ue_id);
		mimoPrecoding[getIndex(ulReservation[i].target_ue_id)] = dynamicParameters->getMimoPrecoding(ulReservation[i].target_ue_id);
		transmissionPowerControl[getIndex(ulReservation[i].target_ue_id)] = dynamicParameters->getTPC(ulReservation[i].target_ue_id);
	}
}