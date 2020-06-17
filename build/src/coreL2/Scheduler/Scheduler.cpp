/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : Scheduler.cpp
@Classification : Scheduler
@
@Last alteration : June 17th, 2020
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module is responsible for Scheduling SDUs into MAC PDUs
            to be transmitted in the next subframe allocating information into RBs.
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

void 
Scheduler::scheduleRequest(
    vector<uint8_t> ueIds,                  //Vector of Ue identifications for next transmission
    vector<size_t> numberSDUs,              //Vector of number of SDUs on buffer for each UE
    vector<size_t> numberBytes,             //Vector of Number of total Bytes on buffer for each UE
    vector<allocation_cfg_t> &allocations)  //Vector where spectrum allocations will be stored
{
    //Define variables
    size_t numberUEs = ueIds.size();        //Number of UEs selected for next transmission
    uint8_t numberAvailableRBs = 0;         //Number of available RBs for next transmission
    size_t totalDesiredAllocation = 0;      //Sum of all desired RB allocations
    uint8_t infoBits;                       //Number of information bits to send
    vector<size_t> desiredRbAllocation;     //Vector of desired allocations for each UE
    vector<uint8_t> rbsAllocation;          //Vector of number of RBs (allocation) for each UE   

    //Resize vectors
    desiredRbAllocation.resize(numberUEs);
    rbsAllocation.resize(numberUEs);

    //Calculate total number of available RBs for next transmission
    for(int i=0;i<4;i++)
        numberAvailableRBs += (((currentParameters->getFLUTMatrix())>>i)&1)*33;

    //For each UE, calculate desired number of RBs to allocate
    for(int i=0;i<numberUEs;i++){
        //Calculate number of info bytes = 2[SA+DA+numSDUs]+2[CRC])*2[no maximo 2 cabeçalhos] + 2[Flag D/C + SizeSDU]*numSDUs + numBytes
        infoBits = 8 + 2*numberSDUs[i] + numberBytes[i];    //Bytes
        infoBits *= 8;  //Convert bytes to bits    

        //Calculate desired RB allocation
        desiredRbAllocation[i] = get_num_required_rb(currentParameters->getNumerology(), currentParameters->getMimo(ueIds[i]),
            mcsToModulation[currentParameters->getMcsDownlink(ueIds[i])], mcsToCodeRate[currentParameters->getMcsDownlink(ueIds[i])], infoBits);
        
        //Increase total desired RB allocation counter
        totalDesiredAllocation += desiredRbAllocation[i];
    }

    //Test if derired allocations exceed total RBs available
    if(totalDesiredAllocation>numberAvailableRBs){      //Then RBs need to be allocated proportionally to desired RB allocation
        for(int i=0;i<numberUEs;i++){
            //Truncating result ignores fractional part of the result and prevents from allocating more RBs than available (but can be less efficient)
            rbsAllocation[i] = trunc((float)desiredRbAllocation[i]*(float)numberAvailableRBs/((float)totalDesiredAllocation));
        }
    }
    else    //Then, RB allocation equals desired RB allocations
    {
        rbsAllocation = vector<uint8_t>(desiredRbAllocation.begin(), desiredRbAllocation.end());
    }
    

    //Declare variables to evaluate actual allocations considering "holes" in channels available
    int rbOffset=0;         //Offset for each RB allocation
    int currentChannel;     //Current spectrum channel (0,1,2 or 3)
    int ueOffset = 0;       //Index to identify current UE

    while(ueOffset<ueIds.size()){       //Do it for all UEs
        //Verify if current channel is free or search for next channel
        //First, get current channel based on RBs offset
        currentChannel = rbOffset/33;       //Doing integer division will ignore fractional part
        //Then, verify if channel is available in Fusion lookup table. If it is not, search for next available channel
        if(rbOffset%33==0){
            while(((currentParameters->getFLUTMatrix()>>currentChannel)&1)!=1){
                currentChannel++;
                rbOffset+=33;
            }
        }

        //Define spectrum allocation structure
        allocations.resize(allocations.size()+1);
        allocations.back().target_ue_id = ueIds[ueOffset];
        allocations.back().first_rb = rbOffset;
        allocations.back().number_of_rb = 0;

        while(rbsAllocation[ueOffset]>0){
            allocations.back().number_of_rb += 1;
            rbsAllocation[ueOffset]--;
            rbOffset++;
            currentChannel = rbOffset/33;   //Doing integer division will ignore fractional part

            //Verify if next RB is idle
            if(rbOffset<132&&rbOffset%33==0){
                if(((currentParameters->getFLUTMatrix()>>currentChannel)&1)!=1){
                    break;
                }
            }
        }

        if(rbsAllocation[ueOffset]>0)
            continue;
        else
            ueOffset++;
    }    
}   

void
Scheduler::fillMacPdus(
    vector<MacPDU> &macPdus)     //Vector of MacPDUs to be filled
{
    uint8_t destinationUeId;                //Current Destination UE Identification
    char sduBuffer[MQ_MAX_MSG_SIZE];        //Buffer to store SDU on aggregation procedure
    size_t sduSize;                         //SDU Size in bytes
    for(int i=0;i<macPdus.size();i++){
        destinationUeId = macPdus[i].allocation_.target_ue_id;

        //Fill Numerology
        macPdus[i].numID_ = currentParameters->getNumerology();

        //Fill MAC - PHY control struct
        macPdus[i].macphy_ctl_.first_tb_in_subframe = i==0;
        macPdus[i].macphy_ctl_.last_tb_in_subframe = i==(macPdus.size()-1);
        macPdus[i].macphy_ctl_.sequence_number = i;

        //Fill MIMO struct
        macPdus[i].mimo_ = currentParameters->getMimo(destinationUeId);

        //Fill MCS struct
        uint8_t mcsValue;   //Value of MCS for current transmission and current destination
        if(currentParameters->isBaseStation()){
            mcsValue = currentParameters->getMcsDownlink(destinationUeId);
        }
        else{
            mcsValue = currentParameters->getMcsUplink(destinationUeId);
            macPdus[i].mcs_.power_offset=currentParameters->getTPC(destinationUeId);
        }
        macPdus[i].mcs_.modulation = mcsToModulation[mcsValue];
        macPdus[i].mcs_.num_coded_bytes = get_bit_capacity(macPdus[i]);

        //Calculate number of bits for next transmission
        size_t numberBytes = get_net_byte_capacity(macPdus[i].numID_, macPdus[i].allocation_, macPdus[i].mimo_, macPdus[i].mcs_.modulation, mcsToCodeRate[mcsValue]);
        if(verbose) cout<<"[Scheduler] Scheduled "<<numberBytes<<" Bytes for PDU "<<i<<endl;

        //Create a new Multiplexer object to aggregate SDUs
        Multiplexer* multiplexer = new Multiplexer(numberBytes, 0, destinationUeId, verbose);

        //Aggregation procedure - Control SDUs
        int numberControlSDUs = sduBuffers->getNumberControlSdus(destinationUeId);
        if(numberControlSDUs==-1){
            if(verbose) cout<<"[Scheduler] Invalid MAC address scheduling control SDUs."<<endl;
            exit(1);
        }
        
        for(int j=0;j<numberControlSDUs;j++){
            //Verify if it is possivel to enqueue next SDU
            if((multiplexer->getNumberofBytes() + 2 + sduBuffers->getNextControlSduSize(destinationUeId))>numberBytes){
                if(verbose) cout<<"[Scheduler] End of scheduling control SDUs: extrapolated bit capacity."<<endl;
                break;
            }

            if(multiplexer->getNumberSdusMultiplexed()==255){
                if(verbose) cout<<"[Scheduler] End of scheduling control SDUs: extrapolated SDU capacity."<<endl;
                break;
            }

            //Clear buffer
            bzero(sduBuffer, MQ_MAX_MSG_SIZE);

            //Get SDU from queue
            sduSize = sduBuffers->getNextControlSdu(destinationUeId, sduBuffer);

            //Test if valid SDU was received
            if(sduSize==-1)
                break;
                
            //Add SDU to multiplexer
            multiplexer->addSDU(sduBuffer, sduSize, 0);
        }

        //Aggregation procedure - Data SDUs
        int numberDataSDUs = sduBuffers->getNumberDataSdus(destinationUeId);
        if(numberDataSDUs==-1){
            if(verbose) cout<<"[Scheduler] Invalid MAC address scheduling data SDUs."<<endl;
            exit(1);
        }
        
        for(int j=0;j<numberDataSDUs;j++){
            //Verify if it is possivel to enqueue next SDU
            if((multiplexer->getNumberofBytes() + 2 + sduBuffers->getNextDataSduSize(destinationUeId))>numberBytes){
                if(verbose) cout<<"[Scheduler] End of scheduling data SDUs: extrapolated bit capacity."<<endl;
                break;
            }

            if(multiplexer->getNumberSdusMultiplexed()==255){
                if(verbose) cout<<"[Scheduler] End of scheduling control SDUs: extrapolated SDU capacity."<<endl;
                break;
            }
            //Clear buffer
            bzero(sduBuffer, MQ_MAX_MSG_SIZE);

            //Get SDU from queue
            sduSize = sduBuffers->getNextDataSdu(destinationUeId, sduBuffer);

            //Test if valid SDU was received
            if(sduSize==-1)
                break;

            //Add SDU to multiplexer
            multiplexer->addSDU(sduBuffer, sduSize, 1);
        }

        //Retreive full PDU from multiplexer if not empty
        if(!multiplexer->isEmpty()){
            multiplexer->getPDU(macPdus[i].mac_data_);
            macPdus[i].mcs_.num_info_bytes = macPdus[i].mac_data_.size();
        }
        else
            macPdus.erase(macPdus.begin()+i);
        
        //Delete multiplexer
        delete multiplexer;
    }
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
