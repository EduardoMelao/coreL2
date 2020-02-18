/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_CURRENT_PARAMETERS
#define INCLUDED_CURRENT_PARAMETERS

#include <iostream>
#include <fstream>		//File stream
#include <string.h>
#include <vector>
#include <cstdlib>
using namespace std;

#include "../../common/lib5grange/lib5grange.h"
#include "../../common/libMac5gRange/libMac5gRange.h"
#include "../SystemParameters/DynamicParameters.h"
using namespace lib5grange;

/**
 * @brief Class to store Dynamic Parameters as provided in spreadsheet L1-L2_InterfaceDefinition.xlsx.
 */
class CurrentParameters : public DynamicParameters{
private:
	bool flagBS;					//Flag to indicate if current equipment is BS or UE
	bool verbose;					//Verbosity flag

	//Static(only) information as described on spreadsheet L1-L2_InterfaceDefinition.xlsx
	uint8_t numberUEs;				//[4 bits] Number of UserEquipments attached (ignore in case of UEs);
	uint8_t numerology;				//[3 bits] Numerology identification
	uint8_t ofdm_gfdm;				//[1 bit] Flag to indicate data transmission technique. 0=OFDM/1=GFDM
	uint16_t mtu;					//[16 bits] Maximum transmission unity of the system
	uint16_t ipTimeout;				//[16 bits] IP Timeout time (milliseconds)
	uint8_t ssreportWaitTimeout;	//[4 bits] Spectrum Sensing Report Wait Timeout in number of subframes
	uint8_t ackWaitTimeout;			//[4 bits] Acknowledgement Wait Timeout time in number of subframes

public:
	/**
	 * @brief Empty constructor
	 */
	CurrentParameters();

	/**
	 * @brief Constructs object and read file to initialize all variables with persisted information
	 * @param _verbose Verbosity flag
	 */
	CurrentParameters(bool _verbose);

	/**
	 * @brief Destroys Static Default Parameters object
	 */
	~CurrentParameters();

	/**
	 * @brief Reads TXT archive with system parameters information and stores it in class variables
	 * @param fileName Name of the file to be read
	 */
	void readTxtSystemParameters(string fileName);

	/**
	 * @brief Writes into TXT archive all current System parameters
	 */
	void recordTxtCurrentParameters();

	/**
	 * @brief Loads a Dynamic Parameters Object with default information read from file
	 * @param dynamicParameters DynamicParameters object with dynamic parameters to be filled
	 */
	void loadDynamicParametersDefaultInformation(DynamicParameters* dynamicParameters);

	//Getters
	/**
	 * @brief Gets flagBS
	 * @returns True if it is BS; False if it is UE
	 */
	bool isBaseStation();

	/**
	 * @brief Gets number of User Equipments attached
	 * @returns Number of UEs
	 */
	uint8_t getNumberUEs();

	/**
	 * @brief Gets System numerology
	 * @returns numerology
	 */
	uint8_t getNumerology();

	/**
	 * @brief Gets OFDM/GFDM flag 
	 * @returns True if GFDM; False if OFDM
	 */
	bool isGFDM();

	/**
	 * @brief Gets Maximum Transmission Unit of the System
	 * @returns MTU
	 */
	uint16_t getMTU();

	/**
	 * @brief Gets IP packets timeout in milliseconds
	 * @returns Timeout of IP packets
	 */
	uint16_t getIpTimeout();

	/**
	 * @brief Gets Spectrum Sensing Report waiting Timeout in number of subframes
	 * @returns SS Report Wait Timeout
	 */
	uint8_t getSSReportWaitTimeout();

	/**
	 * @brief Gets ACK Waiting Timeout in number of subframes
	 * @returns ACK Waiting Timeout
	 */
	uint8_t getACKWaitTimeout();

	/**
	 * @brief Gets current MAC Address
	 * @returns Current MAC Address
	 */
	uint8_t getCurrentMacAddress();

	/**
	 * @brief Gets MAC Address based on index passed as parameter
	 * @param index Index corresponding to the position of MAC Address in arrays
	 * @returns Corresponding MAC Address
	 */
	uint8_t getMacAddress(int index);

	//SETTERS

	/**
	 * @brief Sets Current Parameters according to dynamic parameters modified by system (ULMCS, DLMCS and fLutMatrix)
	 * @param dynamicParameters Pointer to DynamicParameters object, which stores parameters modified
	 */
	void setSystemParameters(DynamicParameters* dynamicParameters);

	/**
	 * @brief Sets Current Parameters according to dynamic parameters modified by CLI
	 * @param dynamicParameters Pointer to DynamicParameters object, which stores parameters modified
	 */
	void setCLIParameters(DynamicParameters* dynamicParameters);

	/**
	 * @brief [ONLY UE] Sets Current Parameters according to dynamic parameters sent by BS
	 * @param dynamicParameters Pointer to DynamicParameters object, which stores parameters modified
	 */
	void setUEParameters(DynamicParameters* dynamicParameters);
};


#endif	//INCLUDED_CURRENT_PARAMETERS
