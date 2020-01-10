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
     * @param spectrumSensingMeasurement (PROVISIONAL)Array of 132 bits with measurements from PHY
     * @param spectrumSensingReport Array of 132 bits with information about spectrum sensing
     */
    static void calculateSpectrumSensingValue(uint8_t* spectrumSensingMeasurement, uint8_t* spectrumSensingReport);

    /**
     * @brief Calculates number of RBs in idle
     * @param spectrumSensingReport from UE
     * @returns Number of RBs in idle
     */
    static uint16_t spectrumSensingConvertToRBIdle(uint8_t* spectrumSensingReport);
};

#endif  //INCLUDED_COSORA_H