/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : LinkAdaptation.cpp
@Classification : Link Adaptation
@
@Last alteration : February 20th, 2020
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : Module to perform Link Adaptation calculations.
*/

#include "LinkAdaptation.h"

LinkAdaptation::LinkAdaptation() {}

LinkAdaptation::~LinkAdaptation() {}

uint8_t
LinkAdaptation::getSnrConvertToMcs(
    float snr)  //Signal to Noise Ratio informed by PHY
{   
    uint8_t mcs;    //Modulation and Coding Scheme
    
    //#TODO: do some calculations/Look tables here
    mcs = 10;
    return mcs;
}