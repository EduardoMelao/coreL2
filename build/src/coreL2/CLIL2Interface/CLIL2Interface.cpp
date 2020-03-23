/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : CLIL2Interface.cpp
@Classification : CLI - L2 Interface
@
@Last alteration : January 24th, 2020
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
    macStartCommandSignal = false;
    macStopCommandSignal = false;
    macConfigRequestCommandSignal = false;

    //Initialize DynamicParameters object
    dynamicParameters = new DynamicParameters(verbose);
}


CLIL2Interface::~CLIL2Interface(){
    delete dynamicParameters;
}

void
CLIL2Interface::macStartCommand(){
    setMacStartCommandSignal(true);
}

void
CLIL2Interface::macStopCommand(){
    setMacStopCommandSignal(true);
}

void
CLIL2Interface::macConfigRequestCommand(){
    setMacConfigRequestCommandSignal(true);
}

void
CLIL2Interface::setMacStartCommandSignal(
    bool _macStartCommandSignal)          //New Flag value
{
    macStartCommandSignal = _macStartCommandSignal;
}

void
CLIL2Interface::setMacStopCommandSignal(
    bool _macStopCommandFlag)           //New Flag value
{
    macStopCommandSignal = _macStopCommandFlag;
}

void
CLIL2Interface::setMacConfigRequestCommandSignal(
    bool _macConfigRequestCommandSignal)  //New Flag value
{
    macConfigRequestCommandSignal = _macConfigRequestCommandSignal;
}

bool
CLIL2Interface::getMacStartCommandSignal(){
   return macStartCommandSignal;
}

bool
CLIL2Interface::getMacStopCommandSignal(){
   return macStopCommandSignal;
}

bool
CLIL2Interface::getMacConfigRequestCommandSignal(){
   return macConfigRequestCommandSignal;
}

