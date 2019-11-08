#include "CoreL1.h"

CoreL1::CoreL1(bool v){
    verbose = v;
    numSockets = 0;
}

CoreL1::CoreL1(const char *ip, uint16_t port){
    CoreL1(ip, port, false);
}

CoreL1::CoreL1(const char *ip, uint16_t port, bool v){
    verbose = v;
    numSockets = 0;
    addSocket(ip, port);
}

CoreL1::~CoreL1(){
    for(int i=0;i<numSockets;i++){
        close(socksIn[i]);
        close(socksOut[i]);
    }
    delete[] socksIn;
    delete[] socksOut;
    delete[] ipServers;
    delete[] ports;
    delete[] socknames;
}


void CoreL1::addSocket(const char *ip, uint16_t port){
    if((getSockIn(port)!=-1)||(getSockOut(port)!=-1)){
        if(verbose) cout<<"[CoreL1] Socket "<<port<<" already exists."<<endl;
        return;
    }

    //Proceed with substitution of actual arrays
    int *socksIn2 = new int[numSockets+1];
    int *socksOut2 = new int[numSockets+1];
    const char **ipServers2 = new const char*[numSockets+1];
    uint16_t *ports2 = new uint16_t[numSockets+1];
    struct sockaddr_in *socknames2 = new struct sockaddr_in[numSockets+1];

    for(int i=0;i<numSockets;i++){
        socksIn2[i] = socksIn[i];
        socksOut2[i] = socksOut[i];
        ipServers2[i] = ipServers[i];
        ports2[i] = ports[i];
        socknames2[i] = socknames[i];
    }

    delete[] socksIn;   socksIn = socksIn2;
    delete[] socksOut;  socksOut = socksOut2;
    delete[] ipServers; ipServers = ipServers2;
    delete[] ports;     ports = ports2;
    delete socknames;   socknames = socknames2;

    ports[numSockets] = port;
    ipServers[numSockets] = ip;

    //Client allocation
    socksOut[numSockets] = socket(AF_INET, SOCK_DGRAM, 0);
    if(socksOut[numSockets]==-1) perror("[CoreL1] Client socket creationg failed.");
    else if(verbose) cout<<"[CoreL1] Client created successfully."<<endl;
    bzero(&(socknames[numSockets]), sizeof(socknames[numSockets]));

    socknames[numSockets].sin_family = AF_INET;
    socknames[numSockets].sin_port = htons(port);
    socknames[numSockets].sin_addr.s_addr = inet_addr(ipServers[numSockets]);

    //Server allocation
    struct sockaddr_in sockname;
    socksIn[numSockets] = socket(AF_INET, SOCK_DGRAM, 0);
    if(socksOut[numSockets]==-1) perror("[CoreL1] Server socket creationg failed.");
    else if(verbose) cout<<"[CoreL1] Server created successfully."<<endl;

    bzero(&sockname, sizeof(sockname));

    sockname.sin_family = AF_INET;
    sockname.sin_port = htons(port);
    sockname.sin_addr.s_addr = htonl(INADDR_ANY);

    int b = bind(socksIn[numSockets], (const sockaddr*)(&sockname), sizeof(sockname));
    if(b==-1){
        perror("[CoreL1] Bind Error.\n");
        return;
    }
    if(verbose) cout<<"[CoreL1] Bind successfull."<<endl;

    numSockets++;
}

//Send PDU to socket
bool CoreL1::sendPdu(const char* buf, size_t n){
    return sendPdu(buf, n, ports[0]);
}


//Send PDU to socket
bool CoreL1::sendPdu(const char* buf, size_t n, uint16_t port){
    int sockOut, r;
    sockOut=getSockOut(port);
    if(sockOut!=-1){
        r = sendto(socksOut[sockOut], buf, n, MSG_CONFIRM, (const struct sockaddr*)(&(socknames[sockOut])), sizeof(socknames[sockOut]));
        if(r!=-1){
            if(verbose) cout<<"[CoreL1] Pdu sent:"<<n<<" bytes."<<endl;
            return true;;
        }
    }
    if(verbose) cout<<"[CoreL1] Could not send Pdu."<<endl;
    return false;
}

//Receive PDU from socket
ssize_t CoreL1::receivePdu(const char* buf, size_t maxSiz){
    return receivePdu(buf, maxSiz, ports[0]);
}

//Receive PDU from socket
ssize_t CoreL1::receivePdu(const char* buf, size_t maxSiz, uint16_t port){
    int sockIn = getSockIn(port);
    if(sockIn==-1){
        if(verbose) cout<<"[CoreL1] Socket not found."<<endl;
        return -1;
    }
    return recv(socksIn[getSockIn(port)], (void*) buf, maxSiz, MSG_WAITALL);
}

//Get socket for receiving messages. Returns -1 if does not find it.
int CoreL1::getSockIn(uint16_t port){
    for(int i=0;i<numSockets;i++)
        if(ports[i] == port)
            return i;
    return -1;
}

//Get socket for sending messages. Returns -1 if does not find it.
int CoreL1::getSockOut(uint16_t port){
    for(int i=0;i<numSockets;i++)
        if(ports[i] == port)
            return i;
    return -1;
}

uint16_t* CoreL1::getPorts(){
    return ports;
}
