/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : ReceptionProtocol.cpp
@Classification : Reception Protocol
@
@Last alteration : April 23rd, 2020
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module acts as broker between L2 Core and upper and lower layers.
*/

#include "ReceptionProtocol.h"

ReceptionProtocol::ReceptionProtocol(
    L1L2Interface* _l1l2Interface,  //Object to receive packets from L1
    TunInterface* _tunInterface,    //Object to receive packets from L3
    bool _verbose)                  //Verbosity flag
{
    l1l2Interface = _l1l2Interface;
    tunInterface = _tunInterface;
    verbose = _verbose;
}

ReceptionProtocol::~ReceptionProtocol() {}

void 
ReceptionProtocol::receivePackageFromL1(
    vector<MacPDU*> & buffer,   //Buffer where packet will be stored
    int maxSize)
{
    if(verbose) cout<<"[ReceptionProtocol] Receiving packet from L1."<<endl;
    l1l2Interface->receivePdus(buffer, maxSize);
}

ssize_t 
ReceptionProtocol::receivePackageFromL3(
    char* buffer,       //Buffer where packet will be stored
    int maximumSize)    //Maximum size of buffer in Bytes
{
    return tunInterface->readTunInterface(buffer, maximumSize);
}
