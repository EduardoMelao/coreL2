/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : L1L2Interface.cpp
@Classification : L1 L2 Interface
@
@Last alteration : June 28th, 2020
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0
Project : H2020 5G-Range
Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados
@Description : This module controls de communication between MAC and PHY,
    using Message Queues between the two Layers to exchange data Bytes and 
    control Bytes. CRC Calculation and checking is made here too.
*/

#include "L1L2Interface.h"

using namespace std;
using namespace lib5grange;

L1L2Interface::L1L2Interface(
    bool _verbose)      //Verbosity flag
{
    verbose = _verbose;

    //Message queue creation 
    l1l2InterfaceQueues.createMessageQueues();
}

L1L2Interface::~L1L2Interface() {
    l1l2InterfaceQueues.closeMessageQueues();
}

void
L1L2Interface::sendPdus(
	vector<MacPDU> macPdus)           //MAC PDUs structure
{
    ssize_t numberSent;                 //Number of Bytes sent to L1
    size_t numberPduBytes;              //Number of Bytes contained into PDU 
    vector<uint8_t> serializedMacPdus;  //Array of Bytes containing all MAC PDUs serialized

    //Perform CRC calculations
    for(int i=0;i<macPdus.size();i++){
        numberPduBytes = macPdus[i].mac_data_.size();       //Number of Data Bytes before inserting CRC
        macPdus[i].mac_data_.resize(numberPduBytes+2);      //Resize vector
        crcPackageCalculate((char*)&(macPdus[i].mac_data_[0]), numberPduBytes);
        macPdus[i].mcs_.num_info_bytes += 2;                //Insert 2 Bytes for CRC
        macPdus[i].serialize(serializedMacPdus);            //Serialize MAC PDU
    }

    //Send PDU to L1
    numberSent = mq_send(l1l2InterfaceQueues.mqPduToPhy, (const char*)&(serializedMacPdus[0]), serializedMacPdus.size(), 1);

    //Verify if transmission was successful
	if(numberSent!=-1){
		if(verbose) cout<<"[L1L2Interface] Pdu sent:"<<serializedMacPdus.size()<<" bytes."<<endl;
		return;
	}
	if(verbose) cout<<"[L1L2Interface] Could not send Pdu."<<endl;
}

void
L1L2Interface::receivePdus(
    vector<MacPDU*> & buffer)   //Buffer where PDUs are going to be stored
{
    char *receptionBuffer = new char[MQ_MAX_MSG_SIZE];  //Buffer to receive PDUs from L1
    bzero(receptionBuffer, MQ_MAX_MSG_SIZE);            //Clear Reception buffer


    //Perform socket UDP packet reception
    ssize_t totalSize = mq_receive(l1l2InterfaceQueues.mqPduFromPhy, receptionBuffer, MQ_MAX_MSG_SIZE, NULL);

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
    while(mq_send(l1l2InterfaceQueues.mqControlToPhy, buffer, numberBytes, 1)==-1){
        if(errno == EAGAIN)
            continue;
        if(verbose) perror("[L1L2Interface] Error sending control message.");
        exit(1);
    }
}

ssize_t
L1L2Interface::receiveControlMessage(
    char* buffer)               //Buffer where message will be stored
{
    return mq_receive(l1l2InterfaceQueues.mqControlFromPhy, buffer, MQ_MAX_MSG_SIZE, NULL);
}

void 
L1L2Interface::crcPackageCalculate(
    char* buffer,       //Buffer of Bytes of PDU
    int size)           //PDU size in Bytes
{
    unsigned short crc = auxiliaryCalculationCRC(buffer, size);

    //Input CRC value at the end of Buffer
    buffer[size] = crc>>8;
    buffer[size+1] = crc&255;
}

bool 
L1L2Interface::crcPackageChecking(
    char* buffer,       //Bytes of PDU
    int size)           //Size of PDU in Bytes
{
    unsigned short receivedCRC;     //Value of CRC received at the end of PDU
    unsigned short calculatedCRC;   //Value of CRC calculated with PDU bytes

    //Get received CRC from 2 ending Bytes of PDU received
    receivedCRC = ((buffer[size-2]&255)<<8)|((buffer[size-1])&255);

    //Calculate CRC of received PDU discounting 2 ending Bytes used above
    calculatedCRC = auxiliaryCalculationCRC(buffer, size-2);    

    //Return true of values match
    return receivedCRC == calculatedCRC;
}

unsigned short 
L1L2Interface::auxiliaryCalculationCRC(
    char* buffer,       //Buffer of Bytes of PDU
    int size)           //PDU size in Bytes
{
    boost::crc_16_type result;              //Result of CRC calculation
    result.process_bytes(buffer, size);     //Process buffer Bytes

    return result.checksum(); //Get CRC value
}
