/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : TimerSubframe.cpp
@Classification : Timer Subframe
@
@Last alteration : March 30th, 2020
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module acts with a timer and supports main module to count Subframes elapsed.
*/

#include "TimerSubframe.h"

TimerSubframe::TimerSubframe() { 
    killThread = false;
}

TimerSubframe::~TimerSubframe() {
    killThread = true;
}

void
TimerSubframe::countingThread(){
    while(!killThread){
        //Sleep for Subframe duration
        std::this_thread::sleep_for(std::chrono::microseconds(SUBFRAME_DURATION));

        //Increase counter value
        subframeCounter++;
    }
}

unsigned long long
TimerSubframe::getSubframeNumber(){
    return subframeCounter;
}

void
TimerSubframe::stopCounting(){
    //Just set thread to be killed
    killThread = true;
}