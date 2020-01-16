/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_STATIC_DEFAULT_PARAMETERS
#define INCLUDED_STATIC_DEFAULT_PARAMETERS

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
 * @brief 
 */
class StaticDefaultParameters{
private:
	fstream defaultConfigurationsFile;		//File descriptor to default configuration
	bool verbose;							//Verbosity flag

public:

	bool flagBS;								//Flag to indicate if current equipment is BS or UE

	//Static information as described on spreadsheet L1-L2_InterfaceDefinition.xlsx
	uint8_t numberUEs;							//[4 bits] Number of UserEquipments attached (ignore in case of UEs);
	vector<uint8_t> numerology;					//[3 bits] Numerology identification
	vector<uint8_t> ofdm_gfdm;					//[1 bit] Flag to indicate data transmission technique. 0=OFDM/1=GFDM
	vector<uint16_t> mtu;						//[16 bits] Maximum transmission unity of the system
	vector<uint16_t> ipTimeout;					//[16 bits] IP Timeout time (milliseconds)
	vector<uint8_t> ssreportWaitTimeout;		//[4 bits] Spectrum Sensing Report Wait Timeout time(milliseconds)
	vector<uint8_t> ackWaitTimeout;				//[4 bits] Acknowledgement Wait Timeout time(milliseconds)

	//Default information as described on spreadsheet L1-L2_InterfaceDefinition.xlsx
	vector<allocation_cfg_t> ulReservations;	//[24 bits each] Spectrum allocation for Uplink
	vector<uint8_t> mcsDownlink;				//[4 bit each] Default Modulation Coding Scheme for Downlink
	vector<uint8_t> mcsUplink;					//[4 bit each] Default Modulation Coding Scheme for Uplink
	vector<uint8_t> mimoConf;					//[1 bit each] 0 = SISO; 1 = MIMO
	vector<uint8_t> mimoDiversityMultiplexing;	//[1 bit each] 0 = Diversity; 1 = Multiplexing
	vector<uint8_t> mimoAntenna;				//[1 bit each] 0 = 2x2; 1 = 4x4
	vector<uint8_t> mimoOpenLoopClosedLoop;		//[1 bit each] 0 = Open Loop; 1 = Closed Loop
	vector<uint8_t> mimoPrecoding;				//[4 bits each] MIMO codeblock configuration for DL and UL
	vector<uint8_t> transmissionPowerControl;	//[6 bits each] Transmission Power Control
	uint8_t fLutMatrix[17];						//[132 bits] BitMap from Fusion Spectrum Analysis
	vector<uint8_t> rxMetricPeriodicity;		//[4 bits each] CSI period for CQI, PMI and SSM provided by PHY

	/**
	 * @brief Constructs object and read file to initialize all variables with default information
	 * @param _verbose Verbosity flag
	 */
	StaticDefaultParameters(bool _verbose);

	/**
	 * @brief Destroys Static Default Parameters object
	 */
	~StaticDefaultParameters();

	/**
	 * @brief Reads TXT archive with static & default information and starts class variables
	 */
	void readTxtStaticParameters();

	/**
	 * @brief Loads a Dynamic Parameters Object with default information read from file
	 * @param dynamicParameters DynamicParameters object with dynamic parameters to be filled
	 */
	void loadDynamicParametersDefaultInformation(DynamicParameters* dynamicParameters);
};


#endif	//INCLUDED_STATIC_DEFAULT_PARAMETERS
