/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_COSORA_H
#define INCLUDED_COSORA_H

#include <stdint.h>     //uint8_t

/**
 * @brief Class regarding Collaborative Spectrum Sensing Optimized for Rural Areas 
 */
class Cosora{
private:
public:
    /**
     * @brief Constructor for Cosora class
     */
    Cosora();

    /**
     * @brief Destructor for Cosora class
     */
    ~Cosora();

    /**
     * @brief Performs calculation of Spectrum Sensing base on measurements from PHY
     * @param spectrumSensingMeasurement (#TODO) Verify if it is an array of 4 bits with measurements from PHY
     * @returns Array of 4 bits with information about spectrum sensing
     */
    static uint8_t calculateSpectrumSensingValue(uint8_t spectrumSensingMeasurement);

    /**
     * @brief Calculates number of RBs in idle
     * @param spectrumSensingReport from UE
     * @returns Number of RBs in idle
     */
    static uint16_t spectrumSensingConvertToRBIdle(uint8_t spectrumSensingReport);
};

#endif  //INCLUDED_COSORA_H