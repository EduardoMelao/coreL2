/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_LINK_ADAPTATION_H
#define INCLUDED_LINK_ADAPTATION_H

#include <stdint.h>     //uint8_t

#include "../AdaptiveModulationCoding/AdaptiveModulationCoding.h"

/**
 * @brief Class to perform Link Adaptation calculations such as converting SINR to CQI-UL
 */
class LinkAdaptation{
private:
public:
    /**
     * @brief Constructor for Link Adaptation 
     */
    LinkAdaptation();

    /**
     * @brief Destructor for Link Adaptation
     */
    ~LinkAdaptation();

    /**
     * @brief Based on SINR, performs calculation of Channel Quality Information for Uplink and Uplink MCS
     * @returns Uplink Modulation and Coding Scheme
     */
    static uint8_t getSinrConvertToCqiUplink(float sinr);
};
#endif  //INCLUDED_LINK_ADAPTATION_H