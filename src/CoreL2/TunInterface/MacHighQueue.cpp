#include "MacHighQueue.h"

MacHighQueue::MacHighQueue(TunInterface* tun, bool v){
    tunIf = tun;
    verbose = v;
}

MacHighQueue::~MacHighQueue(){ }

void MacHighQueue::reading(){
    char *buf;
    ssize_t nread = 0;
    while(1){
        buf = new char[MAXLINE];
        bzero(buf, MAXLINE);
        nread = tunIf->readTunInterface(buf, MAXLINE);
        {
            lock_guard<mutex> lk(tunMutex);
            
            if(nread==0)    //EOF
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
            
            //Everything is ok, can add buffer to queue
            queue.push_back(buf);
            sizes.push_back(nread);
            if(verbose) cout<<"[MacHighQueue] SDU added to Queue. Num SDUs: "<<queue.size()<<endl;
        }
    }
}

int MacHighQueue::getNum(){
    lock_guard<mutex> lk(tunMutex);
    return (int)queue.size();
}

ssize_t MacHighQueue::getNextSdu(char* buf){
    ssize_t retValue;
    lock_guard<mutex> lk(tunMutex);
    if(queue.size()==0){
        if(verbose) cout<<"[MacHighQueue] Tried to get empty SDU from L3."<<endl;
        return -1;
    }
    retValue = sizes.front();
    char* buf2 = queue.front();
    for(int i=0;i<sizes.front();i++)
        buf[i] = buf2[i];           //Copying
    
    delete [] buf2;

    queue.erase(queue.begin());
    sizes.erase(sizes.begin());

    if(verbose) cout<<"[MacHighQueue] Got SDU from L3 queue."<<endl;

    return retValue;
}
