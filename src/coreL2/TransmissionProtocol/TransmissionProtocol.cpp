/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : TransmissionProtocol.cpp
@Classification : Transmission Protocol
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

@Description : This module acts as broker between L2 Core and upper and upper layers.
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

bool TransmissionProtocol::sendPacketToL1(
    char* dataBuffer,       //Information Buffer
    size_t dataSize,        //Size of information Buffer in Bytes
    char* controlBuffer,    //Control information Buffer
    size_t controlSize,     //Size of control information in Bytes
    int port)               // Socket port to identify which socket to send information
{
    return l1l2Interface->sendPdu((uint8_t*)dataBuffer, dataSize,(uint8_t*)controlBuffer, controlSize, port);
}

bool TransmissionProtocol::sendPacketToL3(
    char* buffer,   //Buffer where packet will be stored
    size_t size)       //Size of information in Bytes
{
    return tunInterface->writeTunInterface(buffer, size);
}