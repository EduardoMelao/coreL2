#include "MacCtHeader.h"

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

ssize_t MacCtHeader::insertControlHeader(char* buf, int size){
    int delta = (flagBS?CONTROLBYTES2UE:CONTROLBYTES2BS);
    char *buf2 = new char[size+delta];
    buf2[0] = id;
    if(flagBS){     //Just BS encoding
        buf2[1] = ulMCS;
        buf2[2] = numRBs;
        buf2[3] = rbStart;
        buf2[4] = (MIMOon&(MIMOdiv<<1)&(MIMOantenna<<2)&(MIMOolcl<<3));
    }
    if(verbose) cout<<"[MacCtHeader] Control Header inserted successfully!"<<endl;
    memcpy(buf2+delta, buf, size);
    memcpy(buf, buf2, size+delta);
    delete buf2;
    return size+delta;
}

ssize_t MacCtHeader::removeControlHeader(char* buf, int size){
    int delta = (flagBS?CONTROLBYTES2BS:CONTROLBYTES2UE);
    memcpy(buf, buf+delta, size-delta);
    if(verbose) cout<<"[MacCtHeader] Control Header removed successfully!"<<endl;
    return size - delta;
}
