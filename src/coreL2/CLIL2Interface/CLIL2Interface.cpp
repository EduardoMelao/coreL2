/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : CLIL2Interface.cpp
@Classification : CLI - L2 Interface
@
@Last alteration : January 22nd, 2019
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : 	This module has operations to perform interaction between CLI and MAC.
*/

#include "CLIL2Interface.h"

CLIL2Interface::CLIL2Interface(
		bool _verbose)		//Verbosity flag
{
    verbose = _verbose;

    //Initialize flags with FALSE
    macStartCommandFlag = false;
    macStopCommandFlag = false;
    macConfigRequestCommandFlag = false;

    //Initialize DynamicParameters object
    dynamicParameters = new DynamicParameters(verbose);
}


CLIL2Interface::~CLIL2Interface(){
    delete dynamicParameters;
}

void
CLIL2Interface::macStartCommand(){
    setMacStartCommandFlag(true);
}

void
CLIL2Interface::macStopCommand(){
    setMacStopCommandFlag(true);
}

void
CLIL2Interface::macConfigRequestCommand(){
    setMacConfigRequestCommandFlag(true);
}

void
CLIL2Interface::setMacStartCommandFlag(
    bool _macStartCommandFlag)          //New Flag balue
{
    macStartCommandFlag = _macStartCommandFlag;
}

void
CLIL2Interface::setMacStopCommandFlag(
    bool _macStopCommandFlag)           //New Flag balue
{
    macStopCommandFlag = _macStopCommandFlag;
}

void
CLIL2Interface::setMacConfigRequestCommandFlag(
    bool _macConfigRequestCommandFlag)  //New Flag balue
{
    macConfigRequestCommandFlag = _macConfigRequestCommandFlag;
}

bool
CLIL2Interface::getMacStartCommandFlag(){
   return macStartCommandFlag;
}

bool
CLIL2Interface::getMacStopCommandFlag(){
   return macStopCommandFlag;
}

bool
CLIL2Interface::getMacConfigRequestCommandFlag(){
   return macConfigRequestCommandFlag;
}

