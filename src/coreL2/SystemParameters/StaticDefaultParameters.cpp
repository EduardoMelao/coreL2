/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : StaticDefaultParameters.cpp
@Classification : System Parameters - Static and Default Parameters
@
@Last alteration : January 16th, 2019
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

#include "StaticDefaultParameters.h"

StaticDefaultParameters::StaticDefaultParameters(
		bool _verbose)		//Verbosity flag
{
	verbose = _verbose;
}

StaticDefaultParameters::~StaticDefaultParameters() {}

void
StaticDefaultParameters::readTxtStaticParameters(){
	string readBuffer;		//Buffer that will be used to read file

	defaultConfigurationsFile = fstream("Default.txt");

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

	//Resize vector
	ulReservations.resize(numberUEs);
	numerology.resize(numberUEs);
	ofdm_gfdm.resize(numberUEs);
	if(flagBS) mcsDownlink.resize(numberUEs);
	mcsUplink.resize(numberUEs);
	mimoConf.resize(numberUEs);
	mimoDiversityMultiplexing.resize(numberUEs);
	mimoAntenna.resize(numberUEs);
	mimoOpenLoopClosedLoop.resize(numberUEs);
	mimoPrecoding.resize(numberUEs);
	transmissionPowerControl.resize(numberUEs);
	rxMetricPeriodicity.resize(numberUEs);
	mtu.resize(numberUEs);
	ipTimeout.resize(numberUEs);
	if(flagBS){
		ssreportWaitTimeout.resize(numberUEs);
		ackWaitTimeout.resize(numberUEs);
	}

	//Loop to read the parameters numberUEs times
	for(int i=0;i<numberUEs;i++){	
		//Gets UPLinkReservations
		getline(defaultConfigurationsFile, readBuffer);
		ulReservations[i].target_ue_id = stoi(readBuffer);
		readBuffer.clear();
		getline(defaultConfigurationsFile, readBuffer);
		ulReservations[i].first_rb = stoi(readBuffer);
		readBuffer.clear();
		getline(defaultConfigurationsFile, readBuffer);
		ulReservations[i].number_of_rb = stoi(readBuffer);
		readBuffer.clear();

		//Gets numerology
		getline(defaultConfigurationsFile, readBuffer);
		numerology[i] = stoi(readBuffer);
		readBuffer.clear();

		//Gets OFDM GFDM option
		getline(defaultConfigurationsFile, readBuffer);
		ofdm_gfdm[i] = stoi(readBuffer);
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
		rxMetricPeriodicity[i] = stoi(readBuffer);
		readBuffer.clear();

		//Gets Maximum Transmission Unity in Bytes
		getline(defaultConfigurationsFile, readBuffer);
		mtu[i] = stol(readBuffer);
		readBuffer.clear();

		//Gets IP Timeout
		getline(defaultConfigurationsFile, readBuffer);
		ipTimeout[i] = stoi(readBuffer);
		readBuffer.clear();

		//The following are only for BS
		if(flagBS){
			//Gets Spectrum Sensing Report waiting Timeout
			getline(defaultConfigurationsFile, readBuffer);
			ssreportWaitTimeout[i] = stoi(readBuffer);
			readBuffer.clear();

			//Gets Acknowledgement waiting Timeout
			getline(defaultConfigurationsFile, readBuffer);
			ackWaitTimeout[i] = stoi(readBuffer);
			readBuffer.clear();
		}
	}

	defaultConfigurationsFile.close();

	if(verbose) cout<<"[StaticDefaultParameters] Reading Default information from file successful."<<endl;
}

void 
StaticDefaultParameters::loadDynamicParametersDefaultInformation(
	DynamicParameters* dynamicParameters)	//DynamicParameters object with dynamic information to be filled
{
	if(flagBS)
		dynamicParameters->fillDynamicVariables(fLutMatrix, ulReservations, mcsDownlink, mcsUplink, mimoConf, mimoDiversityMultiplexing, mimoAntenna,
												mimoOpenLoopClosedLoop, mimoPrecoding, transmissionPowerControl, rxMetricPeriodicity);
	else
		dynamicParameters->fillDynamicVariables(ulReservations[0], mcsUplink[0], mimoConf[0], mimoDiversityMultiplexing[0], mimoAntenna[0],
												mimoOpenLoopClosedLoop[0], mimoPrecoding[0], transmissionPowerControl[0], rxMetricPeriodicity[0]);
	if(verbose) cout<<"[StaticDefaultParameters] DynamicParameters filled correctly."<<endl;
}
