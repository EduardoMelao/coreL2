/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "CoreL1.h"
/**
 * @brief Initializes a new instance of CoreL1 with 0 sockets
 * @param v Verbosity flag
 */
CoreL1::CoreL1(bool v){
    verbose = v;
    numSockets = 0;
}

/**
 * @brief Initializes a new instance of CoreL1 with 1 socket which informations were passed as parameters and no verbose
 * @param ip Destination IP address 
 * @param port Socket port
 */
CoreL1::CoreL1(const char *ip, uint16_t port){
    CoreL1(ip, port, false);
}

/**
 * @brief Initializes a new instance of CoreL1 with 1 socket which informations were passed as parameters
 * @param ip Destination IP address 
 * @param port Socket port
 * @param v Verbosity flag
 */
CoreL1::CoreL1(const char *ip, uint16_t port, bool v){
    verbose = v;
    numSockets = 0;
    addSocket(ip, port);
}

/**
 * @brief Destructor of CoreL1 object
 */
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

/**
 * @brief Adds new socket information in CoreL1. 
 * Declarates a client socket to send information to destination and a server socket to receive information from destination.
 * @param ip Destination IP address
 * @param port Socket port
 */
void 
CoreL1::addSocket(const char *ip, uint16_t port){
    //Verify if socket is added already
    if((getSockIn(port)!=-1)||(getSockOut(port)!=-1)){
        if(verbose) cout<<"[CoreL1] Socket "<<port<<" already exists."<<endl;
        return;
    }

    //Create new arrays to proceed with substitution of actual arrays
    int *socksIn2 = new int[numSockets+1];
    int *socksOut2 = new int[numSockets+1];
    const char **ipServers2 = new const char*[numSockets+1];
    uint16_t *ports2 = new uint16_t[numSockets+1];
    struct sockaddr_in *socknames2 = new struct sockaddr_in[numSockets+1];

    //Copy old values
    for(int i=0;i<numSockets;i++){
        socksIn2[i] = socksIn[i];
        socksOut2[i] = socksOut[i];
        ipServers2[i] = ipServers[i];
        ports2[i] = ports[i];
        socknames2[i] = socknames[i];
    }

    //Delete old arrays
    delete[] socksIn;   socksIn = socksIn2;
    delete[] socksOut;  socksOut = socksOut2;
    delete[] ipServers; ipServers = ipServers2;
    delete[] ports;     ports = ports2;
    delete socknames;   socknames = socknames2;

    //Add new values
    ports[numSockets] = port;
    ipServers[numSockets] = ip;

    //Client allocation
    socksOut[numSockets] = socket(AF_INET, SOCK_DGRAM, 0);
    if(socksOut[numSockets]==-1) perror("[CoreL1] Client socket creation failed.");
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

    //Server bind to socket
    int b = bind(socksIn[numSockets], (const sockaddr*)(&sockname), sizeof(sockname));
    if(b==-1){
        perror("[CoreL1] Bind Error.\n");
        return;
    }
    if(verbose) cout<<"[CoreL1] Bind successfull."<<endl;

    numSockets++;
}

/**
 * @brief Send PDU to socket considering there's just one socket added
 * @param buf Information buffer
 * @param n Size of information in bytes
 * @returns If transmission was successfull
 */
bool 
CoreL1::sendPdu(const char* buf, size_t n){
    return sendPdu(buf, n, ports[0]);
}


/**
 * @brief Send PDU to socket identified by port
 * @param buf Information buffer
 * @param n Size of information in bytes
 * @param port Socket port to identify which socket to send information
 * @returns True if transmission was successfull; False if it was not
 */
bool 
CoreL1::sendPdu(const char* buf, size_t n, uint16_t port){
    int sockOut, r;

    //Gets socket index
    sockOut=getSockOut(port);

    //Verify if socket exists
    if(sockOut!=-1){

        //Send information in buf to socket
        r = sendto(socksOut[sockOut], buf, n, MSG_CONFIRM, (const struct sockaddr*)(&(socknames[sockOut])), sizeof(socknames[sockOut]));

        //Verify if transmission was successful
        if(r!=-1){
            if(verbose) cout<<"[CoreL1] Pdu sent:"<<n<<" bytes."<<endl;
            return true;
        }
    }
    if(verbose) cout<<"[CoreL1] Could not send Pdu."<<endl;
    return false;
}

/**
 * @brief Receive PDU from socket considering there's just one socket added
 * @param buf Information buffer to put the information received
 * @param maxSiz Maximum size of buffer
 * @returns Length in bytes of information received
 */
ssize_t 
CoreL1::receivePdu(const char* buf, size_t maxSiz){
    return receivePdu(buf, maxSiz, ports[0]);
}

/**
 * @brief Receive PDU from socket identified by port
 * @param buf Information buffer to put the information received
 * @param maxSiz Maximum size of buffer
 * @param port Socket port to identify which socket to receive information
 * @returns Length in bytes of information received
 */
ssize_t 
CoreL1::receivePdu(const char* buf, size_t maxSiz, uint16_t port){
    //Gets socket index
    int sockIn = getSockIn(port);

    //Verify if socket exists
    if(sockIn==-1){
        if(verbose) cout<<"[CoreL1] Socket not found."<<endl;
        return -1;
    }

    //Returns socket receiving function
    return recv(socksIn[getSockIn(port)], (void*) buf, maxSiz, MSG_WAITALL);
}

/**
 * @brief Get index of socket to receive information
 * @param port Socket port
 * @returns Socket index or -1 if socket was not found
 */
int 
CoreL1::getSockIn(uint16_t port){
    for(int i=0;i<numSockets;i++)
        if(ports[i] == port)
            return i;
    return -1;
}

/**
 * @brief Get index of socket to send information
 * @param port Socket port
 * @returns Socket index or -1 if socket was not found
 */
int 
CoreL1::getSockOut(uint16_t port){
    for(int i=0;i<numSockets;i++)
        if(ports[i] == port)
            return i;
    return -1;
}

/**
 * @brief Get ports currently added to CoreL1 object
 * @returns Array of ports
 */
uint16_t* 
CoreL1::getPorts(){
    return ports;
}
