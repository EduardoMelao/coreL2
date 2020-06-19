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
     * @brief Spectrum allocation procedure for BS
     * @param ueIds UEIDs for next transmission
     * @param numberSDUs Number of SDUs on buffer for each UE
     * @param numberBytes Number of total Bytes on buffer for each UE
     * @param allocations Vector where allocations will be stored
     */
    void scheduleRequestBS(vector<uint8_t> ueIds, vector<size_t> numberSDUs, vector<size_t> numberBytes, vector<allocation_cfg_t> &allocations);

    /**
     * @brief Spectrum allocation procedure for UE
     * @param numberSDUs Number of SDUs on buffer for BS
     * @param numberBytes Number of total Bytes on buffer for BS
     * @param allocations Spectrum allocation
     */
    void scheduleRequestUE(size_t numberSDUs, size_t numberBytes, allocation_cfg_t &allocation);

    /**
     * @brief Fills MacPdu with SDUs and information to PHY
     * @param macPdus Vector of MacPDUs to be filled
     */
    void fillMacPdus(vector<MacPDU> &macPdus);

    /**
     * @brief Procedure thar calculates spectrum allocation for 2 UEs (and only) for schedulingRequest procedure for downlink
     * @param macPDUs Array of MAC PDUs where scheduler will store Multiplexed SDUs
     * @param fusionLutValue Fusion Lookup Table value: array of 4 least significat bits
     */
    void calculateDownlinkSpectrumAllocation(MacPDU** macPdus, uint8_t fusionLutValue);
};

#endif //INCLUDED_SCHEDULER_H