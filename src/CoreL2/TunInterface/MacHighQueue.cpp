/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "MacHighQueue.h"

MacHighQueue::MacHighQueue(
    TunInterface* tun,      //Tun Interface object
    bool _verbose)          //Verbosity flag
{
    tunInterface = tun;
    verbose = _verbose;
}

MacHighQueue::~MacHighQueue(){ }

void 
MacHighQueue::reading(){
    char *buffer;
    ssize_t nread = 0;
    while(1){
        //Allocate buffer
        buffer = new char[MAXLINE];
        bzero(buffer, MAXLINE);

        //Read from TUN Interface
        nread = tunInterface->readTunInterface(buffer, MAXLINE);
        {
            //Lock to write in the queue
            lock_guard<mutex> lk(tunMutex);
            
            //Check EOF
            if(nread==0)
                break;

            //Check ipv4
            if(((buffer[0]>>4)&15) != 4){
                if(verbose) cout<<"[MacHighQueue] Dropped non-ipv4 packet."<<endl;
                continue;
            }

            //Check broadcast
            if((buffer[DST_OFFSET]==255)&&(buffer[DST_OFFSET+1]==255)&&(buffer[DST_OFFSET+2]==255)&&(buffer[DST_OFFSET+3]==255)){
                if(verbose) cout<<"[MacHighQueue] Dropped broadcast packet."<<endl;
                continue;
            }

            //Check multicast
            if((buffer[DST_OFFSET]>=224)&&(buffer[DST_OFFSET]<=239)){
                if(verbose) cout<<"[MacHighQueue] Dropped multicast packet."<<endl;
                continue;
            }
            
            //Everything is ok, buffer can be added to queue
            queue.push_back(buffer);
            sizes.push_back(nread);
            if(verbose) cout<<"[MacHighQueue] SDU added to Queue. Num SDUs: "<<queue.size()<<endl;
        }
    }
}

int 
MacHighQueue::getNum(){
    //Lock mutex to consult queue size information
    lock_guard<mutex> lk(tunMutex);
    return (int)queue.size();
}

ssize_t 
MacHighQueue::getNextSdu(
    char* buffer)       //Buffer to store the SDU
{          
    ssize_t retValue;   //Return value

    //Lock mutex to remove SDU from the head of the queue
    lock_guard<mutex> lk(tunMutex);
    if(queue.size()==0){
        if(verbose) cout<<"[MacHighQueue] Tried to get empty SDU from L3."<<endl;
        return -1;
    }

    //Get front values from que vectors
    retValue = sizes.front();
    char* buffer2 = queue.front();
    for(int i=0;i<sizes.front();i++)
        buffer[i] = buffer2[i];           //Copying
    
    delete [] buffer2;

    //Delete front positions
    queue.erase(queue.begin());
    sizes.erase(sizes.begin());

    if(verbose) cout<<"[MacHighQueue] Got SDU from L3 queue."<<endl;

    return retValue;
}
