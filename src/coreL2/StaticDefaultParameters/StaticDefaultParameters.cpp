/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : StaticDefaultParameters.cpp
@Classification : Static Default Parameters
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

@Description : 	This module reads Default configuration from file when system starts
				and stores this information in Class variables.
*/

#include "StaticDefaultParameters.h"

StaticDefaultParameters::StaticDefaultParameters(
		bool _verbose)		//Verbosity flag
{
	verbose = _verbose;

	string readBuffer;		//Buffer that will be used to read file

	defaultConfigurationsFile = fstream("Default.txt");

	//Gets FlagBS
	getline(defaultConfigurationsFile, readBuffer);
	flagBS = (readBuffer[0]=='1');
	readBuffer.clear();

	//Gets Number of UEs (only BS)
	if(flagBS){
		getline(defaultConfigurationsFile, readBuffer);
		numberUEs = stoi(readBuffer);
		readBuffer.clear();
	}
	else numberUEs = 1;		//Consider just one UE if it is not BS

	//Gets Number of RBs available for DL (only BS)
	if(flagBS){
		getline(defaultConfigurationsFile, readBuffer);
		numTRBsDL = stoi(readBuffer);
		readBuffer.clear();
	}

	//Gets UPLinkReservations
	ulReservations.resize(numberUEs);
	for(int i=0;i<numberUEs;i++){
		getline(defaultConfigurationsFile, readBuffer);
		ulReservations[i].target_ue_id = stoi(readBuffer);
		readBuffer.clear();
		getline(defaultConfigurationsFile, readBuffer);
		ulReservations[i].first_rb = stoi(readBuffer);
		readBuffer.clear();
		getline(defaultConfigurationsFile, readBuffer);
		ulReservations[i].number_of_rb = stoi(readBuffer) - ulReservations[i].first_rb;
		readBuffer.clear();
	}

	//Gets numerology
	getline(defaultConfigurationsFile, readBuffer);
	numerology = stoi(readBuffer);
	readBuffer.clear();

	//Gets OFDM GFDM option
	getline(defaultConfigurationsFile, readBuffer);
	ofdm_gfdm = stoi(readBuffer);
	readBuffer.clear();

	//Gets MCS Downlink (only BS)
	if(flagBS){
		getline(defaultConfigurationsFile, readBuffer);
		mcsDownlink = stoi(readBuffer);
		readBuffer.clear();
	}

	//Gets MCS Uplink
	getline(defaultConfigurationsFile, readBuffer);
	mcsUplink = stoi(readBuffer);
	readBuffer.clear();

	//Gets MIMO configuration
	getline(defaultConfigurationsFile, readBuffer);
	mimoConf = stoi(readBuffer);
	readBuffer.clear();
	getline(defaultConfigurationsFile, readBuffer);
	mimoDiversityMultiplexing = stoi(readBuffer);
	readBuffer.clear();
	getline(defaultConfigurationsFile, readBuffer);
	mimoAntenna = stoi(readBuffer);
	readBuffer.clear();
	getline(defaultConfigurationsFile, readBuffer);
	mimoOpenLoopClosedLoop = stoi(readBuffer);
	readBuffer.clear();
	getline(defaultConfigurationsFile, readBuffer);
	mimoPrecoding = stoi(readBuffer);
	readBuffer.clear();

	//Gets Transmission Power Control Information
	getline(defaultConfigurationsFile, readBuffer);
	transmissionpowerControl = stoi(readBuffer);
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
	mtu = stoi(readBuffer);
	readBuffer.clear();

	//The following are only for BS
	if(flagBS){
		//Gets IP Timeout
		getline(defaultConfigurationsFile, readBuffer);
		ipTimeout = stoi(readBuffer);
		readBuffer.clear();

		//Gets Spectrum Sensing Report waiting Timeout
		getline(defaultConfigurationsFile, readBuffer);
		ssreportWaitTimeout = stoi(readBuffer);
		readBuffer.clear();

		//Gets Acknowledgement waiting Timeout
		getline(defaultConfigurationsFile, readBuffer);
		ackWaitTimeout = stoi(readBuffer);
		readBuffer.clear();
	}

	defaultConfigurationsFile.close();

	if(verbose) cout<<"[StaticDefaultParameters] Reading Default information from file successful."<<endl;
}

StaticDefaultParameters::~StaticDefaultParameters() {}
