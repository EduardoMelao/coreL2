/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_ADAPTIVE_MODULATION_CODING_H
#define INCLUDED_LINK_ADAPTATION_H

#include <stdint.h>     //uint8_t

/**
 * @brief Class to perform Adaptive Modulation and Coding calculations such as converting CQI-UL to Uplink-MCS
 */
class AdaptiveModulationCoding{
private:
public:
    /**
     * @brief Constructor for Adaptive Modulation and Coding
     */
    AdaptiveModulationCoding();

    /**
     * @brief Destructor for Adaptive Modulation and Coding
     */
    ~AdaptiveModulationCoding();

    /**
     * @brief Based on CQI Uplink, performs calculation of Uplink MCS
     * @returns Uplink Modulation and Coding Scheme
     */
    static uint8_t getCqiUplinkConvertToUplinkMcs(uint8_t cqiUplink);
};

#endif  //INCLUDED_ADAPTIVE_MODULATION_CODING_H
