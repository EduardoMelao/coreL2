/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacConfigRequest.cpp
@Classification : MAC Configuration Request
@
@Last alteration : January 14th, 2019
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : 	This module uses dynamic information to configure BS and UE
*/

#include "MacConfigRequest.h"

MacConfigRequest::MacConfigRequest(
		bool _verbose)		//Verbosity flag
{
    verbose = _verbose;
    dynamicParameters = new DynamicParameters(verbose);
}


MacConfigRequest::~MacConfigRequest(){
    delete dynamicParameters;
}
