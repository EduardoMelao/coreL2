/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : L1L2Interface.cpp
@Classification : L1 L2 Interface
@
@Last alteration : November 28th, 2019
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0
Project : H2020 5G-Range
Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados
@Description : This module controls de communication between MAC and PHY,
    using common data structures shared by the two Layers to exchange data.
*/

#include "L1L2Interface.h"

using namespace std;
using namespace lib5grange;

L1L2Interface::L1L2Interface(
    CoreL1* _l1)                 //CoreL1 object initialized with static parameters
{
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

    l1 = _l1;
}

L1L2Interface::~L1L2Interface() {}

bool
L1L2Interface::sendPdu(
	uint8_t* buffer,        //Buffer with the PDU
	size_t size,            //PDU size in Bytes
	uint8_t* controlBuffer, //Buffer with control information
	size_t controlSize,     //Control information size in bytes
	uint16_t port)          //Socket port to identify which socket to send information
{
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
	return l1->sendPdu((const char*) buffer, size, port);
}

ssize_t
L1L2Interface::receivePdu(
    const char* buffer,         //Buffer where PDU is going to be store
    size_t maximumSize,         //Maximum PDU size
    uint16_t port)              //Port to identify socket to listen to
{
    ssize_t returnValue;    //Value that will be returned at the end of this procedure
    returnValue = receivePdu(buffer, maximumSize, port);
    if(returnValue>0){
        if(!crcPackageChecking((char*)buffer, returnValue))
            return -2;
    }
    return returnValue==0? 0:returnValue-2;     //Value returned considers size without CRC
}

uint16_t*
L1L2Interface::getPorts()
{
    return l1->getPorts();
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
