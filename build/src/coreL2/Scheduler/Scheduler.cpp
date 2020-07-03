/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : Scheduler.cpp
@Classification : Scheduler
@
@Last alteration : July 2nd, 2020
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
Scheduler::scheduleRequestBS(
    vector<uint8_t> ueIds,                  //Vector of Ue identifications for next transmission
    vector<size_t> numberSDUs,              //Vector of number of SDUs on buffer for each UE
    vector<size_t> numberBytes,             //Vector of Number of total Bytes on buffer for each UE
    vector<allocation_cfg_t> &allocations)  //Vector where spectrum allocations will be stored
{
    //Define variables
    size_t numberUEs = ueIds.size();        //Number of UEs selected for next transmission
    uint8_t numberAvailableRBs = 0;         //Number of available RBs for next transmission
    size_t totalDesiredAllocation = 0;      //Sum of all desired RB allocations
    size_t infoBits;                        //Number of information bits to send
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

        if(verbose) cout<<"[Scheduler] BS has "<<infoBits/8<<" bytes to transmit to UE "<<(int)ueIds[i]<<" and needs "<<desiredRbAllocation[i]<<" RBs."<<endl;
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
Scheduler::scheduleRequestUE(
    size_t numberSDUs,              //Number of SDUs on buffer for BS
    size_t numberBytes,             //Number of total Bytes on buffer for BS
    allocation_cfg_t &allocation)   //Spectrum allocations will be stored
{
    //Get starting allocation that equals uplink reservation
    allocation = currentParameters->getUlReservation(currentParameters->getCurrentMacAddress());
    allocation.target_ue_id = 0;    //Target UEID for an UE is BS

    //Define variables
    uint8_t numberAvailableRBs = allocation.number_of_rb;   //Number of available RBs for next transmission
    size_t desiredRbAllocation;                             //Desired RB allocation 
    size_t infoBits;                                        //Number of information bits to send

    //Calculate number of info bytes = 2[SA+DA+numSDUs]+2[CRC])*2[no maximo 2 cabeçalhos] + 2[Flag D/C + SizeSDU]*numSDUs + numBytes
    infoBits = 8 + 2*numberSDUs + numberBytes;    //Bytes
    infoBits *= 8;  //Convert bytes to bits    

    //Calculate desired RB allocation
    desiredRbAllocation = get_num_required_rb(currentParameters->getNumerology(), currentParameters->getMimo(0),
        mcsToModulation[currentParameters->getMcsUplink(0)], mcsToCodeRate[currentParameters->getMcsUplink(0)], infoBits);
    
    allocation.number_of_rb = desiredRbAllocation > numberAvailableRBs ? numberAvailableRBs:desiredRbAllocation;

    if(verbose) cout<<"[Scheduler] UE "<<(int)currentParameters->getCurrentMacAddress()<<" has "<<infoBits/8<<" bytes to transmit and needs "<<desiredRbAllocation<<" RBs."<<endl;
}

void
Scheduler::fillMacPdus(
    vector<MacPDU> &macPdus)     //Vector of MacPDUs to be filled
{
    uint8_t destinationUeId;                //Current Destination UE Identification
    char sduBuffer[MQ_MAX_MSG_SIZE];        //Buffer to store SDU on aggregation procedure
    size_t sduSize;                         //SDU Size in bytes
    vector<MacPDU>::iterator pduIterator;   //Iterator for MacPDUs vector

    //Set iterator to vector begin
    pduIterator = macPdus.begin();          

    //Iterate each PDU filling it
    while(pduIterator != macPdus.end()){
        //Get destination UeID for this PDU
        destinationUeId = pduIterator->allocation_.target_ue_id;

        //Fill Numerology
        pduIterator->numID_ = currentParameters->getNumerology();

        //Fill MAC - PHY control struct
        pduIterator->macphy_ctl_.first_tb_in_subframe = pduIterator==macPdus.begin();
        pduIterator->macphy_ctl_.last_tb_in_subframe = pduIterator==(macPdus.end()-1);
        pduIterator->macphy_ctl_.sequence_number = pduIterator - macPdus.begin();

        //Fill MIMO struct
        pduIterator->mimo_ = currentParameters->getMimo(destinationUeId);

        //Fill MCS struct
        uint8_t mcsValue;   //Value of MCS for current transmission and current destination
        if(currentParameters->isBaseStation()){
            mcsValue = currentParameters->getMcsDownlink(destinationUeId);
        }
        else{
            mcsValue = currentParameters->getMcsUplink(destinationUeId);
            pduIterator->mcs_.power_offset=currentParameters->getTPC(destinationUeId);
        }
        pduIterator->mcs_.modulation = mcsToModulation[mcsValue];
        pduIterator->mcs_.num_coded_bytes = get_bit_capacity(*pduIterator)/8;

        //Calculate number of bits for next transmission
        size_t numberBytes = get_net_byte_capacity(pduIterator->numID_, pduIterator->allocation_, pduIterator->mimo_, pduIterator->mcs_.modulation, mcsToCodeRate[mcsValue]);
        if(verbose) cout<<"[Scheduler] Scheduled "<<numberBytes<<" Bytes for PDU "<<(int)pduIterator->macphy_ctl_.sequence_number<<" destinated to UE "<<(int)pduIterator->allocation_.target_ue_id<<endl;

        //Create a new Multiplexer object to aggregate SDUs
        Multiplexer* multiplexer = new Multiplexer(numberBytes, currentParameters->getCurrentMacAddress(), destinationUeId, verbose);

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
        
        //Lock Data Queue modifications semaphore
        sduBuffers->lockDataSduQueue();
        
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
        //Release Data Queue modifications semaphore
        sduBuffers->unlockDataSduQueue();

        //Retreive full PDU from multiplexer if not empty
        if(!multiplexer->isEmpty()){
            multiplexer->getPDU(pduIterator->mac_data_);
            pduIterator->mcs_.num_info_bytes = pduIterator->mac_data_.size();
            //Avance PDU iterator
            pduIterator++;
        }
        else
            macPdus.erase(pduIterator);
        
        //Delete multiplexer
        delete multiplexer;
    }
}