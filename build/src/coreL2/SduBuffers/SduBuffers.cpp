/* ***************************************/
/* ***************************************/
/* Copyright Notice                      */
/* Copyright(c)2020 5G Range Consortium  */
/* All rights Reserved                   */
/*****************************************/
/**
@Arquive name : SduBuffers.cpp
@Classification : SDU Buffers
@
@Last alteration : March 27th, 2020
@Responsible : Eduardo Melao
@Email : emelao@cpqd.com.br
@Telephone extension : 7015
@Version : v1.0

Project : H2020 5G-Range

Company : Centro de Pesquisa e Desenvolvimento em Telecomunicacoes (CPQD)
Direction : Diretoria de Operações (DO)
UA : 1230 - Centro de Competencia - Sistemas Embarcados

@Description : This module enqueues L3 packets received from TUN interface as Data SDUs
(MACD SDUs). It also enqueues Control SDUs (MACC SDUs) and is responsible to report BSI to Scheduler.
*/

#include "SduBuffers.h"

SduBuffers::SduBuffers(
    ReceptionProtocol* _reception,          //Object to receive packets from L3
    CurrentParameters* _currentParameters,  //Object with the parameters that are currently being used by the system
    MacAddressTable* _ipMacTable,           //Table of correlation of IP Addresses and MAC5GR Addresses
    bool _verbose)                          //Verbosity flag
{
    //Assign class variables
    reception = _reception;
    currentParameters = _currentParameters;
    ipMacTable = _ipMacTable;
    verbose = _verbose;

    //Resize vectors
    dataSduQueue.resize(currentParameters->getNumberUEs());
    controlSduQueue.resize(currentParameters->getNumberUEs());
    dataSizes.resize(currentParameters->getNumberUEs());
    controlSizes.resize(currentParameters->getNumberUEs());
}

SduBuffers::~SduBuffers(){
    //Delete Dynamically allocated buffers for SDUs
    while(dataSduQueue.size()>0){
        while(dataSduQueue[0].size()>0){
            char* buffer1 = dataSduQueue[0].front();
            char* buffer2 = controlSduQueue[0].front();
            delete [] buffer1;
            delete [] buffer2;
            dataSduQueue[0].erase(dataSduQueue[0].begin());
            controlSduQueue[0].erase(controlSduQueue[0].begin());
        }
        dataSduQueue.erase(dataSduQueue.begin());
    }
}

void 
SduBuffers::enqueueingDataSdus()
{
    char buffer[MAXIMUM_BUFFER_LENGTH];
    ssize_t numberBytesRead = 0;
    
    //Mark current MAC Tun mode as ENABLED for reading TUN interface and enqueueing Data SDUs.
    currentParameters->setMacTunMode(TUN_ENABLED);

    //Loop will execute until STOP mode is activated
    while(currentParameters->getMacMode()!=STOP_MODE){
        //Allocate buffer
        bzero(buffer, MAXIMUM_BUFFER_LENGTH);

        //Read from TUN Interface
        numberBytesRead = reception->receivePackageFromL3(buffer, MAXIMUM_BUFFER_LENGTH);

        //Check if there is actually information received
        if(numberBytesRead<0){
            continue;
        }

        else
        {
            //Lock to write in the queue
            lock_guard<mutex> lk(dataMutex);
            
            //Check EOF
            if(numberBytesRead==0){
                if(verbose) cout<<"[SduBuffers] End of Transmission."<<endl;
            	break;
            }

            //Check ipv4
            if(((buffer[0]>>4)&15) != 4){
                if(verbose) cout<<"[SduBuffers] Dropped non-ipv4 packet."<<endl;
                continue;
            }

            //Check broadcast
            if((buffer[DST_OFFSET]==255)&&(buffer[DST_OFFSET+1]==255)&&(buffer[DST_OFFSET+2]==255)&&(buffer[DST_OFFSET+3]==255)){
                if(verbose) cout<<"[SduBuffers] Dropped broadcast packet."<<endl;
                continue;
            }

            //Check multicast
            if((buffer[DST_OFFSET]>=224)&&(buffer[DST_OFFSET]<=239)){
                if(verbose) cout<<"[SduBuffers] Dropped multicast packet."<<endl;
                continue;
            }
            
            //Everything is ok, buffer can be added to queue
            //First, consult destination MAC address based on IP Address
            int index = currentParameters->getIndex(getMacAddress(buffer));

            
            //Test if index exists
            if(index!=-1){
                //Allocate new buffer and copy information
                char* bufferDataSdu = new char[numberBytesRead];
                for(int i=0;i<numberBytesRead;i++)
                    bufferDataSdu[i]=buffer[i];
                
                dataSduQueue[index].push_back(bufferDataSdu);
                dataSizes[index].push_back(numberBytesRead);
            }
            else
                if(verbose) cout<<"[SduBuffers] Data SDU could not be added to queue: index not found."<<endl;
            
            if(verbose) cout<<"[SduBuffers] Data SDU added to Queue "<<index<<". Num SDUs: "<<dataSduQueue[index].size()<<endl;
        }
    }

    if(verbose) cout<<"[SduBuffers] Entering STOP_MODE."<<endl;

    //Mark current MAC Tun mode as DISABLED for reading TUN interface and enqueueing Data SDUs.
    currentParameters->setMacTunMode(TUN_DISABLED);
}

uint8_t
SduBuffers::getMacAddress(
    char* dataSdu)          //SDU containg IP bytes
{
    uint8_t ipAddress[4];   //Destination IP address encapsulated into SDU

    //Gets IP Address from packet
    for(int i=0;i<4;i++)
        ipAddress[i] = (uint8_t) dataSdu[DST_OFFSET+i]; //Copying IP address
    
    //Search IP Address in MacAddressTable and returns it
    return ipMacTable->getMacAddress(ipAddress);
}

void
SduBuffers::enqueueControlSdu(
    uint8_t* controlSdu,
    size_t numberBytes,
    uint8_t macAddress)
{
    lock_guard<mutex> lk(controlMutex);

    //Dynamically allocate buffer for SDU
    char* buffer = new char[numberBytes];

    //Copy SDU to buffer
    for(int i=0;i<numberBytes;i++)
        buffer[i]=controlSdu[i];

    int index = currentParameters->getIndex(macAddress);
            
    //Test if index exists
    if(index!=-1){
        controlSduQueue[index].push_back(buffer);
        controlSizes[index].push_back(numberBytes);
    }
    else
        if(verbose) cout<<"[SduBuffers] Control SDU could not be added to queue: index not found."<<endl;
    
    if(verbose) cout<<"[SduBuffers] Control SDU added to Queue "<<index<<". Num SDUs: "<<controlSduQueue[index].size()<<endl;

}

int 
SduBuffers::getNumberDataSdus(
    uint8_t macAddress)     //Destination MAC Address
{
    int index = currentParameters->getIndex(macAddress);
    if(index!=-1)
        return dataSduQueue[index].size();
    if(verbose) cout<<"[SduBuffers] Index not found getting number of Data SDUs."<<endl;
    return -1;
}

int 
SduBuffers::getNumberControlSdus(
    uint8_t macAddress)     //Destination MAC Address
{
    int index = currentParameters->getIndex(macAddress);
    if(index!=-1)
        return controlSduQueue[index].size();
    if(verbose) cout<<"[SduBuffers] Index not found getting number of Control SDUs."<<endl;
    return -1;
}

bool
SduBuffers::bufferStatusInformation(
    uint8_t macAddress)
{
    return (getNumberDataSdus(macAddress)!=0)||(getNumberControlSdus(macAddress)!=0);
}

bool
SduBuffers::bufferStatusInformation(){
    bool returnValue = false;
    for(int i=0;i<currentParameters->getNumberUEs();i++){
        returnValue = returnValue||bufferStatusInformation(currentParameters->getMacAddress(i));
    }
    return returnValue;
}

ssize_t 
SduBuffers::getNextDataSdu(
    uint8_t macAddress,     //Destination MAC Address
    char* buffer)           //Buffer to store the SDU
{          
    ssize_t returnValue;   //Return value
    int index = currentParameters->getIndex(macAddress);

    //Lock mutex to remove SDU from the head of the queue
    lock_guard<mutex> lk(dataMutex);
    if(dataSduQueue[index].size()==0){
        if(verbose) cout<<"[SduBuffers] Tried to get empty SDU from L3."<<endl;
        return -1;
    }

    //Get front values from the vectors
    returnValue = dataSizes[index].front();
    char* buffer2 = dataSduQueue[index].front();
    for(int i=0;i<returnValue;i++)
        buffer[i] = buffer2[i];           //Copying
    
    delete [] buffer2;

    //Delete front positions
    dataSizes[index].erase(dataSizes[index].begin());
    dataSduQueue[index].erase(dataSduQueue[index].begin());

    if(verbose) cout<<"[SduBuffers] Got SDU from L3 queue."<<endl;

    return returnValue;
}

ssize_t 
SduBuffers::getNextControlSdu(
    uint8_t macAddress,     //Destination MAC Address
    char* buffer)           //Buffer to store the SDU
{          
    ssize_t returnValue;   //Return value
    int index = currentParameters->getIndex(macAddress);

    //Lock mutex to remove SDU from the head of the queue
    lock_guard<mutex> lk(controlMutex);
    if(controlSduQueue[index].size()==0){
        if(verbose) cout<<"[SduBuffers] Tried to get empty SDU from control SDU queue."<<endl;
        return -1;
    }

    //Get front values from the vectors
    returnValue = controlSizes[index].front();
    char* buffer2 = controlSduQueue[index].front();
    for(int i=0;i<returnValue;i++)
        buffer[i] = buffer2[i];           //Copying
    
    delete [] buffer2;

    //Delete front positions
    controlSizes[index].erase(controlSizes[index].begin());
    controlSduQueue[index].erase(controlSduQueue[index].begin());

    if(verbose) cout<<"[SduBuffers] Got SDU from control SDU queue."<<endl;

    return returnValue;
}

ssize_t 
SduBuffers::getNextDataSduSize(
    uint8_t macAddress)     //Destination MAC Address
{          
    int index = currentParameters->getIndex(macAddress);

    //Lock mutex to remove SDU from the head of the queue
    lock_guard<mutex> lk(dataMutex);
    if(dataSduQueue[index].size()==0){
        if(verbose) cout<<"[SduBuffers] Tried to get empty SDU from L3."<<endl;
        return -1;
    }

    //Get front values from the vectors
    return dataSizes[index].front();
}

ssize_t 
SduBuffers::getNextControlSduSize(
    uint8_t macAddress)     //Destination MAC Address
{          
    int index = currentParameters->getIndex(macAddress);

    //Lock mutex to remove SDU from the head of the queue
    lock_guard<mutex> lk(controlMutex);
    if(controlSduQueue[index].size()==0){
        if(verbose) cout<<"[SduBuffers] Tried to get empty SDU from L3."<<endl;
        return -1;
    }

    //Get front values from the vectors
    return controlSizes[index].front();
}
