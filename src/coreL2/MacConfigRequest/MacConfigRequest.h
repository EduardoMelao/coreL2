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

class MacConfigRequest{
private:
	bool verbose;							//Verbosity flag
public:
	DynamicParameters* dynamicParameters;	//Dynamic Parameters as defined in Spreadsheet L1-L2_InterfaceDefinition.xlsx.

	/**
	 *@brief Empty constructor for Dynamic Variables
	 *@param _verbose Verbosity flag
	 */
	MacConfigRequest(bool _verbose);

	/**
	 * @brief Destructs MacConfigRequest object
	 */
	~MacConfigRequest();
};

#endif  //INCLUDED_MAC_CONFIG_REQUEST_H
