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
#include "../MacConfigRequest/MacConfigRequest.h"
using namespace lib5grange;

class StaticDefaultParameters{
private:
	fstream defaultConfigurationsFile;		//File descriptor to default configuration
	bool verbose;							//Verbosity flag

public:
	//Static information as informed on spreadsheet L1-L2_InterfaceDefinition.xlsx
	bool flagBS;								//Flag to indicate if current equipment is BS or UE
	uint8_t numberUEs;							//[4 bits] Number of UserEquipments attached (ignore in case of UEs);
	vector<allocation_cfg_t> ulReservations;	//[24 bits each] Spectrum allocation for Uplink
	uint8_t numerology;							//[3 bits] Numerology identification
	uint8_t ofdm_gfdm;							//[1 bit] Flag to indicate data transmission technique. 0=OFDM/1=GFDM
	uint8_t mcsDownlink;						//[4 bit] Default Modulation Coding Scheme for Downlink
	uint8_t mcsUplink;							//[4 bit] Default Modulation Coding Scheme for Uplink
	uint8_t mimoConf;							//[1 bit] 0 = SISO; 1 = MIMO
	uint8_t mimoDiversityMultiplexing;			//[1 bit] 0 = Diversity; 1 = Multiplexing
	uint8_t mimoAntenna;						//[1 bit] 0 = 2x2; 1 = 4x4
	uint8_t mimoOpenLoopClosedLoop;				//[1 bit] 0 = Open Loop; 1 = Closed Loop
	uint8_t mimoPrecoding;						//[4 bits] MIMO codeblock configuration for DL and UL
	uint8_t transmissionPowerControl;			//[6 bits] Transmission Power Control
	uint8_t fLutMatrix[17];						//[132 bits] BitMap from Fusion Spectrum Analysis
	uint8_t rxMetricPeriodicity;				//[4 bits] CSI period for CQI, PMI and SSM provided by PHY
	uint16_t mtu;								//[16 bits] Maximum transmission unity of the system
	uint16_t ipTimeout;							//[16 bits] IP Timeout time (milliseconds)
	uint16_t ssreportWaitTimeout;				//[4 bits] Spectrum Sensing Report Wait Timeout time(milliseconds)
	uint16_t ackWaitTimeout;				    //[4 bits] Acknowledgement Wait Timeout time(milliseconds)

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
	 * @brief Loads a Dynamic Parameters Object with default information read from file
	 * @param dynamicParameters MacConfigRequest object with dynamic parameters to be filled
	 */
	void loadDynamicParametersDefaultInformation(MacConfigRequest* dynamicParameters);

};


#endif	//INCLUDED_STATIC_DEFAULT_PARAMETERS
