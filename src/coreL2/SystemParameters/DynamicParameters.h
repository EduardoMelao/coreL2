/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_DYNAMIC_PARAMETERS_H
#define INCLUDED_DYNAMIC_PARAMETERS_H

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

/**
 * @brief Class to store Dynamic Parameters as provided in spreadsheet L1-L2_InterfaceDefinition.xlsx.
 */
class DynamicParameters{
private:
protected:
    //Dynamic information as informed on spreadsheet L1-L2_InterfaceDefinition.xlsx
	uint8_t fLutMatrix;							//[4 bits] BitMap from Fusion Spectrum Analysis
	vector<allocation_cfg_t> ulReservation;	    //[24 bits each] Spectrum allocation for Uplink
	vector<uint8_t> mcsDownlink;				//[4 bit each] Modulation and Coding Scheme for Downlink
	vector<uint8_t> mcsUplink;					//[4 bit each] Modulation and Coding Scheme for Uplink
	vector<uint8_t> mimoConf;					//[1 bit each] 0 = SISO; 1 = MIMO
	vector<uint8_t> mimoDiversityMultiplexing;	//[1 bit each] 0 = Diversity; 1 = Multiplexing
	vector<uint8_t> mimoAntenna;				//[1 bit each] 0 = 2x2; 1 = 4x4
	vector<uint8_t> mimoOpenLoopClosedLoop;		//[1 bit each] 0 = Open Loop; 1 = Closed Loop
	vector<uint8_t> mimoPrecoding;				//[4 bits each] MIMO codeblock configuration for DL and UL
	vector<uint8_t> transmissionPowerControl;	//[6 bits each] Transmission Power Control
	uint8_t rxMetricPeriodicity;				//[4 bits each] CSI period for CQI, PMI and SSM provided by PHY
	mutex dynamicParametersMutex;				//Mutex to control access of parameters
	bool verbose;								//Verbosity flag

public:
	/**
	 * @brief Default constructor
	 */
	DynamicParameters() { }

    /**
	 *@brief Empty constructor for Dynamic Variables
	 *@param _verbose Verbosity flag
	 */
    DynamicParameters(bool _verbose);

    /**
	 * @brief Destructs Dynamic Parameters object
	 */
	~DynamicParameters();

    /**
	 * @brief Initialize all variables with dynamic information on BS
	 * @param _fLutMatrix BitMap from Fusion Spectrum Analisys
	 * @param _ulReservation Spectrum allocation for Uplink
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
	void fillDynamicVariables(uint8_t _fLutMatrix, vector<allocation_cfg_t> _ulReservations, vector<uint8_t> _mcsDownlink, vector<uint8_t> _mcsUplink, vector<uint8_t> _mimoConf, vector<uint8_t> _mimoDiversityMultiplexing,
						vector<uint8_t> _mimoAntenna, vector<uint8_t> _mimoOpenLoopClosedLoop, vector<uint8_t> _mimoPrecoding, vector<uint8_t> _transmissionPowerControl, uint8_t _rxMetricPeriodicity);

    /**
	 * @brief Initialize all variables with dynamic information on UE
	 * @param _ulReservation Spectrum allocation for Uplink
	 * @param _mcsUplink Modulation and Coding Scheme for Uplink
	 * @param _mimoConf SISO(0) or MIMO(1) flag
	 * @param _mimoDiversityMultiplexing Diversity(0) or Multiplexing(1) flag
	 * @param _mimoAntenna MIMO 2x2(0) or 4x4(1) antenna scheme
	 * @param _mimoOpenLoopClosedLoop MIMO 0 = Open Loop; 1 = Closed Loop
	 * @param _mimoPrecoding MIMO codeblock configuration for DL and UL
	 * @param _transmissionPowerControl Transmission Power Control
	 * @param _rxMetricPeriodicity CSI period for CQI, PMI and SSM provided by PHY
	 */
	void fillDynamicVariables(allocation_cfg_t _ulReservation, uint8_t _mcsUplink, uint8_t _mimoConf, uint8_t _mimoDiversityMultiplexing, uint8_t _mimoAntenna, 
							uint8_t _mimoOpenLoopClosedLoop, uint8_t _mimoPrecoding, uint8_t _transmissionPowerControl, uint8_t _rxMetricPeriodicity);

    /**
	 * @brief Seriaizes all variables to a sequence of bytes to be transmitted to UE
	 * @param targetUeId Target UE Identification
	 * @param bytes Vector  where bytes will be stored
	 */
	void serialize(uint8_t targetUeId, vector<uint8_t> & bytes);

    /**
	 * @brief Deserialize bytes to initialize all variables with dynamic information from MACC SDU
	 * @param bytes Bytes with all variables information serialized
	 */
	void deserialize(vector<uint8_t> & bytes);

    //SETTERS
    /**
	 * @brief Sets Fusion Lookup Matrix
	 * @param _fLutMatrix BitMap from Fusion Spectrum Analisys
	 */
	void setFLutMatrix(uint8_t _fLutMatrix);

	/**
	 * @brief Sets Uplink Reservation
	 * @param _ulReservation Spectrum allocation for Uplink
	 */
	void setUlReservation(allocation_cfg_t _ulReservation);

	/**
	 * @brief Sets MCS for Downlink
     * @param macAddress UE MAC Address
	 * @param _mcsDownlink Modulation and Coding Scheme for Downlink
	 */
	void setMcsDownlink(uint8_t macAddress, uint8_t _mcsDownlink);
	
	/**
	 * @brief Sets MCS for Uplink
     * @param macAddress UE MAC Address
	 * @param _mcsUplink Modulation and Coding Scheme for Uplink
	 */
	void setMcsUplink(uint8_t macAddress, uint8_t _mcsUplink);

	/**
	 * @brief Sets MIMO configurations
     * @param macAddress UE MAC Address
	 * @param _mimoConf SISO(0) or MIMO(1) flag
	 * @param _mimoDiversityMultiplexing Diversity(0) or Multiplexing(1) flag
	 * @param _mimoAntenna MIMO 2x2(0) or 4x4(1) antenna scheme
	 * @param _mimoOpenLoopClosedLoop MIMO 0 = Open Loop; 1 = Closed Loop
	 * @param _mimoPrecoding MIMO codeblock configuration for DL and UL
	 */
	void setMimo(uint8_t macAddress, uint8_t _mimoConf, uint8_t _mimoDiversityMultiplexing, uint8_t _mimoAntenna, uint8_t _mimoOpenLoopClosedLoop, uint8_t _mimoPrecoding);

	/**
	 * @brief Sets TPC
     * @param macAddress UE MAC Address
	 * @param _transmissionPowerControl Transmission Power Control
	 */
	void setTPC(uint8_t macAddress, uint8_t _transmissionPowerControl);

	/**
	 * @brief Sets RX Metrics Periodicity
	 * @param _rxMetricPeriodicity CSI period for CQI, PMI and SSM provided by PHY
	 */
	void setRxMetricPeriodicity(uint8_t _rxMetricPeriodicity);

    //GETTERS
    /**
     * @brief Gets index referent to macAddress on all class arrays
     * @param macAddress MAC Address 
     * @param returns -1 if not found; index of macAddress in arrays
     */
    int getIndex(uint8_t macAddress);

    /**
     * @brief Gets Fusion Lookup Table matrix
     * @returns 8-bit integer containing array of 4 bits of Fusion LUT (least significant bits)
     */
    uint8_t getFLUTMatrix();

    /**
     * @brief Getter for Ul Reservation
     * @param macAddress UE MAC Address
     * @returns Uplink reservation of UE identified as parameter
     */
    allocation_cfg_t getUlReservation(uint8_t macAddress);

	/**
	 * @brief Getter for all Ul Reservations
	 * @param _ulReservations Vector where UL Reservations will be stored
	 */
	void getUlReservations(vector<allocation_cfg_t> & _ulReservations);

    /**
     * @brief Getter for MCS Downlink
     * @param macAddress UE MAC Address
     * @returns MCS Downlink of UE identified as parameter
     */
    uint8_t getMcsDownlink(uint8_t macAddress);

    /**
     * @brief Getter for MCS Uplink
     * @param macAddress UE MAC Address
     * @returns MCS Uplink of UE identified as parameter
     */
    uint8_t getMcsUplink(uint8_t macAddress);

    /**
     * @brief Getter for MIMO Configuration
     * @param macAddress UE MAC Address
     * @returns MIMO Configuration of UE identified as parameter
     */
    uint8_t getMimoConf(uint8_t macAddress);

    /**
     * @brief Getter for MIMO Diversity or Multiplexing flag
     * @param macAddress UE MAC Address
     * @returns MIMO Diversity or Multiplexing flag of UE identified as parameter
     */
    uint8_t getMimoDiversityMultiplexing(uint8_t macAddress);

    /**
     * @brief Getter for MIMO Antennas number
     * @param macAddress UE MAC Address
     * @returns MIMO Antennas number of UE identified as parameter
     */
    uint8_t getMimoAntenna(uint8_t macAddress);

    /**
     * @brief Getter for MIMO OpenLoop or ClosedLoop flag
     * @param macAddress UE MAC Address
     * @returns MIMO OpenLoop or ClosedLoop flag of UE identified as parameter
     */
    uint8_t getMimoOpenLoopClosedLoop(uint8_t macAddress);

    /**
     * @brief Getter for MIMO Precoding index
     * @param macAddress UE MAC Address
     * @returns MIMO Precoding index of UE identified as parameter
     */
    uint8_t getMimoPrecoding(uint8_t macAddress);

    /**
     * @brief Getter for Transmission Power Control
     * @param macAddress UE MAC Address
     * @returns Transmission Power Control of UE identified as parameter
     */
    uint8_t getTPC(uint8_t macAddress);

    /**
     * @brief Getter for Rx Metrics Periodicity
     * @returns Rx Metrics Periodicity of System
     */
    uint8_t getRxMetricsPeriodicity();

};

#endif  //INCLUDED_DYNAMIC_PARAMETERS_H
