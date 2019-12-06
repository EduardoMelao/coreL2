/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : MacConfigRequest.cpp
@Classification : MAC Configutarion Request
@
@Last alteration : December 6th, 2019
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
    string readingBuffer;               //String to receive number of UEs from file
    vector<uint8_t> bufferToDecode;     //Buffer that will be used for decodification of information
    
    //Starting some variables
    verbose = _verbose;
    flagModified = false;

    configurationFile = fstream("ULReservation.txt");

    //Get information persisted earlier
    getline(configurationFile, readingBuffer);

    //Transfer it to vector
    for(int i=0;i<readingBuffer.size();i++)
        bufferToDecode.push_back(readingBuffer[i]);
    
    //Call decoding procedure
    decodeULReservation(bufferToDecode);

    //Verify if there was information persisted. If not, adds static information
    if(numberUEs){
        if(verbose) cout<<"[MacConfigRequest] Parsing successfull."<<endl;
        flagModified = 1;
    }
    else{
    ////////////PROVISIONAL: STATIC PARAMETERS///////////
        if(verbose) cout<<"[MacConfigRequest] Setting static information."<<endl;
        setPhyParameters(2, 125, 33, 66);
    }
}

MacConfigRequest::~MacConfigRequest(){
    if(configurationFile.is_open())
        configurationFile.close();
}

void
MacConfigRequest::setPhyParameters(
    uint8_t ueId,       //UE IDentification
    uint8_t tpc,        //Transmission Power Control
    uint8_t rbStart,    //Number of starting Resource Block
    uint8_t rbEnd)      //Number of ending Resource Block
{
    //Define allocation reservation struct
    allocation_cfg_t allocationConfig;
    allocationConfig.target_ue_id = (0x00|ueId);    //Complete with zeros
    allocationConfig.first_rb = rbStart;
    allocationConfig.number_of_rb = rbEnd-rbStart;

    //Add new values to vectors
    tpcs.push_back(tpc);
    uplinkReservations.push_back(allocationConfig);

    //Increment number of UEs
    numberUEs++;

    //Serialize allocation configuration struct
    vector<uint8_t> allocationBytes;
    allocationConfig.serialize(allocationBytes);

    //Persist new values
    configurationFile.open("ULReservation.txt",fstream::out|fstream::app);
    streampos auxiliary = configurationFile.tellp();    //Old output position
    configurationFile.seekp(0);     //Go to beggining
    configurationFile<<numberUEs;   //Write number of UEs in the beggining
    configurationFile.seekg(auxiliary); //Go back to end
    configurationFile<<tpc;
    for(int i=0;i<allocationBytes.size();i++)
        configurationFile<<allocationBytes[i];

    configurationFile.close();

    //Update flag
    flagModified = true;
}

void
MacConfigRequest::decodeULReservation(
    vector<uint8_t> buffer)     //Buffer containing bytes to decode
{
    int sizeOfInformation;
    vector<uint8_t> allocationDeserialization;

    //Calculate size of packets
    numberUEs = buffer[0];
    
    if(numberUEs){
        if(verbose) cout<<"[MacConfigRequest] Decoding UL Reservation..."<<endl;
        sizeOfInformation = (buffer.size()-1)/numberUEs;
    }
    else{
        if(verbose)
            cout<<"[MacConfigRequest] There was no information to decode."<<endl;
        return;
    }

    tpcs.resize(numberUEs);
    uplinkReservations.resize(numberUEs);
    
    for(int i=0;i<numberUEs;i++){    //For each UE
        tpcs[i] = buffer[i*sizeOfInformation];
        for(int j=1;j<sizeOfInformation;j++){   //Fill allocation deserialization vector
            allocationDeserialization.push_back(buffer[i*sizeOfInformation+j]);
        }
        uplinkReservations[i].deserialize(allocationDeserialization);
        allocationDeserialization.clear();
    }
    if(verbose) cout<<"[MacConfigRequest] Parsing successfull!"<<endl;    
}
