/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : LinkAdaptation.cpp
@Classification : Link Adaptation
@
@Last alteration : January 10th, 2019
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

#include "AdaptiveModulationCoding.h"

AdaptiveModulationCoding::AdaptiveModulationCoding() {}

AdaptiveModulationCoding::~AdaptiveModulationCoding() {}

uint8_t
AdaptiveModulationCoding::getCqiConvertToMcs(
    uint8_t cqi)      //Channel Quality Information
{
    uint8_t mcs;  //Modulation and Coding Scheme
    //PROVISIONAL: do some calculations here with cqi and returns
    mcs = 9;
    return mcs;
}
