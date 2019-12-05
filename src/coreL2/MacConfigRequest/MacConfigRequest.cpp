/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacConfigRequest.cpp
@Classification : MAC Configutarion Request
@
@Last alteration : December 5th, 2019
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module interacts with CLI commands and persists dynamic
parameters to be used in next transmission by MacController.
*/

#include "MacConfigRequest.h"

MacConfigRequest::MacConfigRequest(
    bool _verbose)      //Verbosity flag
{
    string numberUEsString;     //String to receive number of UEs from file
    string tpcString;           //String to receive TPC from file
    string allocationString;    //String to receive allocation from file
    vector<uint8_t> allocationVector;   //Vector to be used to deserialize allocation configuration

    verbose = _verbose;
    flagModified = false;

    if(verbose) cout<<"[MacController] Parsing file to detect previous configuration"<<endl;
    configurationFile = fstream("ULReservation.txt");
    getline(configurationFile, numberUEsString);
    numberUEs = stoi(numberUEsString);

    if(verbose&&numberUEs) cout<<"[MacCofigRequest] Parsing "<<numberUEs<<" UE configurations from file..."<<endl;

    tpcs.resize(numberUEs);
    uplinkReservations.resize(numberUEs);

    for(int i=0;i<numberUEs;i++){
        getline(configurationFile, tpcString);
        tpcs.push_back(stoi(tpcString));
        getline(configurationFile, allocationString);
        allocationVector = vector<uint8_t>(allocationString.begin(), allocationString.end());
        uplinkReservations[i].deserialize(allocationVector);
    }

    if(verbose&&numberUEs){
        cout<<"[MacConfigRequest] Parsing successfull."<<endl;
        flagModified = 1;
    }

    ////////////PROVISIONAL: STATIS PARAMETERS///////////
    if(!numberUEs){
        if(verbose) cout<<"[MacConfigRequest] Setting static information."<<endl;
        setPhyParameters(2, 125, 33, 66);
    }
}

MacConfigRequest::~MacConfigRequest(){
    configurationFile.close();
}

void
MacConfigRequest::setPhyParameters(
    uint8_t ueId,
    uint8_t tpc,
    uint8_t RBStart,
    uint8_t RBEnd)
{
    //Define allocation reservation struct
    allocation_cfg_t allocationConfig;
    allocationConfig.target_ue_id = (0x00|ueId);    //Complete with zeros
    allocationConfig.first_rb = RBStart;
    allocationConfig.number_of_rb = RBEnd-RBStart;

    //Add new values to vectors
    tpcs.push_back(tpc);
    uplinkReservations.push_back(allocationConfig);

    //Increment number of UEs
    numberUEs++;

    //Serialize allocation configuration struct
    vector<uint8_t> allocationBytes;
    allocationConfig.serialize(allocationBytes);

    //Persist new values
    streampos auxiliary = configurationFile.tellp();    //Old output position
    configurationFile.seekp(0);     //Go to beggining
    configurationFile<<numberUEs;   //Write number of UEs in the beggining
    configurationFile.seekg(auxiliary); //Go back to end
    configurationFile<<tpc<<endl;
    for(auto i=allocationBytes.begin();i!=allocationBytes.end;i++)
        configurationFile<<*i;
    configurationFile<<endl;

    //Update flag
    flagModified = true;

}