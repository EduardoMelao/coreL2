/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_TIMER_SUBFRAME_H
#define INCLUDED_TIMER_SUBFRAME_H

#define SUBFRAME_DURATION 4600      //Subframe duration in nanoseconds

#include <iostream>
#include <chrono>
#include <mutex>

class TimerSubframe{
private:
    unsigned long long subframeCounter;     //Counter to store number of subframes elapsed. Max value: 18446744073709551615ULL
    bool killThread;                        //Boolean to be checked into thread. If false, counting thread is killed
public:
    /**
     * @brief Constructor for TimerSubframe object
     */
    TimerSubframe();

    /**
     * @brief Destructor for TimerSubframe object
     */
    ~TimerSubframe();

    /**
     * @brief Procedure to sleep for a Subframe and then increase counter
     */
    void countingThread();

    /**
     * @brief Gets actual state of subframeCounter
     * @returns Number of current subframe
     */
    unsigned long long getSubframeNumber();

    /**
     * @brief Procedure to end countingThread
     */
    void stopCounting();
};

#endif  //INCLUDED_TIMER_SUBFRAME_H