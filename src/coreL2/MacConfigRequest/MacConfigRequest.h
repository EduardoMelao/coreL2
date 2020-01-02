/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_MAC_CONFIG_REQUEST_H
#define INCLUDED_MAC_CONFIG_REQUEST_H

#include <iostream>
#include <fstream>		//File stream
#include <string.h>
#include <vector>
#include <cstdlib>
#include <mutex>
using namespace std;

#include "../../common/lib5grange/lib5grange.h"
#include "../../common/libMac5gRange/libMac5gRange.h"
using namespace lib5grange;

class MacConfigRequest{
private:
	//Dynamic information as informed on spreadsheet L1-L2_InterfaceDefinition.xlsx
	bool modified;                              //Flag to indicate if there's information changed to send to UE via MACC SDU
    uint8_t fLutMatrix[17];						//[132 bits] BitMap from Fusion Spectrum Analysis
	vector<allocation_cfg_t> ulReservations;	//[24 bits each] Spectrum allocation for Uplink
	uint8_t mcsDownlink;						//[4 bit] Modulation and Coding Scheme for Downlink
	uint8_t mcsUplink;							//[4 bit] Modulation and Coding Scheme for Uplink
	uint8_t mimoConf;							//[1 bit] 0 = SISO; 1 = MIMO
	uint8_t mimoDiversityMultiplexing;			//[1 bit] 0 = Diversity; 1 = Multiplexing
	uint8_t mimoAntenna;						//[1 bit] 0 = 2x2; 1 = 4x4
	uint8_t mimoOpenLoopClosedLoop;				//[1 bit] 0 = Open Loop; 1 = Closed Loop
	uint8_t mimoPrecoding;						//[4 bits] MIMO codeblock configuration for DL and UL
	uint8_t transmissionPowerControl;			//[6 bits] Transmission Power Control
	uint8_t rxMetricPeriodicity;				//[4 bits] CSI period for CQI, PMI and SSM provided by PHY
	bool verbose;								//Verbosity flag

public:
	mutex dynamicParametersMutex;				//Mutex to control access and alterations on dynamic parameters 

	/**
	 *@brief Empty constructor for Dynamic Variables
	 *@param _verbose Verbosity flag
	 */
	MacConfigRequest(bool _verbose);

	/**
	 * @brief Constructor on UE side to initialize all variables with dynamic information from MACC SDU
	 * @param bytes Bytes with all variables information serialized
	 */
	MacConfigRequest(vector<uint8_t> & bytes);

	/**
	 * @brief Destructs MacConfigRequest object
	 */
	~MacConfigRequest();

	/**
	 * @brief Initialize all variables with dynamic information
	 * @param _fLutMatrix BitMap from Fusion Spectrum Analisys
	 * @param _ulReservations Spectrum allocation for Uplink
	 * @param _mcsDownlink Modulation and Coding Scheme for Downlink
	 * @param _mcsUplink Modulation and Coding Scheme for Uplink
	 * @param _mimoConf SISO(0) or MIMO(1) flag
	 * @param _mimoDiversityMultiplexing Diversity(0) or Multiplexing(1) flag
	 * @param _mimoAntenna MIMO 2x2(0) or 4x4(1) antenna scheme
	 * @param _mimoOpenLoopClosedLoop MIMO 0 = Open Loop; 1 = Closed Loop
	 * @param _mimoPrecoding MIMO codeblock configuration for DL and UL
	 * @param _transmissionPowerControl Transmission Power Control
	 * @param _rxMetricPeriodicity CSI period for CQI, PMI and SSM provided by PHY
	 */
	void fillDynamicVariables(uint8_t* _fLutMatrix, vector<allocation_cfg_t> _ulReservations, uint8_t _mcsDownlink, uint8_t _mcsUplink, uint8_t _mimoConf, uint8_t _mimoDiversityMultiplexing,
						uint8_t _mimoAntenna, uint8_t _mimoOpenLoopClosedLoop, uint8_t _mimoPrecoding, uint8_t _transmissionPowerControl, uint8_t _rxMetricPeriodicity);

	/**
	 * @brief Sets Fusion Lookup Matrix
	 * @param _fLutMatrix BitMap from Fusion Spectrum Analisys
	 */
	void setFLutMatrix(uint8_t* _fLutMatrix);

	/**
	 * @brief Sets Uplink Reservations
	 * @param _ulReservations Spectrum allocation for Uplink
	 */
	void setUlReservations(vector<allocation_cfg_t> _ulReservations);

	/**
	 * @brief Sets MCS for Downlink
	 * @param _mcsDownlink Modulation and Coding Scheme for Downlink
	 */
	void setMcsDownlink(uint8_t _mcsDownlink);
	
	/**
	 * @brief Sets MCS for Uplink
	 * @param _mcsUplink Modulation and Coding Scheme for Uplink
	 */
	void setMcsUplink(uint8_t _mcsUplink);

	/**
	 * @brief Sets MIMO configurations
	 * @param _mimoConf SISO(0) or MIMO(1) flag
	 * @param _mimoDiversityMultiplexing Diversity(0) or Multiplexing(1) flag
	 * @param _mimoAntenna MIMO 2x2(0) or 4x4(1) antenna scheme
	 * @param _mimoOpenLoopClosedLoop MIMO 0 = Open Loop; 1 = Closed Loop
	 * @param _mimoPrecoding MIMO codeblock configuration for DL and UL
	 */
	void setMimo(uint8_t _mimoConf, uint8_t _mimoDiversityMultiplexing, uint8_t _mimoAntenna, uint8_t _mimoOpenLoopClosedLoop, uint8_t _mimoPrecoding);

	/**
	 * @brief Sets TPC
	 * @param _transmissionPowerControl Transmission Power Control
	 */
	void setTPC(uint8_t _transmissionPowerControl);

	/**
	 * @brief Sets RX Metrics Periodicity
	 * @param _rxMetricPeriodicity CSI period for CQI, PMI and SSM provided by PHY
	 */
	void setRxMetricPeriodicity(uint8_t _rxMetricPeriodicity);
	
	/**
	 * @brief Sets modified flag to an especific value
	 * @param _modified New modified flag value
	 */
	void setModified(bool _modified);

	/**
	 * @brief Seriaizes all variables to a sequence of bytes to be transmitted
	 * @param targetUeId Target UE Identification
	 * @param bytes Vector  where bytes will be stored
	 */
	void serialize(uint8_t targetUeId, vector<uint8_t> & bytes);

	/**
	 * @brief Test if there are paremeters to transmit
	 * @returns True if there are modified parameters. False otherwise
	 */
	bool isModified();

};

#endif  //INCLUDED_MAC_CONFIG_REQUEST_H
