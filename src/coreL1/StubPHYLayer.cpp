/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : StubPHYLayer.cpp
@Classification : Core L1 [STUB]
@
@Last alteration : January 22nd, 2019
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

#include "StubPHYLayer.h"

CoreL1::CoreL1(
    bool _verbose)  //Verbosity flag
{
    verbose = _verbose;
    numberSockets = 0;

    //PDUs Client socket creation
    socketToL2 = createClientSocketToSendMessages(PORT_TO_L2, &serverPdusSocketAddress, "127.0.0.1");

    //PDUs Server socket creation
    socketFromL2 = createServerSocketToReceiveMessages(PORT_FROM_L2);
    
    //Control Client socket creation
    socketControlMessagesToL2 = createClientSocketToSendMessages(CONTROL_MESSAGES_PORT_TO_L2, &serverControlMessagesSocketAddress, "127.0.0.1");

    //ControlServer socket creation
    socketControlMessagesFromL2 = createServerSocketToReceiveMessages(CONTROL_MESSAGES_PORT_FROM_L2);
}

CoreL1::~CoreL1()
{
    for(int i=0;i<numberSockets;i++){
        close(socketsIn[i]);
        close(socketsOut[i]);
    }
    close(socketFromL2);
    close(socketToL2);
    if(numberSockets){
        delete[] socketsIn;
        delete[] socketsOut;
        delete[] ipServers;
        delete[] ports;
        delete[] macAddresses;
        delete[] socketNames;
    }
}

int
CoreL1::createClientSocketToSendMessages(
    short port,                                     //Socket Port
    struct sockaddr_in* serverReceiverOfMessage,    //Struct to store server address to which client will send messages
    const char* serverIp)                           //Ip address of server
{
    int socketDescriptor;

    //Client socket creation
    socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if(socketDescriptor==-1) perror("[CoreL1] Socket to send information creation failed.");
    else if(verbose) cout<<"[CoreL1] Client socket to send info created successfully."<<endl;
    bzero(serverReceiverOfMessage, sizeof(*serverReceiverOfMessage));

    serverReceiverOfMessage->sin_family = AF_INET;
    serverReceiverOfMessage->sin_port = htons(port);
    serverReceiverOfMessage->sin_addr.s_addr = inet_addr(serverIp);  //Localhost
    return socketDescriptor;
}

int
CoreL1::createServerSocketToReceiveMessages(
    short port)         //Socket Port
{
    struct sockaddr_in sockname;        //Struct to configure which address server will bind to
    int socketDescriptor;

    socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if(socketDescriptor==-1) perror("[CoreL1] Socket to receive information creation failed.");

    bzero(&sockname, sizeof(sockname));

    sockname.sin_family = AF_INET;
    sockname.sin_port = htons(port);
    sockname.sin_addr.s_addr = htonl(INADDR_ANY);

    //Serve bind to socket to listen to local messages in port PORT_FROM_L1
    int bindSuccess = bind(socketDescriptor, (const sockaddr*)(&sockname), sizeof(sockname));
    if(bindSuccess==-1)
        perror("[CoreL1] Bind error.\n");
    else
        if(verbose) cout<<"[CoreL1] Bind successfully to listen to messages."<<endl;
    return socketDescriptor;
}

void 
CoreL1::addSocket(
    const char *ip,     //Destinaton socket IP
    uint16_t port,      //Destination socket port
    uint8_t macAddress) //Destination MAC Address
{
    //Verify if socket is added already
    if((getSocketIndex((uint16_t)port)!=-1)){
        if(verbose) cout<<"[CoreL1] Socket "<<port<<" already exists."<<endl;
        return;
    }

    //Create new arrays to proceed with substitution of actual arrays
    int *socketsIn2 = new int[numberSockets+1];
    int *socketsOut2 = new int[numberSockets+1];
    const char **ipServers2 = new const char*[numberSockets+1];
    uint16_t *ports2 = new uint16_t[numberSockets+1];
    uint8_t *macAddresses2 = new uint8_t[numberSockets+1];
    struct sockaddr_in *socketNames2 = new struct sockaddr_in[numberSockets+1];

    //Copy old values
    for(int i=0;i<numberSockets;i++){
        socketsIn2[i] = socketsIn[i];
        socketsOut2[i] = socketsOut[i];
        ipServers2[i] = ipServers[i];
        ports2[i] = ports[i];
        macAddresses2[i] = macAddresses[i];
        socketNames2[i] = socketNames[i];
    }

    //If they exist, delete old arrays
    if(numberSockets){
        delete[] socketsIn;
        delete[] socketsOut;
        delete[] ipServers;
        delete[] ports;
        delete[] macAddresses;
        delete socketNames;
    }

    //Assign new values
    socketsIn = socketsIn2;
    socketsOut = socketsOut2;
    ipServers = ipServers2;
    ports = ports2;
    macAddresses = macAddresses2;
    socketNames = socketNames2;

    //Add new values
    ports[numberSockets] = port;
    ipServers[numberSockets] = ip;
    macAddresses[numberSockets] = macAddress;

    //Client allocation
    socketsOut[numberSockets] = createClientSocketToSendMessages(port, &(socketNames[numberSockets]), ip);
    
    //Server allocation
    socketsIn[numberSockets] = createServerSocketToReceiveMessages(port);

    numberSockets++;
}

bool 
CoreL1::sendPdu(
    const char* buffer,     //Information buffer
    size_t size,            //Size of information in bytes
    uint16_t port)          //Port of destination socket
{
    int socketOut;          //Descriptor of the socket to which data will be sent
    ssize_t numberSent;     //Number of bytes sent

    //Gets socket index
    socketOut = getSocketIndex((uint16_t)port);

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
    int socketIn = getSocketIndex((uint16_t)port);

    //Verify if socket exists
    if(socketIn==-1){
        if(verbose) cout<<"[CoreL1] Socket not found."<<endl;
        return -1;
    }

    //Returns socket receiving function
    return recv(socketsIn[getSocketIndex((uint16_t)port)], (void*) buffer, maxSiz, MSG_WAITALL);
}

int 
CoreL1::getSocketIndex(
    uint16_t port)  //Socket port
{
    for(int i=0;i<numberSockets;i++)
        if(ports[i] == port)
            return i;
    return -1;
}

int 
CoreL1::getSocketIndex(
    uint8_t macAddress) //Socket destination MAC address
{
    for(int i=0;i<numberSockets;i++)
        if(macAddresses[i] == macAddress)
            return i;
    return -1;
}

void
CoreL1::encoding(){
    char buffer[MAXIMUMSIZE];   //Buffer to store packet from L2
    ssize_t size;               //Size of packet received
    uint8_t macAddress;         //Destination MAC address
    
    //Clear buffer
    bzero(buffer, MAXIMUMSIZE);

    //Receive from L2
    size = recv(socketFromL2, buffer, MAXIMUMSIZE, MSG_WAITALL);

    //Deserialize MAC PDU received
	vector<uint8_t> serializedMacPdu;
	serializedMacPdu.resize(size);
	serializedMacPdu.assign(buffer, buffer+size);
	MacPDU macPdu(serializedMacPdu);

    //#TODO: Remove this part of code because PHY will not send MAC PDUs via sockets
	macAddress = (((uint8_t)macPdu.mac_data_[0])&15);

    //Send PDU through correct port  
    sendPdu((const char*) &(macPdu.mac_data_[0]), macPdu.mac_data_.size(), ports[getSocketIndex(macAddress)]);
}

void 
CoreL1::decoding(
    uint8_t macAddress)
{ 
    char buffer[MAXIMUMSIZE];       //Buffer to store packet incoming
    ssize_t size;                   //Size of packet received
    bool flagBS = (macAddress!=0);  //Flag to indicate if it is BaseStation (true) or UserEquipment (false)   

    //Create SubframeRx.Start message
    string messageParameters;		            //This string will contain the parameters of the message
	vector<uint8_t> messageParametersBytes;	    //Vector to receive serialized parameters structure

    if(flagBS){     //Create BSSubframeRx.Start message
    	BSSubframeRx_Start messageBS;	//Message parameters structure
    	messageBS.sinr = 10;    
    	messageBS.serialize(messageParametersBytes);
    	for(uint i=0;i<messageParametersBytes.size();i++)
    		messageParameters+=messageParametersBytes[i];
    }
    else{       //Create UESubframeRx.Start message
        UESubframeRx_Start messageUE;	//Messages parameters structure
        messageUE.sinr = 11;
        messageUE.pmi = 1;
        messageUE.ri = 2;
        for(int i=0;i<17;i++)
            messageUE.ssm[i]=0;
        messageUE.serialize(messageParametersBytes);
        for(uint i=0;i<messageParametersBytes.size();i++)
            messageParameters+=messageParametersBytes[i];
    }

    //Downlink routine:
    string subFrameStartMessage = flagBS? "BS":"UE";    //SubframeRx.Start control message
    subFrameStartMessage+="SubframeRx.Start";
    string subFrameEndMessage = flagBS? "BS":"UE";      //SubframeRx.End control message
    subFrameEndMessage += "SubframeRx.End";
    
    //Add parameters
    subFrameStartMessage+=messageParameters;

    //Clear buffer
    bzero(buffer, MAXIMUMSIZE);

    size = receivePdu(buffer, MAXIMUMSIZE, ports[getSocketIndex(macAddress)]);

    //Communication Stream
    while(size>0){
        if(verbose) cout<<"[CoreL1] PDU with size "<<(int)size<<" received."<<endl;

        //Send control messages and PDU to L2
        sendto(socketControlMessagesToL2, &(subFrameStartMessage[0]), subFrameStartMessage.size(), MSG_CONFIRM, (const struct sockaddr*)(&serverControlMessagesSocketAddress), sizeof(serverControlMessagesSocketAddress));
        sendto(socketToL2, buffer, size, MSG_CONFIRM, (const struct sockaddr*)(&serverPdusSocketAddress), sizeof(serverPdusSocketAddress));
        sendto(socketControlMessagesToL2, &(subFrameEndMessage[0]), subFrameEndMessage.size(), MSG_CONFIRM, (const struct sockaddr*)(&serverControlMessagesSocketAddress), sizeof(serverControlMessagesSocketAddress));

        //Receive next PDU
        bzero(buffer, MAXIMUMSIZE);
        size = receivePdu(buffer, MAXIMUMSIZE, ports[getSocketIndex(macAddress)]);
    }
}

void
CoreL1::sendInterlayerMessage(
    char* buffer,           //Buffer containing message
    size_t numberBytes)     //Size of message in Bytes
{
    if(sendto(socketControlMessagesToL2, buffer, numberBytes, MSG_CONFIRM, (const struct sockaddr*)(&serverControlMessagesSocketAddress), sizeof(serverControlMessagesSocketAddress))==-1){
        if(verbose) cout<<"[CoreL1] Error sending control message."<<endl;
    }
}

void
CoreL1::receiveInterlayerMessage(){
    char buffer[MAXIMUMSIZE];       //Buffer where message will be stored
    string message;                 //String containing message converted from char*
    ssize_t messageSize = recv(socketControlMessagesFromL2, buffer, MAXIMUMSIZE, MSG_WAITALL);

    //Control message stream
    while(messageSize>0){
        //#TODO: Implement other messages decoding because it is CONSIDERING ONLY SubframeTx.Start messages

        //Manually convert char* to string
    	int subFrameStartSize = 18;
        for(int i=0;i<subFrameStartSize;i++)
            message+=buffer[i];

    	vector<uint8_t> messageParametersBytes; //Array of Bytes where serialized message parameters will be stored


        if(message=="BSSubframeTx.Start"){
            //Treat BSSubframeTx.Start parameters and trigger encoding MAC PDU to UE procedure
        	BSSubframeTx_Start messageParametersBS;
        	for(int i=subFrameStartSize;i<messageSize;i++)
        		messageParametersBytes.push_back(message[i]);
        	messageParametersBS.deserialize(messageParametersBytes);
        	if(verbose) cout<<"[CoreL1] Received BSSubframeTx.Start message. Receiving PDU from L2..."<<endl;
			encoding();
        }
        if(message=="UESubframeTx.Start"){
            //Treat UESubframeTx.Start parameters and trigger encoding MAC PDU to BS procedure
			UESubframeTx_Start messageParametersUE;
			for(int i=subFrameStartSize;i<messageSize;i++)
				messageParametersBytes.push_back(message[i]);
			messageParametersUE.deserialize(messageParametersBytes);
			if(verbose) cout<<"[CoreL1] Received UESubframeTx.Start message. Receiving PDU from L2..."<<endl;
			encoding();
		}

        if(message=="BSSubframeTx.End"||message=="UESubframeTx.End"){
            if(verbose) cout<<"[CoreL1] Received SubframeTx.End message from "<<message[0]<<message[1]<<"."<<endl;
        }

        //Clear buffer and message and receive next control message
        bzero(buffer, MAXIMUMSIZE);
        message.clear();
        messageSize = recv(socketControlMessagesFromL2, buffer, MAXIMUMSIZE, MSG_WAITALL);
    }
}

void
CoreL1::startThreads(){
    int numberThreads = 1+numberSockets;    //Number of threads
    /** Thread list:
     *  0 .. numberSockets-1    --> decoding
     *  numberSockets           --> receiving Interlayer messages
     */
    thread threads[numberThreads];

    for(int i=0;i<numberSockets;i++)
        threads[i] = thread(&CoreL1::decoding, this, macAddresses[i]);
    threads[numberThreads-1] = thread(&CoreL1::receiveInterlayerMessage, this);

    //Join all threads
    for(int i=0;i<numberThreads;i++)
        threads[i].join();
}

