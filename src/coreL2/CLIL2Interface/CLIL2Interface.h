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
#include "../SystemParameters/DynamicParameters.h"
using namespace lib5grange;

/**
 * @brief Class to interact with CLI
 */
class CLIL2Interface{
private:
	bool macStartCommandFlag;			//Flag to control CLI's request of MacStartCommand
	bool macStopCommandFlag;			//Flag to control CLI's request of MacStopCommand
	bool macConfigRequestCommandFlag;	//Flag to control CLI's request of MacConfigRequestCommand
	bool verbose;						//Verbosity flag

public:
	DynamicParameters* dynamicParameters;	//Dynamic Parameters as defined in Spreadsheet L1-L2_InterfaceDefinition.xlsx.

	/**
	 *@brief Empty constructor for CLI interface with L2
	 *@param _verbose Verbosity flag
	 */
	CLIL2Interface(bool _verbose);

	/**
	 * @brief Destructs CLIL2Interface object
	 */
	~CLIL2Interface();

	/**
	 * @brief This procedure is called when MACStartCommand is triggered in CLI
	 */
	void macStartCommand();

	/**
	 * @brief This procedure is called when MACStopCommand is triggered in CLI
	 */
	void macStopCommand();

	/**
	 * @brief This procedure is called when MACConfigRequestCommand is triggered in CLI
	 */
	void macConfigRequestCommand();

	//Flag Setters
	/**
	 * @brief Sets MacStartCommandFlag value
	 * @param _macStartCommandFlag New flag value
	 */	
	void setMacStartCommandFlag(bool _macStartCommandFlag);

	/**
	 * @brief Sets MacStopCommandFlag value
	 * @param _macStopCommandFlag New flag value
	 */	
	void setMacStopCommandFlag(bool _macStopCommandFlag);

	/**
	 * @brief Sets MacConfigRequestCommandFlag value
	 * @param _macConfigRequestCommandFlag New flag value
	 */	
	void setMacConfigRequestCommandFlag(bool _macConfigRequestCommandFlag);

	//Flag Getters
	/**
	 * @brief Gets MacStartCommandFlag current value
	 * @returns Flag value
	 */
	bool getMacStartCommandFlag();

	/**
	 * @brief Gets MacStopCommandFlag current value
	 * @returns Flag value
	 */
	bool getMacStopCommandFlag();

	/**
	 * @brief Gets MacConfigRequestCommandFlag current value
	 * @returns Flag value
	 */
	bool getMacConfigRequestCommandFlag();
};

#endif  //INCLUDED_MAC_CONFIG_REQUEST_H
