#include "TunInterface.h"
using namespace std;    

TunInterface::TunInterface(){
    TunInterface("tun0", false);
}

TunInterface::TunInterface(bool v){
    TunInterface("tun0", v);
}

TunInterface::TunInterface(const char* devName){
    TunInterface(devName,false);
}

TunInterface::TunInterface(const char* devName, bool v){
    verbose = v;
    dev = new char[IFNAMSIZ+1];
    memset(dev,0,IFNAMSIZ+1);
    if(devName!=NULL) strncpy(dev,devName,sizeof(dev)-1);
}

TunInterface::~TunInterface(){
    delete[] dev;
    close(fd);
}

bool TunInterface::allocTunInterface(){
    if(dev==NULL){
        if(verbose) cout << "[TunInterface] Error creating interface: dev = NULL." << endl;
        return false;
    }

    fd = open("/dev/net/tun", O_RDWR);

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    ioctl(fd, TUNSETIFF, (void *) &ifr);
    strncpy(dev,ifr.ifr_name, IFNAMSIZ);
    char cmd[100];
    sprintf(cmd, "ifconfig %s up", ifr.ifr_ifrn.ifrn_name);
    system(cmd);
    if(verbose) cout << "[TunInterface] Tun interface allocated successfully." << endl;
    return true;
}

ssize_t TunInterface::readTunInterface(char* buf, size_t n_bytes){
    return read(fd, buf, n_bytes);
}

bool TunInterface::writeTunInterface(char* buf, size_t n){
    ssize_t returnValue = write(fd, buf, n);
    if(returnValue==-1){
        if(verbose) cout<<"[TunInterface] Could not write to Tun Interface."<<endl;
        return false;
    }
    return true;
}
