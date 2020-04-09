/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : L1L2Interface.cpp
@Classification : L1 L2 Interface
@
@Last alteration : April 1st, 2020
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0
Project : H2020 5G-Range
Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados
@Description : This module controls de communication between MAC and PHY,
    using UPD sockets in the two Layers to exchange data Bytes and control Bytes. 
    CRC Calculation and checking is made here too.
*/

#include "L1L2Interface.h"

using namespace std;
using namespace lib5grange;

L1L2Interface::L1L2Interface(
    bool _verbose)      //Verbosity flag
{
    verbose = _verbose;

    //Client PDUs socket creation
    socketPduToL1 = createClientSocketToSendMessages(PORT_TO_L1, &serverPdusSocketAddress, "127.0.0.1");

    //ServerPDUs socket creation
    socketPduFromL1 = createServerSocketToReceiveMessages(PORT_FROM_L1);
    
    //Client Control Messages socket creation
    socketControlMessagesToL1 = createClientSocketToSendMessages(CONTROL_MESSAGES_PORT_TO_L1, &serverControlMessagesSocketAddress, "127.0.0.1");

    //Server Control Messages socket creation
    socketControlMessagesFromL1 = createServerSocketToReceiveMessages(CONTROL_MESSAGES_PORT_FROM_L1);
    
}

L1L2Interface::~L1L2Interface() {
    close(socketPduFromL1);
    close(socketPduToL1);
    close(socketControlMessagesToL1);
    close(socketControlMessagesFromL1);
}

int
L1L2Interface::createClientSocketToSendMessages(
    short port,                                     //Socket Port
    struct sockaddr_in* serverReceiverOfMessage,    //Struct to store server address to which client will send messages
    const char* serverIp)                           //Ip address of server
{
    int socketDescriptor;

    //Client socket creation
    socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if(socketDescriptor==-1) perror("[L1L2Interface] Socket to send information creation failed.");
    else if(verbose) cout<<"[L1L2Interface] Client socket to send info created successfully."<<endl;
    bzero(serverReceiverOfMessage, sizeof(*serverReceiverOfMessage));

    serverReceiverOfMessage->sin_family = AF_INET;
    serverReceiverOfMessage->sin_port = htons(port);
    serverReceiverOfMessage->sin_addr.s_addr = inet_addr(serverIp);  //Localhost

    connect(socketDescriptor, (const sockaddr*)(serverReceiverOfMessage), sizeof(*serverReceiverOfMessage));
    return socketDescriptor;
}

int
L1L2Interface::createServerSocketToReceiveMessages(
    short port)         //Socket Port
{
    struct sockaddr_in sockname;        //Struct to configure which address server will bind to
    int socketDescriptor;

    socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if(socketDescriptor==-1) perror("[L1L2Interface] Socket to receive information creation failed.");

    bzero(&sockname, sizeof(sockname));

    sockname.sin_family = AF_INET;
    sockname.sin_port = htons(port);
    sockname.sin_addr.s_addr = htonl(INADDR_ANY);

    //Serve bind to socket to listen to local messages in port PORT_FROM_L1
    int bindSuccess = bind(socketDescriptor, (const sockaddr*)(&sockname), sizeof(sockname));
    if(bindSuccess==-1)
        perror("[L1L2Interface] Bind error.\n");
    else
        if(verbose) cout<<"[L1L2Interface] Bind successfully to listen to messages."<<endl;

    listen(socketDescriptor, 5);

    socklen_t length = sizeof(sockname);

    accept(socketDescriptor, (sockaddr*)(&sockname), &length);

    return socketDescriptor;
}

void
L1L2Interface::sendPdus(
	MacPDU** macPdus,           //MAC PDUs structure
    int numberPdus)             //Number of MAC PDUs for transmission
{
    ssize_t numberSent;                 //Number of Bytes sent to L1
    size_t numberPduBytes;              //Number of Bytes contained into PDU 
    vector<uint8_t> serializedMacPdus;  //Array of Bytes containing all MAC PDUs serialized

    //Perform CRC calculations
    for(int i=0;i<numberPdus;i++){
        numberPduBytes = macPdus[i]->mac_data_.size();      //Number of Data Bytes before inserting CRC
        macPdus[i]->mac_data_.resize(numberPduBytes+2);     //Resize vector
        crcPackageCalculate((char*)&(macPdus[i]->mac_data_[0]), numberPduBytes);
        macPdus[i]->serialize(serializedMacPdus);           //Serialize MAC PDU
    }

    //Send PDU to L1
    numberSent = sendto(socketPduToL1,&(serializedMacPdus[0]), serializedMacPdus.size(), MSG_CONFIRM, (const struct sockaddr*)(&serverPdusSocketAddress), sizeof(serverPdusSocketAddress));

    //Verify if transmission was successful
	if(numberSent!=-1){
		if(verbose) cout<<"[L1L2Interface] Pdu sent:"<<serializedMacPdus.size()<<" bytes."<<endl;
		return;
	}
	if(verbose) cout<<"[L1L2Interface] Could not send Pdu."<<endl;
}

void
L1L2Interface::receivePdus(
    vector<MacPDU*> & buffer,   //Buffer where PDUs are going to be stored
    size_t maximumSize)         //Maximum PDU size
{
    char *receptionBuffer = new char[maximumSize];  //Buffer to receive PDUs from L1
    bzero(receptionBuffer, maximumSize);            //Clear Reception buffer


    //Perform socket UDP packet reception
    ssize_t totalSize = recv(socketPduFromL1, receptionBuffer, maximumSize, MSG_WAITALL);

    //Turn reception buffer into vector of Bytes for MAC PDU deserialization
    vector<uint8_t> receptionBufferBytes;
    receptionBufferBytes.resize(totalSize);
    receptionBufferBytes.assign(receptionBuffer, receptionBuffer+totalSize);

    //Test if Received information is valid
    if(totalSize<1){
        if(verbose) cout<<"[L1L2Interface] Invalid information received from PHY."<<endl;
        exit(1);
    }
    
    //While offset does not reach the end of receptionBuffer
    while(receptionBufferBytes.size()){
        //Resize MAC PDUs vector and reserialize next PDU
        buffer.resize(buffer.size()+1);
        buffer[buffer.size()-1] = new MacPDU(receptionBufferBytes);

        //Drop PDU if CRC does not check
        if(!crcPackageChecking((char*)&(buffer[buffer.size()-1]->mac_data_[0]), buffer[buffer.size()-1]->mac_data_.size())){
            if(verbose) cout<<"Drop Package due to CRC error"<<endl;
            buffer.erase(buffer.end());
        }
    }

    delete [] receptionBuffer;
}

void
L1L2Interface::sendControlMessage(
    char* buffer,           //Buffer containing the message
    size_t numberBytes)     //Message size in Bytes
{
    if(sendto(socketControlMessagesToL1, buffer, numberBytes, MSG_CONFIRM, (const struct sockaddr*)(&serverControlMessagesSocketAddress), sizeof(serverControlMessagesSocketAddress))==-1){
        if(verbose) cout<<"[L1L2Interface] Error sending control message."<<endl;
    }
}

ssize_t
L1L2Interface::receiveControlMessage(
    char* buffer,               //Buffer where message will be stored
    size_t maximumLength)       //Maximum message length in Bytes
{
    return recv(socketControlMessagesFromL1, buffer, maximumLength, MSG_DONTWAIT);
}

void 
L1L2Interface::crcPackageCalculate(
    char* buffer,       //Buffer of Bytes of PDU
    int size)           //PDU size in Bytes
{
    unsigned short crc = 0x0000;
    for(int i=0;i<size;i++){
        crc = auxiliaryCalculationCRC(buffer[i],crc);
    }
    buffer[size] = crc>>8;
    buffer[size+1] = crc&255;
}

bool 
L1L2Interface::crcPackageChecking(
    char* buffer,       //Bytes of PDU
    int size)           //Size of PDU in Bytes
{
    //Perform CRC calculation with auxiliary function
    unsigned short crc1, crc2;
    crc1 = ((buffer[size-2]&255)<<8)|((buffer[size-1])&255);
    crc2 = 0x0000;
    for(int i=0;i<size-2;i++){
        crc2 = auxiliaryCalculationCRC(buffer[i],crc2);
    }

    return crc1==crc2;
}

unsigned short 
L1L2Interface::auxiliaryCalculationCRC(
    char data,              //Byte from PDU
    unsigned short crc)     //CRC history
{
    //Perform CRC calculation per Byte
    char i, bit;
    for(i=0x01;i;i<<=1){
        bit = (((crc&0x0001)?1:0)^((data&i)?1:0));
        crc>>=1;
        if(bit) crc^=0x9299;
    }
    return crc;
}
