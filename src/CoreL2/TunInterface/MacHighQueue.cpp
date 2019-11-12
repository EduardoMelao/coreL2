/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "MacHighQueue.h"

/**
 * @brief Constructs an empty MacHighQueue with a TUN descriptor
 * @param tun TUN Interface object
 * @param v Verbosity flag 
 */
MacHighQueue::MacHighQueue(TunInterface* tun, bool v){
    tunIf = tun;
    verbose = v;
}

/**
 * @brief Destroys MacHighQueue
 */
MacHighQueue::~MacHighQueue(){ }

/**
 * @brief Proceeding that executes forever, receiving packets from L3 and storing them in the queue
 */
void 
MacHighQueue::reading(){
    char *buf;
    ssize_t nread = 0;
    while(1){
        //Allocate buffer
        buf = new char[MAXLINE];
        bzero(buf, MAXLINE);

        //Read from TUN Interface
        nread = tunIf->readTunInterface(buf, MAXLINE);
        {
            //Lock to write in the queue
            lock_guard<mutex> lk(tunMutex);
            
            //Check EOF
            if(nread==0)
                break;

            //Check ipv4
            if(((buf[0]>>4)&15) != 4){
                if(verbose) cout<<"[MacHighQueue] Dropped non-ipv4 packet."<<endl;
                continue;
            }

            //Check broadcast
            if((buf[DST_OFFSET]==255)&&(buf[DST_OFFSET+1]==255)&&(buf[DST_OFFSET+2]==255)&&(buf[DST_OFFSET+3]==255)){
                if(verbose) cout<<"[MacHighQueue] Dropped broadcast packet."<<endl;
                continue;
            }

            //Check multicast
            if((buf[DST_OFFSET]>=224)&&(buf[DST_OFFSET]<=239)){
                if(verbose) cout<<"[MacHighQueue] Dropped multicast packet."<<endl;
                continue;
            }
            
            //Everything is ok, buffer can be added to queue
            queue.push_back(buf);
            sizes.push_back(nread);
            if(verbose) cout<<"[MacHighQueue] SDU added to Queue. Num SDUs: "<<queue.size()<<endl;
        }
    }
}

/**
 * @brief Gets number of packets that are currently enqueued
 * @returns Number of packets enqueued
 */
int 
MacHighQueue::getNum(){
    //Lock mutex to consult queue size information
    lock_guard<mutex> lk(tunMutex);
    return (int)queue.size();
}

/**
 * @brief Gets next SDU on queue for treatment
 * @param buf Buffer were SDU will be stored
 * @returns Size of SDU
 */
ssize_t 
MacHighQueue::getNextSdu(char* buf){
    ssize_t retValue;

    //Lock mutex to remove SDU from the head of the queue
    lock_guard<mutex> lk(tunMutex);
    if(queue.size()==0){
        if(verbose) cout<<"[MacHighQueue] Tried to get empty SDU from L3."<<endl;
        return -1;
    }

    //Get front values from que vectors
    retValue = sizes.front();
    char* buf2 = queue.front();
    for(int i=0;i<sizes.front();i++)
        buf[i] = buf2[i];           //Copying
    
    delete [] buf2;

    //Delete front positions
    queue.erase(queue.begin());
    sizes.erase(sizes.begin());

    if(verbose) cout<<"[MacHighQueue] Got SDU from L3 queue."<<endl;

    return retValue;
}
