 
/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_LIB_MAC_5G_RANGE_H
#define INCLUDED_LIB_MAC_5G_RANGE_H

#include <cstdint>
#include <vector>
#include "../lib5grange/lib5grange.h"

using namespace std;
using namespace lib5grange;

/**
 * @brief Struct for BSSubframeTx.Start, as defined in L1-L2_InterfaceDefinition.xlsx
 */
typedef struct{
    uint8_t numTRBsDL;                          //Number of transmission resource blocks in downlink
    vector<allocation_cfg_t> ulReservations;   //UpLinkReservations for each UE
    uint8_t numerology;                         //Numerology to be used in downlink
    uint8_t ofdm_gfdm;                          //Flag to indicate data transmission technique for Downlink (0=OFDM; 1=GFDM)
    uint8_t lutDL[17];                          //Fusion Spectrum Analysis LUT
    uint8_t rxMetricPeriodicity;                //CSI periodicity for CQI, PMI, RI and SSM provided by PHY
    
    /**
     * \brief Serializatyion method for the struct
     * This method convert all menbers of the struct to a sequance of bytes and appends at the end
     * of the vector given as argument
     *
     *  @param v: vector of bytes where the struct will be serialized
     **/
    void serialize(vector<uint8_t> & bytes)
    {
        uint8_t auxiliary;
        push_bytes(bytes, numTRBsDL);
        for(int i=0;i<ulReservations.size();i++)
            ulReservations[i].serialize(bytes);
        for(int i=0;i<16;i++)
            push_bytes(bytes, lutDL[i]);
        auxiliary = (numerology<<4)|lutDL[16];
        push_bytes(bytes, auxiliary);
        auxiliary = (ofdm_gfdm<<7)|rxMetricPeriodicity;
        push_bytes(bytes, auxiliary);

    }
    /** deserializatyion method for the struct (inverse order)**/
    void deserialize(vector<uint8_t> & bytes){
        uint8_t auxiliary;
        pop_bytes(auxiliary, bytes);
        rxMetricPeriodicity = auxiliary&15;     //First 4 bits
        ofdm_gfdm = auxiliary>>7;               //Most significant bit

        pop_bytes(auxiliary, bytes);
        numerology = (auxiliary>>4)&15;
        lutDL[16] = auxiliary&15;
        
        for(int i=15;i>=0;i--)
            pop_bytes(lutDL[i],bytes);

        int numberUEsAllocated = (bytes.size()-1)/sizeof(allocation_cfg_t);
        ulReservations.resize(numberUEsAllocated);
        
        for(int i=numberUEsAllocated-1;i>=0;i--)
            ulReservations[i].deserialize(bytes);

        pop_bytes(numTRBsDL, bytes);
    }
}BSSubframeTx_Start;

/**
 * @brief Struct for UESubframeTx.Start, as defined in L1-L2_InterfaceDefinition.xlsx
 */
typedef struct{
    allocation_cfg_t ulReservation;            //UpLinkReservation
    uint8_t numerology;                         //Numerology to be used in downlink
    uint8_t ofdm_gfdm;                          //Flag to indicate data transmission technique for Downlink (0=OFDM; 1=GFDM)
    uint8_t rxMetricPeriodicity;                //CSI periodicity for CQI, PMI, RI and SSM provided by PHY
    
    /**
     * \brief Serializatyion method for the struct
     * This method convert all menbers of the struct to a sequance of bytes and appends at the end
     * of the vector given as argument
     *
     *  @param v: vector of bytes where the struct will be serialized
     **/
    void serialize(vector<uint8_t> & bytes)
    {
        ulReservation.serialize(bytes);

        uint8_t auxiliary = (ofdm_gfdm<<7)|((numerology&7)<<4)|rxMetricPeriodicity;
        push_bytes(bytes, auxiliary);

    }
    /** deserializatyion method for the struct (inverse order)**/
    void deserialize(vector<uint8_t> & bytes){
        uint8_t auxiliary;
        pop_bytes(auxiliary, bytes);
        rxMetricPeriodicity = auxiliary&15;     //First 4 bits
        numerology = (auxiliary>>4)&7;          //Next 3 bits
        ofdm_gfdm = auxiliary>>7;               //Most significant bit

        ulReservation.deserialize(bytes);
    }
}UESubframeTx_Start;



#endif  //INCLUDED_LIB_MAC_5G_RANGE_H