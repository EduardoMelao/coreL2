/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : Scheduler.cpp
@Classification : Scheduler
@
@Last alteration : March 18th, 2020
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module is responsible for Scheduling SDUs into MAC PDUs
            to be transmitted in the next subframe.
*/

#include "Scheduler.h"

Scheduler::Scheduler(
    CurrentParameters* _currentParameters,  //System Current execution parameters
    SduBuffers* _sduBuffers,                //Buffers responsible to store Control and Data SDUs
    bool _verbose)                          //Verbosity flag
{
    //Assign values do class variables
    currentParameters = _currentParameters;
    sduBuffers = _sduBuffers;
    verbose = _verbose;
}

Scheduler::~Scheduler() {}

vector<int>
Scheduler::selectUEs()
{
    vector<int> indexUEs;   //Array of UEIDs

    //Verify all UEs for data for transmission
    for(int i=0;i<currentParameters->getNumberUEs();i++){
        if(sduBuffers->bufferStatusInformation(currentParameters->getMacAddress(i)))
            indexUEs.push_back(currentParameters->getMacAddress(i));
    }

    return indexUEs;
}

void
Scheduler::scheduleRequestBS(
    MacPDU** macPDUs)   //Array of MAC PDUs where scheduler will store Multiplexed SDUs
{
    //Define variables
    vector<int> ueIds;                      //Array of UEIDs
    uint8_t fusionLutValue;                 //Fusion Lookup Table value: array of 4 least significat bits
    char sduBuffer[MAXIMUM_BUFFER_LENGTH];  //Buffer to store SDU on aggregation procedure
    size_t sduSize;                         //SDU Size in bytes

    //Check buffer status information & select UEs for scheduling
    ueIds = selectUEs();
    if(verbose) cout<<"[Scheduler] Selected "<<ueIds.size()<<" UEs for next transmission."<<endl;

    //If there's just one UE for transmission, duplicate entry to simulate 2 UEs (Scheduler works with 2 UEs for transmission)
    if(ueIds.size()==1)
        ueIds.push_back(ueIds[0]);
    
    //Calculate allocation based on Fusion Lookup Table and fill MacPDU objects
    calculateDownlinkSpectrumAllocation(macPDUs, currentParameters->getFLUTMatrix());
    
    //For each PDU:
    for(int i=0;i<ueIds.size();i++){
    	//Assign MAC PDU UEID
    	macPDUs[i]->allocation_.target_ue_id = ueIds[i];

        //Fill MAC - PHY control struct
        macPDUs[i]->macphy_ctl_.first_tb_in_subframe = i==0;
        macPDUs[i]->macphy_ctl_.last_tb_in_subframe = i==(ueIds.size()-1);
        macPDUs[i]->macphy_ctl_.sequence_number = i;

        //Fill MIMO struct
        macPDUs[i]->mimo_.scheme = (currentParameters->getMimoConf(ueIds[i]))? ((currentParameters->getMimoDiversityMultiplexing(ueIds[i]))? DIVERSITY:MULTIPLEXING):NONE;
        macPDUs[i]->mimo_.num_tx_antenas = currentParameters->getMimoAntenna(ueIds[i]);
        macPDUs[i]->mimo_.precoding_mtx = currentParameters->getMimoPrecoding(ueIds[i]);

        //Fill MCS struct
        macPDUs[i]->mcs_.modulation = mcsToModulation[currentParameters->getMcsDownlink(ueIds[i])];

        //Fill Numerology
        macPDUs[i]->numID_ = currentParameters->getNumerology();

        //Calculate number of bits for next transmission
        size_t numberBits = get_bit_capacity(macPDUs[i]->numID_, macPDUs[i]->allocation_, macPDUs[i]->mimo_, macPDUs[i]->mcs_.modulation);
        if(verbose) cout<<"[Scheduler] Scheduled "<<numberBits/8<<" Bytes for PDU "<<i<<endl;

        //Create a new Multiplexer object to aggregate SDUs
        Multiplexer* multiplexer = new Multiplexer(numberBits/8, 0, ueIds[i], verbose);

        //Aggregation procedure - Control SDUs
        int numberControlSDUs = sduBuffers->getNumberControlSdus(ueIds[i]);
        if(numberControlSDUs==-1){
            if(verbose) cout<<"[Scheduler] Invalid MAC address scheduling control SDUs."<<endl;
            exit(1);
        }
        
        for(int j=0;j<numberControlSDUs;j++){
            //Verify if it is possivel to enqueue next SDU
            if((multiplexer->getNumberofBytes() + 2 + sduBuffers->getNextControlSduSize(ueIds[0]))>numberBits/8){
                if(verbose) cout<<"[Scheduler] End of scheduling control SDUs: extrapolated bit capacity."<<endl;
                break;
            }

            //Clear buffer
            bzero(sduBuffer, MAXIMUM_BUFFER_LENGTH);

            //Get SDU from queue
            sduSize = sduBuffers->getNextControlSdu(ueIds[i], sduBuffer);

            //Add SDU to multiplexer
            multiplexer->addSDU(sduBuffer, sduSize, 0);
        }

        //Aggregation procedure - Data SDUs
        int numberDataSDUs = sduBuffers->getNumberDataSdus(ueIds[i]);
        if(numberDataSDUs==-1){
            if(verbose) cout<<"[Scheduler] Invalid MAC address scheduling data SDUs."<<endl;
            exit(1);
        }
        
        for(int j=0;j<numberDataSDUs;j++){
            //Verify if it is possivel to enqueue next SDU
            if((multiplexer->getNumberofBytes() + 2 + sduBuffers->getNextDataSduSize(ueIds[0]))>numberBits/8){
                if(verbose) cout<<"[Scheduler] End of scheduling data SDUs: extrapolated bit capacity."<<endl;
                break;
            }

            //Clear buffer
            bzero(sduBuffer, MAXIMUM_BUFFER_LENGTH);

            //Get SDU from queue
            sduSize = sduBuffers->getNextDataSdu(ueIds[i], sduBuffer);

            //Add SDU to multiplexer
            multiplexer->addSDU(sduBuffer, sduSize, 1);
        }

        //Retreive full PDU from multiplexer if not empty
        if(!multiplexer->isEmpty())
            multiplexer->getPDU(macPDUs[i]->mac_data_);

        //Delete multiplexer
        delete multiplexer;
    }
}

void
Scheduler::scheduleRequestUE(
    MacPDU* macPDU)     //MAC PDU where scheduler will store Multiplexed SDUs
{
    //Define variables
    char sduBuffer[MAXIMUM_BUFFER_LENGTH];  //Buffer to store SDU on aggregation procedure
    size_t sduSize;                         //SDU Size in bytes
    
    /*  #TODO: fill MAC-PDY ctl struct
    macPDUs[i]->macphy_ctl_.
    */

    //Fill MIMO struct
    macPDU->mimo_.scheme = currentParameters->getMimoConf(0)? ((currentParameters->getMimoDiversityMultiplexing(0))? DIVERSITY:MULTIPLEXING):NONE;
    macPDU->mimo_.num_tx_antenas = currentParameters->getMimoAntenna(0);
    macPDU->mimo_.precoding_mtx = currentParameters->getMimoPrecoding(0);

    //Fill MCS struct
    macPDU->mcs_.modulation = mcsToModulation[currentParameters->getMcsUplink(0)];

    //Fill Allocation
    macPDU->allocation_ = currentParameters->getUlReservation(currentParameters->getCurrentMacAddress());

    //Calculate number of bits for next transmission
    size_t numberBits = get_bit_capacity(currentParameters->getNumerology(), macPDU->allocation_, macPDU->mimo_, macPDU->mcs_.modulation);
    if(verbose) cout<<"[Scheduler] Scheduled "<<numberBits/8<<" Bytes for PDU."<<endl;

    //Create a new Multiplexer object to aggregate SDUs
    Multiplexer* multiplexer = new Multiplexer(numberBits/8, currentParameters->getCurrentMacAddress(), 0, verbose);

    //Aggregation procedure - Control SDUs
    int numberControlSDUs = sduBuffers->getNumberControlSdus(0);
    if(numberControlSDUs==-1){
        if(verbose) cout<<"[Scheduler] Invalid MAC address scheduling control SDUs."<<endl;
        exit(1);
    }
    
    for(int i=0;i<numberControlSDUs;i++){
        //Verify if it is possivel to enqueue next SDU
        if((multiplexer->getNumberofBytes() + 2 + sduBuffers->getNextControlSduSize(0))>numberBits/8){
            if(verbose) cout<<"[Scheduler] End of scheduling control SDUs: extrapolated bit capacity."<<endl;
            break;
        }

        //Clear buffer
        bzero(sduBuffer, MAXIMUM_BUFFER_LENGTH);

        //Get SDU from queue
        sduSize = sduBuffers->getNextControlSdu(0, sduBuffer);

        //Add SDU to multiplexer
        multiplexer->addSDU(sduBuffer, sduSize, 0);
    }

    //Aggregation procedure - Data SDUs
    int numberDataSDUs = sduBuffers->getNumberDataSdus(0);
    if(numberDataSDUs==-1){
        if(verbose) cout<<"[Scheduler] Invalid MAC address scheduling data SDUs."<<endl;
        exit(1);
    }
    
    for(int i=0;i<numberDataSDUs;i++){
        //Verify if it is possivel to enqueue next SDU
        if((multiplexer->getNumberofBytes() + 2 + sduBuffers->getNextDataSduSize(0))>numberBits/8){
            if(verbose) cout<<"[Scheduler] End of scheduling data SDUs: extrapolated bit capacity."<<endl;
            break;
        }

        //Clear buffer
        bzero(sduBuffer, MAXIMUM_BUFFER_LENGTH);

        //Get SDU from queue
        sduSize = sduBuffers->getNextDataSdu(0, sduBuffer);

        //Add SDU to multiplexer
        multiplexer->addSDU(sduBuffer, sduSize, 1);
    }

    //Retreive full PDU from multiplexer if not empty
    if(!multiplexer->isEmpty())
        multiplexer->getPDU(macPDU->mac_data_);

    //Delete multiplexer
    delete multiplexer;
}

void
Scheduler::calculateDownlinkSpectrumAllocation(
    MacPDU** macPdus,           //Array of MAC PDUs where scheduler will store Multiplexed SDUs
    uint8_t fusionLutValue)     //Fusion Lookup Table value: array of 4 least significat bits
{
    //Process cases 
    switch(fusionLutValue){
        case 15:    //1111 -> 2x33 RBs for both UEs
            //UE[0]: RBs 0 to 65
            macPdus[0]->allocation_.first_rb = 0;
            macPdus[0]->allocation_.number_of_rb = 66;

            //UE[1]: RBs 66 to 131
            macPdus[1]->allocation_.first_rb = 66;
            macPdus[1]->allocation_.number_of_rb = 66;
        break;

        case 13: case 11:   //1101(13) or 1011(11) -> 2x33 RBs for one UE and 1x33 RBs for the other
            //UE[0]: RBs 0 to 32(flut=11) or 65(flut=13)
            //UE[1]: RBs 66(flut=11) or 99(flut=13) to 131

            macPdus[0]->allocation_.first_rb = 0;

            if(fusionLutValue==11){
                macPdus[0]->allocation_.number_of_rb = 33;
                macPdus[1]->allocation_.first_rb = 66;
                macPdus[1]->allocation_.number_of_rb = 66;
            }
            else{
                macPdus[0]->allocation_.number_of_rb = 66;
                macPdus[1]->allocation_.first_rb = 99;
                macPdus[1]->allocation_.number_of_rb = 33;
            }
        break;

        case 14: case 7:    //1110(14) or 0111(7) -> 1,5x33 RBs for both UEs
            //UE[0]: RBs 0 to 48(flut=14) or 33 to 81(flut=7)
            //UE[1]: RBs 49 to 98(flut=14) or 82 to 131(flut=7)

            if(fusionLutValue==7){
                macPdus[0]->allocation_.first_rb = 33;
                macPdus[1]->allocation_.first_rb = 82;
            }
            else{
                macPdus[0]->allocation_.first_rb = 0;
                macPdus[1]->allocation_.first_rb = 49;
            }

            macPdus[0]->allocation_.number_of_rb = 49;
            macPdus[1]->allocation_.number_of_rb = 50;
        break;

        case 9: case 10: case 12: case 5: case 6: case 3:           //1001, 1010, 1100, 0101, 0110 or 0011 -> 1x33 RBs for both UEs
            //UE[0]: RBs 0 to 32(flut=9,10,12) or 33 to 65(flut=5,6) or 66 to 98(flut=3)
            //UE[1]: RBs 33 to 65(flut=12) or 66 to 98(flut=6,10) or 99 to 131(flut=9,5,3)
            for(int i=0;i<3;i++){
                if((fusionLutValue<<i)&8==8){
                    macPdus[0]->allocation_.first_rb = i*33;
                    break;
                }
            }
            for(int i=0;i<3;i++){
                if((fusionLutValue>>i)&1==1){
                    macPdus[1]->allocation_.first_rb = (3-i)*33;
                    break;
                }
            }
            macPdus[0]->allocation_.number_of_rb = 33;
            macPdus[1]->allocation_.number_of_rb = 33;
        break;

        case 8: case 4: case 2: case 1:     //1000, 0100, 0010 or 0001 -> 0,5x33 RBs for both UEs
            //UE[0]: RBs 0 to 15(flut=8) or 33 to 48(flut=4) or 66 to 81(flut=2) ir 99 to 114(flut=1)
            //UE[1]: RBs 16 to 32(flut=8) or 49 to 65(flut=4) or 82 to 98(flut=2) ir 115 to 131(flut=1)
            
            for(int i=0;i<4;i++){
                if((fusionLutValue<<i)&8==8){
                    macPdus[0]->allocation_.first_rb = 33*i;
                    break;
                }
            }
            macPdus[1]->allocation_.first_rb = macPdus[0]->allocation_.number_of_rb + 16;

            macPdus[0]->allocation_.number_of_rb = 16;
            macPdus[1]->allocation_.number_of_rb = 17;
        break;

        default:
            if(verbose) cout<<"[Scheduler] Invalid Fusion Lookup Table value"<<endl;
        break;
    }
}
