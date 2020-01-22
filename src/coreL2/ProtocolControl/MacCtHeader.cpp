/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacCtHeader.cpp
@Classification : Protocol Control
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

@Description : This module encodes and decodes the Control Header, used to communicate with PHY. 
*/

#include "MacCtHeader.h"

MacCtHeader::MacCtHeader(
    bool _flagBS,       //BS flag
    bool _verbose)      //Verbosity flag
{
    flagBS = _flagBS;
    verbose = _verbose;
    id = 4;
    uplinkMCS = 18;
    rbStart = 1;
    numberRBs = 2;
    MIMOon = 1;
    MIMOdiversity = 0;
    MIMOopenLoopClosedLoop = 1;
    MIMOantenna = 0;
}

ssize_t 
MacCtHeader::getControlData(
    char* buffer)	//Buffer to store control data
{
    //Calculates the difference in header length base on BS flag
    int size = (flagBS?CONTROLBYTES2UE:CONTROLBYTES2BS);

    //Inserts header in new buffer
    buffer[0] = id;
    if(flagBS){     //Just BS encoding
        buffer[1] = uplinkMCS;
        buffer[2] = numberRBs;
        buffer[3] = rbStart;
        buffer[4] = (MIMOon&(MIMOdiversity<<1)&(MIMOantenna<<2)&(MIMOopenLoopClosedLoop<<3));
    }
    if(verbose) cout<<"[MacCtHeader] Got Control Information to send to PHY!"<<endl;

    return size;
}
