/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_LINK_ADAPTATION_H
#define INCLUDED_LINK_ADAPTATION_H

#include <stdint.h>     //uint8_t

/**
 * @brief Class to perform Link Adaptation calculations such as converting SNR to MCS
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
     * @brief Based on SNR, performs calculation of MCS
     * @returns Modulation and Coding Scheme
     */
    static uint8_t getSnrConvertToMcs(float snr);
};
#endif  //INCLUDED_LINK_ADAPTATION_H