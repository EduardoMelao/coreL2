/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_COSORA_H
#define INCLUDED_COSORA_H

#include <stdint.h>     //uint8_t
#include <chrono>       //chrono::microseconds()
#include <thread>       //thread() ; this_thread namespace
#include <mutex>        //mutex

#include "../SystemParameters/CurrentParameters.h"      //DynamicParameters/CurrentParameters
#include "../../common/libMac5gRange/libMac5gRange.h"   //Mac Modes

using namespace std;

/**
 * @brief Class regarding Collaborative Spectrum Sensing Optimized for Rural Areas 
 */
class Cosora{
private:
    bool isActive;                          //Flag that indicates if Fusion is activated
    bool isWaiting;                         //Flag that indicates that Cosora is waiting for SS Reports from UEs
    unsigned int timeout;                   //Spectrum Sensing Wait timeout in microseconds
    uint8_t fusionLookupTable;              //Fusion Lookup table with 4bits of information
    DynamicParameters* dynamicParameters;   //Dynamic Parameters from Basestation
    CurrentParameters* currentParameters;   //Current System Parameters
    mutex fusionMutex;                      //Mutex to control Fusion calculation while waiting
    bool verbose;                           //Verbosity flag

public:
    /**
     * @brief Constructor for Cosora class
     * @param _dynamicParameters DynamicParameters container from main system to store temporary modifications
     * @param _currentParameters Parameters currently being used by the System
     * @param _verbose Verbosity flag value
     */
    Cosora(DynamicParameters* _dynamicParameters, CurrentParameters* _currentParameters, bool _verbose);

    /**
     * @brief Destructor for Cosora class
     */
    ~Cosora();

    /**
     * @brief Performs calculation of Spectrum Sensing base on measurements from PHY
     * @param spectrumSensingMeasurement Array of 4 bits with measurements from PHY
     * @returns Array of 4 bits with information about spectrum sensing
     */
    static uint8_t calculateSpectrumSensingValue(uint8_t spectrumSensingMeasurement);

    /**
     * @brief Calculates number of RBs in idle
     * @param spectrumSensingReport SS Report from UE
     * @returns Number of RBs in idle
     */
    static uint16_t spectrumSensingConvertToRBIdle(uint8_t spectrumSensingReport);

    /**
     * @brief This procedure waits for SSWaitTimeout subframes receiving SSR from UES and then
     *      calculates the Fusion LUT value
     */
    void spectrumSensingTimeout();

    /**
     * @brief Performs Fusion calculation in Cognitive Cycle
     * @param ssr New Spectrum Sensing Report from UE
     */
    void fusionAlgorithm(uint8_t ssr);

    /**
     * @brief Informs if COSORA is waiting for UEs to send SSR
     * @returns TRUE if COSORA is waiting; FALSE otherwise
     */
    bool isBusy();
};

#endif  //INCLUDED_COSORA_H