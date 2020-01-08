/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : TransmissionProtocol.cpp
@Classification : Transmission Protocol
@
@Last alteration : December 13th, 2019
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module acts as broker between L2 Core and lower/upper layers.
*/

#include "TransmissionProtocol.h"

TransmissionProtocol::TransmissionProtocol(
    L1L2Interface* _l1l2Interface,  //Object to send packets to L1
    TunInterface* _tunInterface,    //Object to send packets to L3
    bool _verbose)                  //Verbosity flag
{
    l1l2Interface = _l1l2Interface;
    tunInterface = _tunInterface;
    verbose = _verbose;
}

TransmissionProtocol::~TransmissionProtocol() {}

void 
TransmissionProtocol::sendPackageToL1(
    MacPDU macPdu,          //MAC PDU structure
    uint8_t macAddress)     //Destination MAC Address
{
    if(verbose) cout<<"[TransmissionProtocol] Sending packet to L1."<<endl;
    l1l2Interface->sendPdu(macPdu, macAddress);
}

void 
TransmissionProtocol::sendControlMessageToL1(
    char* controlBuffer,    //Control information Buffer
    size_t controlSize)     //Size of control information in Bytes
{
    if(verbose) cout<<"[TransmissionProtocol] Sending control message to L1."<<endl;
    return l1l2Interface->sendControlMessage(controlBuffer, controlSize);
}

bool 
TransmissionProtocol::sendPackageToL3(
    char* buffer,   //Buffer where packet will be stored
    size_t size)    //Size of information in Bytes
{
    if(verbose) cout<<"[TransmissionProtocol] Sending packet to L3."<<endl;
    return tunInterface->writeTunInterface(buffer, size);
}
