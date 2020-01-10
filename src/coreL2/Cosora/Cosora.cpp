/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : Cosora.cpp
@Classification : Cosora
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

@Description : Module to perform Collaborative Spectrum Sensing Optimized for Rural Areas calculations.
*/

#include "Cosora.h"

Cosora::Cosora() {}

Cosora::~Cosora() {}

void 
Cosora::calculateSpectrumSensingValue(
    uint8_t* spectrumSensingMeasurement,        //Measurements from PHY
    uint8_t* spectrumSensingReport)             //Report of RBs idleness 
{
    //PROVISIONAL: simple copy of the values.
    for(int i=0;i<17;i++)
        spectrumSensingReport[i] = spectrumSensingMeasurement[i];
}

uint16_t
Cosora::spectrumSensingConvertToRBIdle(
    uint8_t* spectrumSensingReport)     //Spectrum Sensing Report calculated on UE
{
    //PROVISIONAL: do some calculations here
    return 130;
}
