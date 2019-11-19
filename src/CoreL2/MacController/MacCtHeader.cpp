/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacCtHeader.cpp
@Classification : MAC Control Encoder
@
@Last alteration : November 19th, 2019
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

MacCtHeader::MacCtHeader(
    bool _flagBS,       //BS flag
    char* buffer,       //Receiving PDU buffer
    int size,           //Size of PDU in Bytes
    bool _verbose)      //Verbosity flag
{
    flagBS = _flagBS;
    verbose = _verbose;
    id = buffer[0];
    if(!_flagBS){        //Just UE decoding
        uplinkMCS = buffer[1];
        numberRBs = buffer[2];
        rbStart = buffer[3];
        MIMOon = (buffer[4]&1);
        MIMOdiversity = (buffer[4]>>1)&1;
        MIMOantenna = (buffer[4]>>2)&1;
        MIMOopenLoopClosedLoop = (buffer[4]>>3)&1;
    }
}

ssize_t 
MacCtHeader::insertControlHeader(
    char* buffer,       //Encoding PDU buffer
    int size)           //Size of PDU in Bytes
{
    //Calculates the difference in header length base on BS flag
    int delta = (flagBS?CONTROLBYTES2UE:CONTROLBYTES2BS);

    //Dynamically allocates buffer
    char *buffer2 = new char[size+delta];

    //Inserts header in new buffer
    buffer2[0] = id;
    if(flagBS){     //Just BS encoding
        buffer2[1] = uplinkMCS;
        buffer2[2] = numberRBs;
        buffer2[3] = rbStart;
        buffer2[4] = (MIMOon&(MIMOdiversity<<1)&(MIMOantenna<<2)&(MIMOopenLoopClosedLoop<<3));
    }
    if(verbose) cout<<"[MacCtHeader] Control Header inserted successfully!"<<endl;

    //Copies the old buffer to the new buffer with header inserted
    memcpy(buffer2+delta, buffer, size);
    memcpy(buffer, buffer2, size+delta);
    delete buffer2;
    return size+delta;
}

ssize_t 
MacCtHeader::removeControlHeader(
    char* buffer,       //Decoding PDU buffer
    int size)           //Size of PDU in bytes
{
    //Calculates the difference in header length base on BS flag
    int delta = (flagBS?CONTROLBYTES2BS:CONTROLBYTES2UE);

    //////////PROVISIONAL: IGNORES THE HEADER////////////////////////////////
    //Copies the buffer shifted by delta bytes
    memcpy(buffer, buffer+delta, size-delta);
    if(verbose) cout<<"[MacCtHeader] Control Header removed successfully!"<<endl;
    return size - delta;
}
