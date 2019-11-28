/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#ifndef INCLUDED_L1_L2_INTERFACE_H
#define INCLUDED_L1_L2_INTERFACE_H

#include <iostream>
#include <vector>
#include "../../common/lib5grange/lib5grange.h"
#include "../../coreL1/CoreL1.h"

using namespace std;
using namespace lib5grange;

class L1L2Interface{
private:
    MacPDU macPDU;                              //MAC PDU object that will encapsulate all information and will be sent to PHY
    unsigned ueID;                              //User Equipment Identification
    unsigned numerologyID;                      //Numerology identification (0 - 5)
    float codeRate;                             //Code Rate
    macphyctl_t macPhyControl;                  //MAC to PHY control struct
    allocation_cfg_t allocationConfiguration;   //Struct with configuration of the resource allocation
    mimo_cfg_t mimoConfiguration;               //Struct with MIMO configuration
    mcs_cfg_t mcsConfiguration;                 //Struct regarding modulation and coding information
    vector<uint8_t> macData;                    //Uncoded bytes from MAC
    vector<uint8_t> macControl;                 //Uncoded control bits to be sent to PHY
    CoreL1* l1;  
    
    /**
    * @brief Auxiliary function for CRC calculation
    * @param data Single byte from PDU
    * @param crc CRC history
    * @returns 2-byte CRC calculation
    */
    unsigned short auxiliaryCalculationCRC(char data, unsigned short crc);                               //[Stub] CoreL1 object that performs sending and receiving operations in PHY level
public:
    /**
     * @brief Constroys a L1L2Interface object and initializes class variables with static information
     * @param l1 L1 object initialized with static information
     */
    L1L2Interface(CoreL1* _l1);

    /**
     * @brief Destroys a L1L2Interface object
     */
    ~L1L2Interface();

    /**
     * @param buffer Information buffer
     * @param size Size of information in bytes
     * @param controlBuffer Buffer with Control information
     * @param controlSize Control information size in bytes
     * @param port Socket port to identify which socket to send information
     * @returns True if transmission was successful, false otherwise
     */
    bool sendPdu(uint8_t* buffer, size_t size, uint8_t* controlBuffer, size_t controlSize, uint16_t port);

    /**
     * @brief Received a PDU from PHY Layer
     * @param buffer Buffer where PDU is going to be store
     * @param maximumSize Maximum size of PDU
     * @param port Socket port to identify which socket to listen for information
     * @returns Received PDU size in bytes
     */
    ssize_t receivePdu(const char* buffer, size_t maximumSize, uint16_t port);

    /**
     * @brief Get ports currently added to CoreL1 object
     * @returns Array of ports
     */
    uint16_t* getPorts();

    /**
     * @brief Calculates CRC of current PDU passed as parameter
     * @param buffer Bytes of current PDU
     * @param size Size of PDU in bytes
     */
    void crcPackageCalculate(char* buffer, int size);

    /**
     * @brief Checks if CRC contained in received PDU matches calculated CRC
     * @param buffer Bytes of current PDU
     * @param size Size of PDU in bytes
     * @returns True if CRC match; False otherwise
     */
    bool crcPackageChecking(char* buffer, int size);
};
#endif  //INCLUDED_L1_L2_INTERFACE_H