#pragma once

#include <iostream> //cin, cout, endl
#include <thread>   //sleep
#include <chrono>   //seconds
#include <future>   //atomic_bool
#include <string.h> //memcpy

#define MAXSIZE 50

using namespace std;

class MacCQueue{
private:
public:
    MacCQueue();
    ~MacCQueue();
    ssize_t getAck(char* buf);
    ssize_t getControlSduULMCS(char* buf);
    ssize_t getControlSduCSI(char* buf);
};