#ifndef INCLUDED_SCHEDULER_H
#define INCLUDED_SCHEDULER_H

#include <iostream>
#include <vector>

#include "../SystemParameters/CurrentParameters.h"
#include "../Multiplexer/Multiplexer.h"
#include "../SduBuffers/SduBuffers.h"
#include "../../common/lib5grange/lib5grange.h"

class SduBuffers;

using namespace std;
using namespace lib5grange;
/**
 * @brief This class is responsible for scheduling the SDUs for multiple UEs
 *      into multiple MAC PDUs for next transmission.
 */
class Scheduler{
private:
    CurrentParameters* currentParameters;   //System Current up-to-date parameters
    SduBuffers* sduBuffers;                 //Buffers storing MACC and MACD SDUs
    bool verbose;                           //Verbosity flag

    /**
     * @brief Selects the UE(s) for next transmission, based on Buffer Status Information (BSI)
     * @returns Array of indexes of UEs into Current Parameters arrays
     */
    vector<int> selectUEs();

public:
    /**
     * @brief Constructor for Scheduler module. Initializes class variables with system attributes
     * @param _currentParameters System Current execution parameters
     * @param _sduBuffers Buffers responsible to store Control and Data SDUs
     * @param _verbose Verbosity flag
     */
    Scheduler(CurrentParameters* _currentParameters, SduBuffers* _sduBuffers, bool _verbose);

    /**
     * @brief Destructor for Scheduler class
     */
    ~Scheduler();

    /**
     * @brief Scheduling procedure for BS
     * @param macPDUs Array of MAC PDUs where scheduler will store Multiplexed SDUs
     */
    void scheduleRequestBS(MacPDU** macPdus);


    /**
     * @brief Scheduling procedure for UE
     * @param macPDUs MAC PDU where scheduler will store Multiplexed SDUs
     */
    void scheduleRequestUE(MacPDU* macPdu);

    /**
     * @brief Procedure thar calculates spectrum allocation for 2 UEs (and only) for schedulingRequest procedure for downlink
     * @param macPDUs Array of MAC PDUs where scheduler will store Multiplexed SDUs
     * @param fusionLutValue Fusion Lookup Table value: array of 4 least significat bits
     */
    void calculateDownlinkSpectrumAllocation(MacPDU** macPdus, uint8_t fusionLutValue);
};

#endif //INCLUDED_SCHEDULER_H