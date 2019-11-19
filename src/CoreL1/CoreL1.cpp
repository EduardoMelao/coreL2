/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : CoreL1.cpp
@Classification : [STUB] PHY Layer
@
@Last alteration : November 19th, 2019
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This is a stub module that simulates the physical layer transmission and reception.
    UDP sockets are used on transmitter and receiver sides to exchange packets.
*/

#include "CoreL1.h"

CoreL1::CoreL1(
    bool _verbose)  //Verbosity flag
{
    verbose = _verbose;
    numberSockets = 0;
}

CoreL1::CoreL1(
    const char *ip,     //Destination socket IP
    uint16_t port)      //Destination socket port
{
    CoreL1(ip, port, false);
}

CoreL1::CoreL1(
    const char *ip,     //Destination socket IP
    uint16_t port,      //Destination socket port
    bool _verbose)      //Verbosity flag
{
    verbose = _verbose;
    numberSockets = 0;
    addSocket(ip, port);
}

CoreL1::~CoreL1()
{
    for(int i=0;i<numberSockets;i++){
        close(socketsIn[i]);
        close(socketsOut[i]);
    }
    delete[] socketsIn;
    delete[] socketsOut;
    delete[] ipServers;
    delete[] ports;
    delete[] socketNames;
}

void 
CoreL1::addSocket(
    const char *ip,     //Destinaton socket IP
    uint16_t port)      //Destination socket port
{
    //Verify if socket is added already
    if((getSocketIn(port)!=-1)||(getSocketOut(port)!=-1)){
        if(verbose) cout<<"[CoreL1] Socket "<<port<<" already exists."<<endl;
        return;
    }

    //Create new arrays to proceed with substitution of actual arrays
    int *socketsIn2 = new int[numberSockets+1];
    int *socketsOut2 = new int[numberSockets+1];
    const char **ipServers2 = new const char*[numberSockets+1];
    uint16_t *ports2 = new uint16_t[numberSockets+1];
    struct sockaddr_in *socketNames2 = new struct sockaddr_in[numberSockets+1];

    //Copy old values
    for(int i=0;i<numberSockets;i++){
        socketsIn2[i] = socketsIn[i];
        socketsOut2[i] = socketsOut[i];
        ipServers2[i] = ipServers[i];
        ports2[i] = ports[i];
        socketNames2[i] = socketNames[i];
    }

    //Delete old arrays
    delete[] socketsIn;   socketsIn = socketsIn2;
    delete[] socketsOut;  socketsOut = socketsOut2;
    delete[] ipServers; ipServers = ipServers2;
    delete[] ports;     ports = ports2;
    delete socketNames;   socketNames = socketNames2;

    //Add new values
    ports[numberSockets] = port;
    ipServers[numberSockets] = ip;

    //Client allocation
    socketsOut[numberSockets] = socket(AF_INET, SOCK_DGRAM, 0);
    if(socketsOut[numberSockets]==-1) perror("[CoreL1] Client socket creation failed.");
    else if(verbose) cout<<"[CoreL1] Client created successfully."<<endl;
    bzero(&(socketNames[numberSockets]), sizeof(socketNames[numberSockets]));

    socketNames[numberSockets].sin_family = AF_INET;
    socketNames[numberSockets].sin_port = htons(port);
    socketNames[numberSockets].sin_addr.s_addr = inet_addr(ipServers[numberSockets]);

    //Server allocation
    struct sockaddr_in sockname;
    socketsIn[numberSockets] = socket(AF_INET, SOCK_DGRAM, 0);
    if(socketsOut[numberSockets]==-1) perror("[CoreL1] Server socket creation failed.");
    else if(verbose) cout<<"[CoreL1] Server created successfully."<<endl;

    bzero(&sockname, sizeof(sockname));

    sockname.sin_family = AF_INET;
    sockname.sin_port = htons(port);
    sockname.sin_addr.s_addr = htonl(INADDR_ANY);

    //Server bind to socket
    int b = bind(socketsIn[numberSockets], (const sockaddr*)(&sockname), sizeof(sockname));
    if(b==-1){
        perror("[CoreL1] Bind Error.\n");
        return;
    }
    if(verbose) cout<<"[CoreL1] Bind successful."<<endl;

    numberSockets++;
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
    int socketOut;        //Descriptor of the socket to which data will be sent
    ssize_t numberSent; //Number of bytes sent

    //Gets socket index
    socketOut = getSocketOut(port);

    //Verify if socket exists
    if(socketOut!=-1){

        //Send information in buffer to socket
        numberSent = sendto(socketsOut[socketOut], buffer, size, MSG_CONFIRM, (const struct sockaddr*)(&(socketNames[socketOut])), sizeof(socketNames[socketOut]));

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
    int socketIn = getSocketIn(port);

    //Verify if socket exists
    if(socketIn==-1){
        if(verbose) cout<<"[CoreL1] Socket not found."<<endl;
        return -1;
    }

    //Returns socket receiving function
    return recv(socketsIn[getSocketIn(port)], (void*) buffer, maxSiz, MSG_WAITALL);
}

int 
CoreL1::getSocketIn(
    uint16_t port)  //Socket port
{
    for(int i=0;i<numberSockets;i++)
        if(ports[i] == port)
            return i;
    return -1;
}

int 
CoreL1::getSocketOut(
    uint16_t port)  //Socket port
{
    for(int i=0;i<numberSockets;i++)
        if(ports[i] == port)
            return i;
    return -1;
}

uint16_t* 
CoreL1::getPorts()
{
    return ports;
}
