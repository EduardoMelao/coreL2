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
#include <mqueue.h>
#include <sys/resource.h>

#define MQ_PDU_TO_L1 "/mqPduToPhy"
#define MQ_PDU_FROM_L1 "/mqPduFromPhy"
#define MQ_CONTROL_TO_L1 "/mqControlToPhy"
#define MQ_CONTROL_FROM_L1 "/mqControlFromPhy"

#define MQ_MAX_NUM_MSG 100
#define MQ_MAX_MSG_SIZE 204800

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
 * @brief Struct for UESubframeRx.Start, as defined in L1-L2_InterfaceDefinition.xlsx
 */
typedef struct{
    vector<float> snr;  //Signal to Noise Ratio per Resource Block //#TODO: define range
    uint8_t ssm;        //SSM: Spectrum Sensing Measurement. Array of 4 bits
    uint8_t numberPDUs; //Number of PDUs received
    
    /**
     * @brief Serialization method for the struct
     * This method convert all menbers of the struct to a sequance of bytes and appends at the end
     * of the vector given as argument
     *
     * @param bytes: vector of bytes where the struct will be serialized
     **/
    void serialize(vector<uint8_t> & bytes)
    {
        push_bytes(bytes,ssm);
        push_bytes(bytes, numberPDUs);
        serialize_vector(bytes, snr);
    }

    /** deserializatyion method for the struct (inverse order)**/
    void deserialize(vector<uint8_t> & bytes)
    {
        deserialize_vector(snr, bytes);
        pop_bytes(numberPDUs, bytes);
        pop_bytes(ssm, bytes);
    }
}UESubframeRx_Start;

/**
 * @brief Struct for RxMetrics, as defined in L1-L2_InterfaceDefinition.xlsx
 */
typedef struct{
    vector<float> snr;      //Channel Signal to Noise Ratio per Resource Block
    float snr_avg;          //Average Signal to Noise Ratio
    uint8_t rankIndicator;  //Rank Indicator
    uint8_t ssReport;       //SS Report: Spectrum Sensing Report, part of RxMetrics. Array of 4 bits
    
    /**
     * @brief Serialization method for the struct
     * This method convert only snr_avg and rankIndicator menbers of the struct to a sequance of bytes and appends at the end
     * of the vector given as argument
     *
     * @param bytes: vector of bytes where the struct will be serialized
     **/
    void snr_avg_ri_serialize(vector<uint8_t> & bytes)
    {
        push_bytes(bytes, snr_avg);
        push_bytes(bytes, rankIndicator);
    }

    /**
     * @brief Serialization method for the struct
     * This method convert snr and ssr menbers of the struct to a sequance of bytes and appends at the end
     * of the vector given as argument
     *
     * @param bytes: vector of bytes where the struct will be serialized
     **/
    void snr_ssr_serialize(vector<uint8_t> & bytes)
    {
        serialize_vector(bytes, snr);
        push_bytes(bytes, ssReport);
    }

    /** Deserialization method for the struct (inverse order)**/
    void snr_avg_ri_deserialize(vector<uint8_t> & bytes)
    {
        pop_bytes(rankIndicator, bytes);
        pop_bytes(snr_avg, bytes);
    }

    /** Deserialization method for the struct (inverse order)**/
    void snr_ssr_deserialize(vector<uint8_t> & bytes)
    {
        pop_bytes(ssReport, bytes);
        deserialize_vector(snr, bytes);
    }
}RxMetrics;


/**
 * @brief Struct for Message Queues used to interface MAC and PHY
 */
typedef struct{
    mqd_t mqPduToPhy;                       //Message Queue descriptor used to RECEIVE PDUs from L2
    mqd_t mqPduFromPhy;                     //Message Queue descriptor used to SEND PDUs to L2
    mqd_t mqControlToPhy;                   //Message Queue descriptor used to RECEIVE Control Messages from L2
    mqd_t mqControlFromPhy;                 //Message Queue descriptor used to SEND Control Messages to L2

    /**
     * @brief Procedure to create all 4 queues to communicate MAC and PHY
     */
    void createMessageQueues(){
        //Increase System limits according to https://linux.die.net/man/2/setrlimit
        struct rlimit rlim;
        memset(&rlim, 0, sizeof(rlim));
        rlim.rlim_cur = 5*(MQ_MAX_NUM_MSG*MQ_MAX_MSG_SIZE + MQ_MAX_NUM_MSG*sizeof(struct msg_msg *));
        rlim.rlim_max = 5*(MQ_MAX_NUM_MSG*MQ_MAX_MSG_SIZE + MQ_MAX_NUM_MSG*sizeof(struct msg_msg *));
        setrlimit(RLIMIT_MSGQUEUE, &rlim); 

        //Define message queue attributes
        struct mq_attr messageQueueAttributes;
        messageQueueAttributes.mq_maxmsg = MQ_MAX_NUM_MSG;
        messageQueueAttributes.mq_msgsize = MQ_MAX_MSG_SIZE;

        //Open PDU message queues (PDU queues are non-blockable)
        mqPduToPhy = mq_open( MQ_PDU_TO_L1, \
                                O_CREAT|O_RDWR, \
                                0666, \
                                &messageQueueAttributes);
        mqPduFromPhy = mq_open( MQ_PDU_FROM_L1, \
                                O_CREAT|O_RDWR, \
                                0666, \
                                &messageQueueAttributes);   
    
        
        //Open Control message queues
        mqControlToPhy = mq_open( MQ_CONTROL_TO_L1, \
                                O_CREAT|O_RDWR, \
                                0666, \
                                &messageQueueAttributes);
        mqControlFromPhy = mq_open( MQ_CONTROL_FROM_L1, \
                                O_CREAT|O_RDWR, \
                                0666, \
                                &messageQueueAttributes);   
        //Check for errors
        if(mqPduToPhy==-1) 
            perror("Error creating mqPduToPhy message queue");
            
        if(mqPduFromPhy==-1) 
            perror("Error creating mqPduFromPhy message queue");
            
        if(mqControlToPhy==-1) 
            perror("Error creating mqControlToPhy message queue");
            
        if(mqControlFromPhy==-1) 
            perror("Error creating mqControlFromPhy message queue");

        //Clear queues
        clearQueue(mqPduToPhy);
        clearQueue(mqPduFromPhy);
        clearQueue(mqControlToPhy);
        clearQueue(mqControlFromPhy);
    }

    /**
     * @brief This procedure clears the queue when it is being opened
     * @param mQueue
     */
    void clearQueue(mqd_t mQueue){
        struct mq_attr mQueueAttributes;    //Struct to store current messageQueue's number of messages
        char buffer[MQ_MAX_MSG_SIZE];   //Buffer to store provisional message
        //Get attributes
        mq_getattr(mQueue, &mQueueAttributes);

        //Repeat message reception
        for(int i=0;i<mQueueAttributes.mq_curmsgs;i++)
            mq_receive(mQueue, buffer, MQ_MAX_MSG_SIZE, NULL);
        
    }

    /**
     * @brief Procedure to eliminate all Message Queues
     */
    void closeMessageQueues(){
        //Close message queues
        mq_close(mqPduToPhy);
        mq_close(mqPduFromPhy);
        mq_close(mqControlToPhy);
        mq_close(mqControlFromPhy);

        //Unlink message queues
        mq_unlink(MQ_PDU_TO_L1);
        mq_unlink(MQ_PDU_FROM_L1);
        mq_unlink(MQ_CONTROL_TO_L1);
        mq_unlink(MQ_CONTROL_FROM_L1);
    }
}l1_l2_interface_t;
#endif  //INCLUDED_LIB_MAC_5G_RANGE_H