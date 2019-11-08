#pragma once
#include <vector>
#include <mutex>
#include "TunInterface.h"

#define MAXLINE 2048
#define SRC_OFFSET 12
#define DST_OFFSET 16

using namespace std;

class MacHighQueue{
private:
    TunInterface* tunIf;
    vector<char*> queue;
    vector<ssize_t> sizes;
    mutex tunMutex;
    bool verbose;
public:
    MacHighQueue(TunInterface* tun, bool v);
    ~MacHighQueue();
    void reading();
    int getNum();
    ssize_t getNextSdu(char* buf);
};