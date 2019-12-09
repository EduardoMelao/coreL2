/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : L1L2Interface.cpp
@Classification : L1 L2 Interface
@
@Last alteration : December 9th, 2019
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
    bool _verbose)              //Verbosity flag
{
    verbose = _verbose;
    //Static information:
    ueID = 0xCAFE;
    numerologyID = 2;
    codeRate = 0.75;
    int numberBytes = 1024;            /////////////PROVISIONAL///////////////////

    //MIMO Configuration
    mimoConfiguration.scheme = NONE;
    mimoConfiguration.num_tx_antenas = 1;
    mimoConfiguration.precoding_mtx = 0;

    //MCS Configuration
    mcsConfiguration.num_info_bytes = numberBytes;
    mcsConfiguration.modulation = QAM64;

    //Resource allocation configuration
    allocationConfiguration.first_rb = 0;
    allocationConfiguration.number_of_rb = get_num_required_rb(numerologyID, mimoConfiguration, mcsConfiguration.modulation, codeRate, numberBytes*8);
    allocationConfiguration.target_ue_id = ueID;

    //MAC PDU object definition
    macPDU.allocation_ = allocationConfiguration;
    macPDU.mimo_ = mimoConfiguration;
    macPDU.mcs_ = mcsConfiguration;

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
    socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if(socketDescriptor==-1) perror("[L1L2Interface] Socket to send information creation failed.");
    else if(verbose) cout<<"[L1L2Interface] Client socket to send info created successfully."<<endl;
    bzero(serverReceiverOfMessage, sizeof(*serverReceiverOfMessage));

    serverReceiverOfMessage->sin_family = AF_INET;
    serverReceiverOfMessage->sin_port = htons(port);
    serverReceiverOfMessage->sin_addr.s_addr = inet_addr(serverIp);  //Localhost
    return socketDescriptor;
}

int
L1L2Interface::createServerSocketToReceiveMessages(
    short port)         //Socket Port
{
    struct sockaddr_in sockname;        //Struct to configure which address server will bind to
    int socketDescriptor;

    socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
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
    return socketDescriptor;
}

void
L1L2Interface::sendPdu(
	uint8_t* buffer,        //Buffer with the PDU
	size_t size,            //PDU size in Bytes
	uint8_t* controlBuffer, //Buffer with control information
	size_t controlSize,     //Control information size in bytes
	uint8_t macAddress)     //Destination MAC Address
{
    size_t numberSent;      //Number of Bytes sent to L1

    //Perform CRC calculation
    crcPackageCalculate((char*)buffer, size);
    size+=2;    //Add CRC Bytes count
    
	//Fill MAC Data with information
	macData.resize(size);
	for(int i=0;i<size;i++)
		macData[i]=buffer[i];

	//Fill Mac Control with control information
	macControl.resize(controlSize);
	for(int i=0;i<controlSize;i++)
		macControl[i]=controlBuffer[i];

	/////////////////PROVISIONAL: IGNORE ALL THIS INFORMATION///////////////////////////

    numberSent = sendto(socketPduToL1,buffer, size, MSG_CONFIRM, (const struct sockaddr*)(&serverPdusSocketAddress), sizeof(serverPdusSocketAddress));

    //Verify if transmission was successful
	if(numberSent!=-1){
		if(verbose) cout<<"[L1L2Interface] Pdu sent:"<<size<<" bytes."<<endl;
		return;
	}
	if(verbose) cout<<"[L1L2Interface] Could not send Pdu."<<endl;
}

ssize_t
L1L2Interface::receivePdu(
    const char* buffer,         //Buffer where PDU is going to be store
    size_t maximumSize,         //Maximum PDU size
    uint8_t macAddress)              //Port to identify socket to listen to
{
    ssize_t returnValue;    //Value that will be returned at the end of this procedure
    returnValue = recv(socketPduFromL1, (void*)buffer, maximumSize, MSG_WAITALL);
    if(returnValue>0){
        if(!crcPackageChecking((char*)buffer, returnValue))
            return -2;
    }
    return returnValue==0? 0:returnValue-2;     //Value returned considers size without CRC
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
    return recv(socketControlMessagesFromL1, buffer, maximumLength, MSG_WAITALL);
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
    char i, bit;
    for(i=0x01;i;i<<=1){
        bit = (((crc&0x0001)?1:0)^((data&i)?1:0));
        crc>>=1;
        if(bit) crc^=0x9299;
    }
    return crc;
}
