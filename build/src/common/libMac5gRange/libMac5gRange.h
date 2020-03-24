/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_LIB_MAC_5G_RANGE_H
#define INCLUDED_LIB_MAC_5G_RANGE_H

#include <cstdint>
#include <vector>
#include "../lib5grange/lib5grange.h"
#include <mutex>

using namespace std;
using namespace lib5grange;

enum MacModes {STANDBY_MODE, CONFIG_MODE, START_MODE, IDLE_MODE, RECONFIG_MODE, STOP_MODE};
enum MacTxModes {ACTIVE_MODE_TX, DISABLED_MODE_TX};
enum MacRxModes {ACTIVE_MODE_RX, DISABLED_MODE_RX};
enum MacTunModes {TUN_ENABLED, TUN_DISABLED};


/**
 * @brief Struct for BSSubframeTx.Start, as defined in L1-L2_InterfaceDefinition.xlsx
 */
typedef struct{
	uint8_t numUEs;								//Total number of UEs in the system/network
    uint8_t numPDUs;                          	//Number of MAC PDUs to transmit in next subframe
    vector<allocation_cfg_t> ulReservations;   	//UpLinkReservations for each UE
    uint8_t numerology;                         //Numerology to be used in downlink
    uint8_t ofdm_gfdm;                          //Flag to indicate data transmission technique for Downlink (0=OFDM; 1=GFDM)
    uint8_t fLutDL;                             //Fusion Spectrum Analysis LUT
    uint8_t rxMetricPeriodicity;                //CSI periodicity for CQI, PMI, RI and SSM provided by PHY
    
    /**
     * @brief Serializatyion method for the struct
     * This method convert all menbers of the struct to a sequance of bytes and appends at the end
     * of the vector given as argument
     *
     * @param bytes: vector of bytes where the struct will be serialized
     */
    void serialize(vector<uint8_t> & bytes)
    {
        uint8_t auxiliary;
        push_bytes(bytes, numUEs);
        push_bytes(bytes, numPDUs);
        for(int i=0;i<ulReservations.size();i++)
            ulReservations[i].serialize(bytes);
        auxiliary = (numerology<<4)|(fLutDL&15);
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
        fLutDL = auxiliary&15;

        int numberUEsAllocated = (bytes.size()-1)/sizeof(allocation_cfg_t);
        ulReservations.resize(numberUEsAllocated);
        
        for(int i=numberUEsAllocated-1;i>=0;i--)
            ulReservations[i].deserialize(bytes);

        pop_bytes(numPDUs, bytes);
        pop_bytes(numUEs, bytes);
    }
}BSSubframeTx_Start;

/**
 * @brief Struct for UESubframeTx.Start, as defined in L1-L2_InterfaceDefinition.xlsx
 */
typedef struct{
    allocation_cfg_t ulReservation;             //UpLinkReservation
    uint8_t numerology;                         //Numerology to be used in Downlink
    uint8_t ofdm_gfdm;                          //Flag to indicate data transmission technique for Downlink (0=OFDM; 1=GFDM)
    uint8_t rxMetricPeriodicity;                //CSI periodicity for CQI, PMI, RI and SSM provided by PHY
    
    /**
     * @brief Serializatyion method for the struct
     * This method convert all menbers of the struct to a sequance of bytes and appends at the end
     * of the vector given as argument
     *
     * @param bytes: vector of bytes where the struct will be serialized
     */
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

/**
 * @brief Struct for BSSubframeRx.Start, as defined in L1-L2_InterfaceDefinition.xlsx
 */
typedef struct{
    float snr;         //Signal to Noise Ratio //#TODO: define range.
    
    /**
     * @brief Serialization method for the struct
     * This method convert all menbers of the struct to a sequance of bytes and appends at the end
     * of the vector given as argument
     *
     * @param bytes: vector of bytes where the struct will be serialized
     */
    void serialize(vector<uint8_t> & bytes)
    {
        push_bytes(bytes, snr);
    }

    /** deserializatyion method for the struct (inverse order)**/
    void deserialize(vector<uint8_t> & bytes)
    {
        pop_bytes(snr, bytes);
    }
}BSSubframeRx_Start;

/**
 * @brief Struct for UESubframeRx.Start, as defined in L1-L2_InterfaceDefinition.xlsx
 */
typedef struct{
    float snr;          //Signal to Noise Ratio                                 //#TODO: define range
    uint8_t ri;         //RI (Rank Indicator), part of RxMetrics.               //#TODO: define range
    uint8_t pmi;        //PMI: Pre-Coding Matrix Indicator, part of RxMetrics.  //#TODO: define range
    uint8_t ssm;    //SSM: Spectrum Sensing Measurement. Array of 4 bits
    
    /**
     * @brief Serialization method for the struct
     * This method convert all menbers of the struct to a sequance of bytes and appends at the end
     * of the vector given as argument
     *
     * @param bytes: vector of bytes where the struct will be serialized
     **/
    void serialize(vector<uint8_t> & bytes)
    {
        uint8_t auxiliary;
        push_bytes(bytes, snr);
        push_bytes(bytes, pmi);
        auxiliary = (ri<<4)|(ssm&15);
        push_bytes(bytes, auxiliary);
    }

    /** deserializatyion method for the struct (inverse order)**/
    void deserialize(vector<uint8_t> & bytes)
    {
        uint8_t auxiliary;
        pop_bytes(auxiliary, bytes);
        ri = (auxiliary>>4)&15;
        ssm = auxiliary&15;
        
        pop_bytes(pmi, bytes);

        pop_bytes(snr, bytes);
    }
}UESubframeRx_Start;

/**
 * @brief Struct for RxMetrics, as defined in L1-L2_InterfaceDefinition.xlsx
 */
typedef struct{
    float snr;              //Channel Signal to Noise Ratio
    uint8_t ri;             //RI (Rank Indicator), part of RxMetrics.               //#TODO: define range
    uint8_t pmi;            //PMI: Pre-Coding Matrix Indicator, part of RxMetrics.  //#TODO: define range
    uint8_t ssReport;       //SS Report: Spectrum Sensing Report, part of RxMetrics. Array of 4 bits
    
    /**
     * @brief Serialization method for the struct
     * This method convert all menbers of the struct to a sequance of bytes and appends at the end
     * of the vector given as argument
     *
     * @param bytes: vector of bytes where the struct will be serialized
     **/
    void serialize(vector<uint8_t> & bytes)
    {
        uint8_t auxiliary;
        push_bytes(bytes, snr);
        push_bytes(bytes, pmi);
        auxiliary = (ri<<4)|(ssReport&15);
        push_bytes(bytes, auxiliary);
    }

    /** deserializatyion method for the struct (inverse order)**/
    void deserialize(vector<uint8_t> & bytes)
    {
        uint8_t auxiliary;
        pop_bytes(auxiliary, bytes);
        ri = (auxiliary>>4)&15;
        ssReport = auxiliary&15;
        
        pop_bytes(pmi, bytes);

        pop_bytes(snr, bytes);
    }
}RxMetrics;
#endif  //INCLUDED_LIB_MAC_5G_RANGE_H