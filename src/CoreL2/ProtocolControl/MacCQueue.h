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
    MacCQueue();
    ~MacCQueue();
    ssize_t getAck(char* buf);
    ssize_t getControlSduULMCS(char* buf);
    ssize_t getControlSduCSI(char* buf);
};