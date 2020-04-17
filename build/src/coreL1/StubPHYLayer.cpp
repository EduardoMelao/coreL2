/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : StubPHYLayer.cpp
@Classification : Core L1 [STUB]
@
@Last alteration : April 7th, 2020
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
    uint8_t _macAddress,    //MAC Address of this equipment
    bool _verbose)          //Verbosity flag
{
    currentMacAddress = _macAddress;
    verbose = _verbose;
    numberSockets = 0;
    subFrameCounter = 0;    
    rxMetricsPeriodicity = 0;   //Unnactivated
    phyActive = false;          //Initialized as false

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
CoreL1::sendPdus(
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
CoreL1::receivePdus(
    const char* buffer,     //Information buffer
    size_t maxSiz)          //Maximum size of buffer
{         
    return receivePdus(buffer, maxSiz, ports[0]);
}

ssize_t 
CoreL1::receivePdus(
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
CoreL1::encoding(
    uint8_t numberPdus)     //Number of PDUs for transmission
{
    char bufferFromL2[MAXIMUMSIZE]; //Buffer to store packet from L2
    vector<uint8_t> bufferPdu;      //Buffer to store only data to send to the other side
    ssize_t size;                   //Size of packet received
    uint8_t macAddress;             //Destination MAC address
    size_t pdusTotalSize = 0;       //Total size of PDUs for transmission
    MacPDU* macPdu;                 //Pointer to store desserialized PDU

    //Receive from L2
    size = recv(socketFromL2, bufferFromL2, MAXIMUMSIZE, MSG_WAITALL);

    //Convert buffer received to vector<uint8_t>
    vector<uint8_t> serializedMacPdu;
    serializedMacPdu.resize(size);
    serializedMacPdu.assign(bufferFromL2, bufferFromL2+size);

    //Clear bufferPdu
    bufferPdu.clear();

    //Deserialize MAC PDUs received
    for(int i=0;i<numberPdus;i++){
        //Desserialize next PDU
        macPdu = new MacPDU(serializedMacPdu);

        //Copy data to vector that will be sent to other PHY
        for(int j=0;j<macPdu->mac_data_.size();j++)
            bufferPdu.push_back(macPdu->mac_data_[j]);

        //Print allocation
        if(verbose) cout<<"[CoreL1] Allocation: First RB: "<<(int)macPdu->allocation_.first_rb<<" | Number RB: "<<(int)macPdu->allocation_.number_of_rb<<endl;

        //Delete recently created macPdu object
        delete macPdu;
    }

    //Send PDU through all ports
    for(int i=0;i<numberSockets;i++)
        sendPdus((const char*) &(bufferPdu[0]), bufferPdu.size(), ports[i]);
}

void 
CoreL1::decoding(
    uint8_t macAddress)
{ 
    char buffer[MAXIMUMSIZE];       //Buffer to store packet incoming
    ssize_t size;                   //Size of packet received
    bool flagBS = (macAddress!=0);  //Flag to indicate if it is BaseStation (true) or UserEquipment (false)   

    //Create SubframeRx.Start message
	vector<uint8_t> subFrameStartMessage;	            //Vector to receive serialized parameters structure SubframeRx.Start
    subFrameStartMessage.push_back(flagBS? 'C':'D');    //SubframeRx.Start control message
    char subFrameEndMessage = 'E';                      

    
    if(flagBS){     //Create BSSubframeRx.Start message
        BSSubframeRx_Start messageBS;	//Message parameters structure
        for(int i=0;i<132;i++)
            messageBS.snr[i] = 10; 
        messageBS.serialize(subFrameStartMessage);
    }
    else{       //Create UESubframeRx.Start message
        UESubframeRx_Start messageUE;	//Messages parameters structure
        for(int i=0;i<132;i++)
            messageUE.snr[i] = 11;
        messageUE.ssm = 3;
        messageUE.serialize(subFrameStartMessage);
    }

    //Declare variables to be used in demultiplexing PDU(s) received
    size_t offset = 0;          //Offset location decoding buffer
    uint8_t numberSDUs;         //Number of SDUs in actual PDU
    int sizeSdus = 0;           //Total size of SDUs into PDU
    int sizePdu;                //Total size of actual PDU
    vector<MacPDU> macPDUs;     //Array of MACPDUs to be sent to L2
    vector<uint8_t> bytesPDUs;  //Array of bytes corresponding to MAC PDUs serialized

    //Clear buffer
    bzero(buffer, MAXIMUMSIZE);

    size = receivePdus(buffer, MAXIMUMSIZE, ports[getSocketIndex(macAddress)]);

    //Communication Stream
    while(size>0){
        if(verbose) cout<<"[CoreL1] PDU with size "<<(int)size<<" received."<<endl;
        
        //Demultiplex PDUs received, verifying if destination is correct
        offset = 0; //Reset offset

        while(offset<size){
            //Reset counter of total size of SDUs
            sizeSdus = 0;

            //Read PDU size
            numberSDUs = (uint8_t) buffer[offset+1];

            //Calculate total Size of SDUs into PDU
            for(int i=0;i<numberSDUs;i++){
                sizeSdus += (((buffer[offset+2+2*i]&127)<<8)|((buffer[offset+3+2*i])&255));
            }

            //Set offset to PDU total size
            sizePdu = 2+ 2*numberSDUs + sizeSdus + 2;  //2 bytes: SA, DA, numPDUs;2 bytes for each: D/C flag and size; 2 bytes at end: CRC
            offset += sizePdu;

            //Verify if destination is correct
            if(currentMacAddress!=(buffer[offset-sizePdu]&15)){
                if(verbose) cout<<"[CoreL1] Drop PDU destinated to UE "<<(int)(buffer[offset-sizePdu]&15)<<endl;
                continue;
            }

            //Drop PDU if CRC does not check
            macPDUs.resize(macPDUs.size()+1);
            macPDUs.back().mac_data_.resize(sizePdu);
            macPDUs.back().mac_data_.assign(&(buffer[offset-sizePdu]), &(buffer[offset-sizePdu])+sizePdu);
            macPDUs.back().rankIndicator_ = 10;
            macPDUs.back().snr_avg_ = 10;
        }

        //Test if all PDU(s) was(were) droped
        if(macPDUs.size()==0){
                //Receive next PDUs
            bzero(buffer, MAXIMUMSIZE);
            macPDUs.clear();
            size = receivePdus(buffer, MAXIMUMSIZE, ports[getSocketIndex(macAddress)]);
            continue;
        }

        //Create a vector of Bytes to be sent to L2
        for(int i=0;i<macPDUs.size();i++)
            macPDUs[i].serialize(bytesPDUs);

        //Send SubframeRx.Start control message to L2 and RX Metrics if it is time
        if(rxMetricsPeriodicity && subFrameCounter==rxMetricsPeriodicity){
            sendto(socketControlMessagesToL2, &(subFrameStartMessage[0]), subFrameStartMessage.size(), MSG_CONFIRM, (const struct sockaddr*)(&serverControlMessagesSocketAddress), sizeof(serverControlMessagesSocketAddress));
            subFrameCounter = 0;
        }
        else{
            sendto(socketControlMessagesToL2, &(subFrameStartMessage[0]), 1, MSG_CONFIRM, (const struct sockaddr*)(&serverControlMessagesSocketAddress), sizeof(serverControlMessagesSocketAddress));
        }

        if(rxMetricsPeriodicity) subFrameCounter ++;
        
        //Send PDUs
        sendto(socketToL2, &bytesPDUs[0], bytesPDUs.size(), MSG_CONFIRM, (const struct sockaddr*)(&serverPdusSocketAddress), sizeof(serverPdusSocketAddress));
        
        //Send SubframeRx.End message
        sendto(socketControlMessagesToL2, &subFrameEndMessage, 1, MSG_CONFIRM, (const struct sockaddr*)(&serverControlMessagesSocketAddress), sizeof(serverControlMessagesSocketAddress));

        //Receive next PDUs
        bzero(buffer, MAXIMUMSIZE);
        macPDUs.clear();
        bytesPDUs.clear();
        size = receivePdus(buffer, MAXIMUMSIZE, ports[getSocketIndex(macAddress)]);
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
    ssize_t messageSize = recv(socketControlMessagesFromL2, buffer, MAXIMUMSIZE, MSG_WAITALL);

    //Control message stream
    while(messageSize>0){
        //#TODO: Implement other messages decoding because it is CONSIDERING ONLY SubframeTx.Start messages

    	vector<uint8_t> messageParametersBytes; //Array of Bytes where serialized message parameters will be stored
        BSSubframeTx_Start messageParametersBS; //Struct for BSSubframeTx.Start parameters
        UESubframeTx_Start messageParametersUE; //Struct for UESubframeTx.Start parameters


        switch(buffer[0])
        {
            case 'A':
            {
                //Treat PHYConfig.Request message and return PHYConfig.Response
                if(verbose) cout<<"[CoreL1] Received PHYConfig.Request Message."<<endl;
                char configResponseMessage[] = "AA";
                sendInterlayerMessage(configResponseMessage, 2);
                phyActive = true;
            }
            break;
            case 'B':
            {
                //Tread PHYStop.Request message and return PHYStop.Response
                if(verbose) cout<<"[CoreL1] Received PHYStop.Request Message."<<endl;
                char stopResponseMessage[] = "BA";
                phyActive = false;
                sendInterlayerMessage(stopResponseMessage, 2);
            }
            break;
            case 'C':
                //Treat BSSubframeTx.Start parameters and trigger encoding MAC PDU to UE procedure
                for(int i=1;i<messageSize;i++)
                    messageParametersBytes.push_back(buffer[i]);
                messageParametersBS.deserialize(messageParametersBytes);
                rxMetricsPeriodicity = messageParametersBS.rxMetricPeriodicity;
                if(verbose) cout<<"[CoreL1] Received BSSubframeTx.Start message. Receiving PDU from L2..."<<endl;
                encoding(messageParametersBS.numPDUs);
                break;

            case 'D':
                //Treat UESubframeTx.Start parameters and trigger encoding MAC PDU to BS procedure
                for(int i=1;i<messageSize;i++)
                    messageParametersBytes.push_back(buffer[i]);
                messageParametersUE.deserialize(messageParametersBytes);
                rxMetricsPeriodicity = messageParametersUE.rxMetricPeriodicity;
                if(verbose) cout<<"[CoreL1] Received UESubframeTx.Start message. Receiving PDU from L2..."<<endl;
                encoding(1);
                break;

            case 'E':
                if(verbose) cout<<"[CoreL1] Received SubframeTx.End message."<<endl;
                break;

            case 'F':
                if(verbose) cout<<"[CoreL1] Receive new FLUT value: "<<(int)buffer[1]<<endl;
            
            default:
                if(verbose) cout<<"[CoreL1] Unknown Control Message received."<<endl;
                break;
        }

        //Clear buffer and message and receive next control message
        bzero(buffer, MAXIMUMSIZE);
        messageSize = recv(socketControlMessagesFromL2, buffer, MAXIMUMSIZE, MSG_WAITALL);
    }
}

void
CoreL1::startThreads(){
    int numberThreads = 2+numberSockets;    //Number of threads
    /** Thread list:
     *  0 .. numberSockets-1    --> decoding
     *  numberSockets           --> receiving Interlayer messages
     */
    thread threads[numberThreads];

    for(int i=0;i<numberSockets;i++)
        threads[i] = thread(&CoreL1::decoding, this, macAddresses[i]);
    threads[numberThreads-2] = thread(&CoreL1::receiveInterlayerMessage, this);
    threads[numberThreads-1] = thread(&CoreL1::sendTxIndication, this);

    //Join all threads
    for(int i=0;i<numberThreads;i++)
        threads[i].join();
}

void
CoreL1::sendTxIndication()
{
    char txIndication = 'F';    //Tx Indication character

    //Infinite loop
    while(1){
        if(phyActive){
            sendInterlayerMessage(&txIndication, 1);
            this_thread::sleep_for(chrono::microseconds(4600));
        }
    }
}
