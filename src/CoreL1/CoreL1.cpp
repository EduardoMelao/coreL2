/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "CoreL1.h"

CoreL1::CoreL1(
    bool _verbose)  //Verbosity flag
{
    verbose = _verbose;
    numSockets = 0;
}

CoreL1::CoreL1(
    const char *ip,     //Destinaton socket IP
    uint16_t port)      //Destination socket port
{
    CoreL1(ip, port, false);
}

CoreL1::CoreL1(
    const char *ip,     //Destinaton socket IP
    uint16_t port,      //Destination socket port
    bool _verbose)      //Verbosity flag
{
    verbose = _verbose;
    numSockets = 0;
    addSocket(ip, port);
}

CoreL1::~CoreL1()
{
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

void 
CoreL1::addSocket(
    const char *ip,     //Destinaton socket IP
    uint16_t port)      //Destination socket port
{
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

bool 
CoreL1::sendPdu(
    const char* buffer,     //Information buffer
    size_t size)            //Size of information in bytes
{
    return sendPdu(buffer, size, ports[0]);
}

bool 
CoreL1::sendPdu(
    const char* buffer,     //Information buffer
    size_t size,            //Size of information in bytes
    uint16_t port)          //Port of destination socket
{
    int sockOut;        //Descriptor of the socket to which data will be sent
    ssize_t numberSent; //Number of bytes sent

    //Gets socket index
    sockOut = getSockOut(port);

    //Verify if socket exists
    if(sockOut!=-1){

        //Send information in buffer to socket
        numberSent = sendto(socksOut[sockOut], buffer, size, MSG_CONFIRM, (const struct sockaddr*)(&(socknames[sockOut])), sizeof(socknames[sockOut]));

        //Verify if transmission was successful
        if(numberSent!=-1){
            if(verbose) cout<<"[CoreL1] Pdu sent:"<<size<<" bytes."<<endl;
            return true;
        }
    }
    if(verbose) cout<<"[CoreL1] Could not send Pdu."<<endl;
    return false;
}

ssize_t 
CoreL1::receivePdu(
    const char* buffer,     //Information buffer
    size_t maxSiz)          //Maximum size of buffer
{         
    return receivePdu(buffer, maxSiz, ports[0]);
}

ssize_t 
CoreL1::receivePdu(
    const char* buffer,     //Information buffer
    size_t maxSiz,          //Maximum size of buffer
    uint16_t port)          //Port of receiving socket
{
    //Gets socket index
    int sockIn = getSockIn(port);

    //Verify if socket exists
    if(sockIn==-1){
        if(verbose) cout<<"[CoreL1] Socket not found."<<endl;
        return -1;
    }

    //Returns socket receiving function
    return recv(socksIn[getSockIn(port)], (void*) buffer, maxSiz, MSG_WAITALL);
}

int 
CoreL1::getSockIn(
    uint16_t port)  //Socket port
{
    for(int i=0;i<numSockets;i++)
        if(ports[i] == port)
            return i;
    return -1;
}

int 
CoreL1::getSockOut(
    uint16_t port)  //Socket port
{
    for(int i=0;i<numSockets;i++)
        if(ports[i] == port)
            return i;
    return -1;
}

uint16_t* 
CoreL1::getPorts()
{
    return ports;
}
