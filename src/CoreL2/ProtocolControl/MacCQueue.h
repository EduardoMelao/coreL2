/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#pragma once

#include <iostream> //cin, cout, endl
#include <thread>   //sleep
#include <chrono>   //seconds
#include <future>   //atomic_bool
#include <string.h> //memcpy

#define MAXSIZE 50  //Maximum control message size

using namespace std;

/**
 * @brief Queue to store MAC Control SDUs
 */
class MacCQueue{
private:
public:
    /**
     * @brief Constructs an empty Control Queue
     */
    MacCQueue();

    /**
     * @brief Destroys control queue
     */
    ~MacCQueue();

    /**
     * @brief Gets ACK control message that will be used for UE->BS acknowledgment
     * @param buffer Buffer that will store the message
     * @returns Size of the message in bytes
     */
    ssize_t getAck(char* buffer);

    /**
     * @brief Gets CSI Control SDU
     * @param buffer Buffer that will store the message
     * @returns Size of the message in bytes
     */
    ssize_t getControlSduULMCS(char* buffer);

    /**
     * @brief Gets ULMCS Control SDU
     * @param buffer Buffer that will store the message
     * @returns Size of the message in bytes
     */
    ssize_t getControlSduCSI(char* buffer);
};