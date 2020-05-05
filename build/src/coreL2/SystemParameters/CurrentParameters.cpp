/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : CurrentParameters.cpp
@Classification : System Parameters - Current Parameters
@
@Last alteration : May 5th, 2020
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

CurrentParameters::CurrentParameters() { }

CurrentParameters::CurrentParameters(
		bool _verbose)		//Verbosity flag
{
	flagUesOutdated = false;
	verbose = _verbose;
}

CurrentParameters::~CurrentParameters() {}

void
CurrentParameters::readTxtSystemParameters(
	string fileName)	//Name of the file to be read
{
	string readBuffer;		//Buffer that will be used to read file

	ifstream readingConfigurationsFile;

	try{
		readingConfigurationsFile.open(fileName);

		//Gets FlagBS
		getline(readingConfigurationsFile, readBuffer);
		flagBS = (readBuffer[0]=='1');
		readBuffer.clear();

		//Gets Number of UEs (only BS)
		if(flagBS){
			getline(readingConfigurationsFile, readBuffer);
			numberUEs = stoi(readBuffer);
			readBuffer.clear();
		}
		else numberUEs = 1;			//Consider UE has only one equipment: BS

		//Gets numerology
		getline(readingConfigurationsFile, readBuffer);
		numerology = stoi(readBuffer);
		readBuffer.clear();

		//Gets OFDM GFDM option
		getline(readingConfigurationsFile, readBuffer);
		ofdm_gfdm = stoi(readBuffer);
		readBuffer.clear();

		//Gets Fusion LUT matrix default value (only BS)
		if(flagBS){
			getline(readingConfigurationsFile, readBuffer);
			fLutMatrix = stoi(readBuffer);
			readBuffer.clear();
		}

		//Gets Reception Metrics Periodicity
		getline(readingConfigurationsFile, readBuffer);
		rxMetricPeriodicity = stoi(readBuffer);
		readBuffer.clear();

		//Gets Maximum Transmission Unity in Bytes
		getline(readingConfigurationsFile, readBuffer);
		mtu = stol(readBuffer);
		readBuffer.clear();

		//Gets IP Timeout
		getline(readingConfigurationsFile, readBuffer);
		ipTimeout = stoi(readBuffer);
		readBuffer.clear();

		//The following are only for BS
		if(flagBS){
			//Gets Spectrum Sensing Report waiting Timeout
			getline(readingConfigurationsFile, readBuffer);
			ssreportWaitTimeout = stoi(readBuffer);
			readBuffer.clear();

			//Gets Acknowledgement waiting Timeout
			getline(readingConfigurationsFile, readBuffer);
			ackWaitTimeout = stoi(readBuffer);
			readBuffer.clear();

			//Gets Acknowledgement waiting Timeout
			getline(readingConfigurationsFile, readBuffer);
			rbgSize = stoi(readBuffer);
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
			getline(readingConfigurationsFile, readBuffer);
			ulReservation[i].target_ue_id = stoi(readBuffer);
			readBuffer.clear();
			getline(readingConfigurationsFile, readBuffer);
			ulReservation[i].first_rb = stoi(readBuffer);
			readBuffer.clear();
			getline(readingConfigurationsFile, readBuffer);
			ulReservation[i].number_of_rb = stoi(readBuffer);
			readBuffer.clear();

			//Gets MCS Downlink (only BS)
			if(flagBS){
				getline(readingConfigurationsFile, readBuffer);
				mcsDownlink[i] = stoi(readBuffer);
				readBuffer.clear();
			}

			//Gets MCS Uplink
			getline(readingConfigurationsFile, readBuffer);
			mcsUplink[i] = stoi(readBuffer);
			readBuffer.clear();

			//Gets MIMO configuration
			getline(readingConfigurationsFile, readBuffer);
			mimoConf[i] = stoi(readBuffer);
			readBuffer.clear();
			getline(readingConfigurationsFile, readBuffer);
			mimoDiversityMultiplexing[i] = stoi(readBuffer);
			readBuffer.clear();
			getline(readingConfigurationsFile, readBuffer);
			mimoAntenna[i] = stoi(readBuffer);
			readBuffer.clear();
			getline(readingConfigurationsFile, readBuffer);
			mimoOpenLoopClosedLoop[i] = stoi(readBuffer);
			readBuffer.clear();
			getline(readingConfigurationsFile, readBuffer);
			mimoPrecoding[i] = stoi(readBuffer);
			readBuffer.clear();

			//Gets Transmission Power Control Information
			getline(readingConfigurationsFile, readBuffer);
			transmissionPowerControl[i] = stoi(readBuffer);
			readBuffer.clear();
		}

		readingConfigurationsFile.close();

		if(verbose) cout<<"[CurrentParameters] Reading stored information from file successful."<<endl;
	}
	catch(const exception& e){
		cout<<"Error reading Default.txt file. Exiting..."<<endl;
		exit(1);
	}
}

void
CurrentParameters::recordTxtCurrentParameters(){
	string writeBuffer;		//Buffer that will be used to write on file

	ofstream writingConfigurationsFile;
	writingConfigurationsFile.open("Current.txt", ofstream::out | ofstream::trunc);

	//Writes FlagBS
	writingConfigurationsFile << (int)(flagBS&1) <<'\n';

	//Writes Number of UEs (only BS)
	if(flagBS){
		writingConfigurationsFile << to_string((int)numberUEs) << '\n';
	}

	//Writes numerology
	writingConfigurationsFile << to_string((int)numerology) << '\n';

	//Writes OFDM GFDM option
	writingConfigurationsFile << to_string((int)ofdm_gfdm) << '\n';

	//Writes FUSION LUT matrix values (only BS)
	if(flagBS){
		writingConfigurationsFile << to_string(fLutMatrix) << '\n';
	}

	//Writes Reception Metrics Periodicity
	writingConfigurationsFile << to_string((int)rxMetricPeriodicity) << '\n';

	//Writes Maximum Transmission Unity in Bytes
	writingConfigurationsFile << to_string((int)mtu) << '\n';

	//Writes IP Timeout
	writingConfigurationsFile << to_string((int)ipTimeout) << '\n';

	//The following are only for BS
	if(flagBS){
		//Writes Spectrum Sensing Report waiting Timeout
		writingConfigurationsFile << to_string((int)ssreportWaitTimeout) << '\n';

		//Writes Acknowledgement waiting Timeout
		writingConfigurationsFile << to_string((int)ackWaitTimeout) << '\n';

		//Writes Acknowledgement waiting Timeout
		writingConfigurationsFile << to_string((int)rbgSize) << '\n';
	}

	//Loop to write the parameters numberUEs times
	for(int i=0;i<numberUEs;i++){	
		//Writes UPLinkReservations
		writingConfigurationsFile << to_string((int)ulReservation[i].target_ue_id) << '\n';
		writingConfigurationsFile << to_string((int)ulReservation[i].first_rb) << '\n';
		writingConfigurationsFile << to_string((int)ulReservation[i].number_of_rb) << '\n';

		//Writes MCS Downlink (only BS)
		if(flagBS){
			writingConfigurationsFile << to_string((int)mcsDownlink[i]) << '\n';
		}

		//Writes MCS Uplink
		writingConfigurationsFile << to_string((int)mcsUplink[i]) << '\n';

		//Writes MIMO configuration
		writingConfigurationsFile << to_string((int)mimoConf[i]) << '\n';
		writingConfigurationsFile << to_string((int)mimoDiversityMultiplexing[i]) << '\n';
		writingConfigurationsFile << to_string((int)mimoAntenna[i]) << '\n';
		writingConfigurationsFile << to_string((int)mimoOpenLoopClosedLoop[i]) << '\n';
		writingConfigurationsFile << to_string((int)mimoPrecoding[i]) << '\n';

		//Gets Transmission Power Control Information
		writingConfigurationsFile << to_string((int)transmissionPowerControl[i]) << '\n';
	}

	writingConfigurationsFile.close();

	if(verbose) cout<<"[CurrentParameters] Writing Current information into file successful."<<endl;
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
CurrentParameters::getMTU(){
	return mtu;
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
CurrentParameters::getRbgSize(){
	return rbgSize;
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

bool
CurrentParameters::areUesOutdated(){
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);
    
	return flagUesOutdated;
}

MacModes
CurrentParameters::getMacMode()
{
	return currentMacMode;
}

MacTxModes
CurrentParameters::getMacTxMode()
{
	return currentMacTxMode;
}

MacRxModes
CurrentParameters::getMacRxMode()
{
	return currentMacRxMode;
}

MacTunModes
CurrentParameters::getMacTunMode()
{
	return currentMacTunMode;
}

void 
CurrentParameters::setSystemParameters(
	DynamicParameters* dynamicParameters)	//Pointer to DynamicParameters object, which stores parameters modified
{
	//(Only BS) Sets Fusion LookUp Table
	if(flagBS){
		fLutMatrix = dynamicParameters->getFLUTMatrix();
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

void
CurrentParameters::setUEParameters(
	DynamicParameters* dynamicParameters)	//Pointer to DynamicParameters object, which stores parameters modified
{
	//The only diference from setCLIParameters is MCS Uplink
	mcsUplink[0] = dynamicParameters->getMcsUplink(ulReservation[0].target_ue_id);

	setCLIParameters(dynamicParameters);
}

void
CurrentParameters::setFlagUesOutdated(
	bool _flagUesOutdated)	//New flag value
{
    //Lock mutex till the end of procedure
    lock_guard<mutex> lk(dynamicParametersMutex);
    
	flagUesOutdated = _flagUesOutdated;
}

void 
CurrentParameters::setMacMode(
	MacModes macMode)			//New Mac execution mode
{
	currentMacMode = macMode;
}

void 
CurrentParameters::setMacTxMode(
	MacTxModes macTxMode)			//New Mac Tx sub-execution mode
{
	currentMacTxMode = macTxMode;
}

void 
CurrentParameters::setMacRxMode(
	MacRxModes macRxMode)			//New Mac Rx sub-execution mode
{
	currentMacRxMode = macRxMode;
}

void 
CurrentParameters::setMacTunMode(
	MacTunModes macTunMode)			//New Mac Tun sub-execution mode
{
	currentMacTunMode = macTunMode;
}