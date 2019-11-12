/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "MacCtHeader.h"

/**
 * @brief Builds a Control Header from static information to be used on encoding
 * @param bs BS flag: true for BS; false for UE
 * @param v Verbosity flag
 */
MacCtHeader::MacCtHeader(bool bs, bool v){
    flagBS = bs;
    verbose = v;
    id = 4;
    ulMCS = 18;
    rbStart = 1;
    numRBs = 2;
    MIMOon = 1;
    MIMOdiv = 0;
    MIMOolcl = 1;
    MIMOantenna = 0;
}

/**
 * @brief Builds a Control Header from a PDU received on decoding and makes possible the use of these informations
 * @param bs BS flag: true for BS; false for UE
 * @param b Buffer containing PDU
 * @param size Size in bytes of PDU
 * @param v Verbosity flag
 */
MacCtHeader::MacCtHeader(bool bs, char* b, int size, bool v){
    flagBS = bs;
    verbose = v;
    id = b[0];
    if(!bs){        //Just UE decoding
        ulMCS = b[1];
        numRBs = b[2];
        rbStart = b[3];
        MIMOon = (b[4]&1);
        MIMOdiv = (b[4]>>1)&1;
        MIMOantenna = (b[4]>>2)&1;
        MIMOolcl = (b[4]>>3)&1;
    }
}

/**
 * @brief Inserts a Control Header in PDU's encoding process
 * @param buf Buf containing PDU to be encoded
 * @param size Size of PDU in bytes
 * @returns New PDU size after Header insertion
 */
ssize_t 
MacCtHeader::insertControlHeader(char* buf, int size){
    //Calculates the difference in header length base on BS flag
    int delta = (flagBS?CONTROLBYTES2UE:CONTROLBYTES2BS);

    //Dynamically allocates buffer
    char *buf2 = new char[size+delta];

    //Inserts header in new buffer
    buf2[0] = id;
    if(flagBS){     //Just BS encoding
        buf2[1] = ulMCS;
        buf2[2] = numRBs;
        buf2[3] = rbStart;
        buf2[4] = (MIMOon&(MIMOdiv<<1)&(MIMOantenna<<2)&(MIMOolcl<<3));
    }
    if(verbose) cout<<"[MacCtHeader] Control Header inserted successfully!"<<endl;

    //Copies the old buffer to the new buffer with header inserted
    memcpy(buf2+delta, buf, size);
    memcpy(buf, buf2, size+delta);
    delete buf2;
    return size+delta;
}

/**
 * @brief Removes a Control Header in PDU's decoding process
 * @param buf Buf containing PDU to be decoded
 * @param size Size of PDU in bytes
 * @returns Size of decoded PDU
 */
ssize_t 
MacCtHeader::removeControlHeader(char* buf, int size){
    //Calculates the difference in header length base on BS flag
    int delta = (flagBS?CONTROLBYTES2BS:CONTROLBYTES2UE);

    //////////PROVISIONAL: IGNORES THE HEADER////////////////////////////////
    //Copies the buffer shifted by delta bytes
    memcpy(buf, buf+delta, size-delta);
    if(verbose) cout<<"[MacCtHeader] Control Header removed successfully!"<<endl;
    return size - delta;
}
