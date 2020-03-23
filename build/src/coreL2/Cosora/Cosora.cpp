/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : Cosora.cpp
@Classification : Cosora
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

@Description : Module to perform Collaborative Spectrum Sensing Optimized for Rural Areas calculations.
*/

#include "Cosora.h"

Cosora::Cosora() {}

Cosora::~Cosora() {}

uint8_t 
Cosora::calculateSpectrumSensingValue(
    uint8_t spectrumSensingMeasurement)     //Measurements from PHY #TODO: To Be Defined 
{
    //#TODO: Implement Spectrum Sensing Value calculation.
    return spectrumSensingMeasurement;
}

uint16_t
Cosora::spectrumSensingConvertToRBIdle(
    uint8_t spectrumSensingReport)  //Spectrum Sensing Report calculated on UE
{
    //#TODO: Implement Conversion of SS to RBIdle
    return 130;
}
