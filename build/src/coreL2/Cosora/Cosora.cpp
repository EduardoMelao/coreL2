/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : Cosora.cpp
@Classification : Cosora
@
@Last alteration : March 27th, 2020
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

Cosora::Cosora(
    DynamicParameters* _dynamicParameters,      //DynamicParameters container from main system to store temporary modifications
    CurrentParameters* _currentParameters,      //Parameters currently being used by the System
    bool _verbose)                              //Verbosity flag
{
    //Assign pointers
    dynamicParameters = _dynamicParameters;
    currentParameters = _currentParameters;
    verbose = _verbose;

    //Verify if Fusion is activated
    isActive = currentParameters->getSSReportWaitTimeout()>0;

    //Initialize isWaiting flag , fusionLookupTable value and timeout value
    isWaiting = false;
    fusionLookupTable = 15;     //00001111: all 4 channels idle
    timeout = 4600*currentParameters->getSSReportWaitTimeout();    //timeout value in nanoseconds
    

}

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
    uint16_t rbsIdle=0;
    for(int i=0;i<4;i++)
        rbsIdle+=((spectrumSensingReport>>i)&1)*33;
    return rbsIdle;
}

void 
Cosora::spectrumSensingTimeout(){
    //Wait for timeout nanoseconds
    this_thread::sleep_for(chrono::nanoseconds(timeout));

    if(verbose) cout<<"[Cosora] Timeout! Executing Fusion Algorithm"<<endl;

    //Lock fusion mutex to change System FLUT value
    lock_guard<mutex> lk(fusionMutex);

    //Redefine isWaiting flag
    isWaiting = false;

    //Verify is FLUT value changed, change DynamicParameters value and change Mac Mode
    if(fusionLookupTable!=currentParameters->getFLUTMatrix()){
        dynamicParameters->setFLutMatrix(fusionLookupTable);

        if(verbose) cout<<"[Cosora] New fusion LUT value: "<<(int)fusionLookupTable<<endl;
        //Set MAC Mode to RECONFIG_MODE if it is not entering STOP_MODE
        if(currentParameters->getMacMode()!=STOP_MODE)
            currentParameters->setMacMode(RECONFIG_MODE);
    }

    //Finally, set fusionLookupTable back to default value
    fusionLookupTable = 15;
}

void
Cosora::fusionAlgorithm(
    uint8_t ssr)        //New spectrum sensing report
{
    //First, verify if Fusion is activated
    if(!isActive){
        return;
    }

    //Then, lock mutex to make alterations
    lock_guard<mutex> lk(fusionMutex);

    //Perform Fusion algorithm: AND
    fusionLookupTable &= ssr;

    //Detach timeout if it was not detached yet
    if(!isWaiting){
        isWaiting = true;
        thread timeout = thread(&Cosora::spectrumSensingTimeout, this);
        timeout.detach();
    }
}

bool
Cosora::isBusy(){
    lock_guard<mutex> lk(fusionMutex);
    return isWaiting;
}