/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2019 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/

#include "MacCtHeader.h"

MacCtHeader::MacCtHeader(
    bool _flagBS,       //BS flag
    bool _verbose)      //Verbosity flag
{
    flagBS = _flagBS;
    verbose = _verbose;
    id = 4;
    ulMCS = 18;
    rbStart = 1;
    numRBs = 2;
    MIMOon = 1;
    MIMOdiv = 0;
    MIMOolcl = 1;
    MIMOantenna = 0;
}

MacCtHeader::MacCtHeader(
    bool _flagBS,       //BS flag
    char* buffer,       //Receiving PDU buffer
    int size,           //Size of PDU in Bytes
    bool _verbose)      //Verbosity flag
{
    flagBS = _flagBS;
    verbose = _verbose;
    id = buffer[0];
    if(!_flagBS){        //Just UE decoding
        ulMCS = buffer[1];
        numRBs = buffer[2];
        rbStart = buffer[3];
        MIMOon = (buffer[4]&1);
        MIMOdiv = (buffer[4]>>1)&1;
        MIMOantenna = (buffer[4]>>2)&1;
        MIMOolcl = (buffer[4]>>3)&1;
    }
}

ssize_t 
MacCtHeader::insertControlHeader(
    char* buffer,       //Encoding PDU buffer
    int size)           //Size of PDU in Bytes
{
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
    memcpy(buf2+delta, buffer, size);
    memcpy(buffer, buf2, size+delta);
    delete buf2;
    return size+delta;
}

ssize_t 
MacCtHeader::removeControlHeader(
    char* buffer,       //Decoding PDU buffer
    int size)           //Size of PDU in bytes
{
    //Calculates the difference in header length base on BS flag
    int delta = (flagBS?CONTROLBYTES2BS:CONTROLBYTES2UE);

    //////////PROVISIONAL: IGNORES THE HEADER////////////////////////////////
    //Copies the buffer shifted by delta bytes
    memcpy(buffer, buffer+delta, size-delta);
    if(verbose) cout<<"[MacCtHeader] Control Header removed successfully!"<<endl;
    return size - delta;
}
