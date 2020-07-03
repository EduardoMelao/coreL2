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
@Last alteration : July 2nd, 2020
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
    TimerSubframe* _timerSubframe,          //Timer with Subframe indication to support IP timeout control
    bool _verbose)                          //Verbosity flag
{
    uint8_t numberUEs = _currentParameters->getNumberUEs();  //Number of UEs in the system
    
    //Assign class variables
    reception = _reception;
    currentParameters = _currentParameters;
    ipMacTable = _ipMacTable;
    timerSubframe = _timerSubframe;
    verbose = _verbose;

    //Resize vectors
    dataSduQueue.resize(numberUEs);
    controlSduQueue.resize(numberUEs);
    dataSizes.resize(numberUEs);
    controlSizes.resize(numberUEs);
    dataTimestamp.resize(numberUEs);
    totalDataBufferBytes.resize(numberUEs);
    totalControlBufferBytes.resize(numberUEs);

    //Initialize Byte counters
    for(int i=0; i<numberUEs; i++){
        totalControlBufferBytes[i] = 0;
        totalDataBufferBytes[i] = 0;
    }
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
    char buffer[MQ_MAX_MSG_SIZE];
    ssize_t numberBytesRead = 0;
    
    //Mark current MAC Tun mode as ENABLED for reading TUN interface and enqueueing Data SDUs.
    currentParameters->setMacTunMode(TUN_ENABLED);

    //Loop will execute until STOP mode is activated
    while(currentParameters->getMacMode()!=STOP_MODE){
        //Allocate buffer
        bzero(buffer, MQ_MAX_MSG_SIZE);

        //Read from TUN Interface
        numberBytesRead = reception->receivePackageFromL3(buffer, MQ_MAX_MSG_SIZE);

        //Check if there is actually information received
        if(numberBytesRead<0){
            continue;
        }

        else
        {
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
            
            //Lock to write in the queue
            lock_guard<mutex> lk(dataMutex);
            
            //Everything is ok, buffer can be added to queue
            //First, consult destination MAC address based on IP Address is it is BS
            //If it is UE, always forward Data packet to BS
            int index = currentParameters->isBaseStation()? currentParameters->getIndex(getMacAddress(buffer)) : 0;

            //Test if index exists
            if(index!=-1){
                //Allocate new buffer and copy information
                char* bufferDataSdu = new char[numberBytesRead];
                for(int i=0;i<numberBytesRead;i++)
                    bufferDataSdu[i]=buffer[i];
                
                //Push back buffer to class arrays and increase byte counter
                dataSduQueue[index].push_back(bufferDataSdu);
                dataSizes[index].push_back(numberBytesRead);
                totalDataBufferBytes[index] += numberBytesRead;

                dataTimestamp[index].push_back(timerSubframe->getSubframeNumber());
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

    //Test if index exists and, if yes, push back buffer to clas arrays and increase byte counter
    if(index!=-1){
        controlSduQueue[index].push_back(buffer);
        controlSizes[index].push_back(numberBytes);
        totalControlBufferBytes[index] += numberBytes;
    }
    else
        if(verbose) cout<<"[SduBuffers] Control SDU could not be added to queue: index not found."<<endl;
    
    if(verbose) cout<<"[SduBuffers] Control SDU added to Queue "<<index<<". Num SDUs: "<<controlSduQueue[index].size()<<endl;
}

ssize_t 
SduBuffers::getNumberDataSdus(
    uint8_t macAddress)     //Destination MAC Address
{
    int index = currentParameters->getIndex(macAddress);
    if(index!=-1)
        return dataSduQueue[index].size();
    if(verbose) cout<<"[SduBuffers] Index not found getting number of Data SDUs."<<endl;
    return -1;
}

ssize_t 
SduBuffers::getNumberControlSdus(
    uint8_t macAddress)     //Destination MAC Address
{
    int index = currentParameters->getIndex(macAddress);
    if(index!=-1)
        return controlSduQueue[index].size();
    if(verbose) cout<<"[SduBuffers] Index not found getting number of Control SDUs."<<endl;
    return -1;
}

void
SduBuffers::bufferStatusInformation(
    vector<uint8_t> &ueIds,         //Vector of selected UE identifications
    vector<size_t> &numberSDUs,     //Vector of Buffer status (SDUs to transmit) for each UE selected
    vector<size_t> &numberBytes)    //Vector of Buffer status (Bytes to transmit) for each UE selected
{
    uint8_t macAddress;     //Current UE Identification
    int numSdus;            //Total Number of DataSdus and ControlSdus

    for(int i=0;i<currentParameters->getNumberUEs();i++){
        //Get current MacAddress
        macAddress = currentParameters->getMacAddress(i);

        //Get current number of SDUs
        numSdus = dataSduQueue[i].size() + controlSduQueue[i].size();

        //Select UE if there are SDUs to transmit
        if(numSdus>0){
            ueIds.push_back(macAddress);
            numberSDUs.push_back(numSdus);
            numberBytes.push_back(totalDataBufferBytes[i]+totalControlBufferBytes[i]);
        }
    }
}

ssize_t 
SduBuffers::getNextDataSdu(
    uint8_t macAddress,     //Destination MAC Address
    char* buffer)           //Buffer to store the SDU
{          
    ssize_t returnValue;   //Return value
    int index = currentParameters->getIndex(macAddress);

    //Remove SDU from the head of the queue
    if(dataSduQueue[index].size()==0){
        if(verbose) cout<<"[SduBuffers] Tried to get empty SDU from L3."<<endl;
        return -1;
    }

    //Get front values from the vectors and decrease Byte count
    returnValue = dataSizes[index].front();
    char* buffer2 = dataSduQueue[index].front();
    for(int i=0;i<returnValue;i++)
        buffer[i] = buffer2[i];           //Copying
    
    delete [] buffer2;

    //Delete front positions
    dataSizes[index].erase(dataSizes[index].begin());
    dataSduQueue[index].erase(dataSduQueue[index].begin());
    dataTimestamp[index].erase(dataTimestamp[index].begin());
    totalDataBufferBytes[index] -= returnValue;

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

    //Lock mutex to remove SDU from the head of the queue and decrease Byte count
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
    totalControlBufferBytes[index] -= returnValue;

    if(verbose) cout<<"[SduBuffers] Got SDU from control SDU queue."<<endl;

    return returnValue;
}

ssize_t 
SduBuffers::getNextDataSduSize(
    uint8_t macAddress)     //Destination MAC Address
{          
    int index = currentParameters->getIndex(macAddress);

    if(dataSduQueue[index].size()==0){
        if(verbose) cout<<"[SduBuffers] Tried to get size of empty SDU from L3."<<endl;
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
        if(verbose) cout<<"[SduBuffers] Tried to get size of empty SDU from Control SDU queue."<<endl;
        return -1;
    }

    //Get front values from the vectors
    return controlSizes[index].front();
}

void 
SduBuffers::dataSduTimeoutChecking(){
    unsigned long long stop;            //Mark current Subframe to calculate subframe difference
    unsigned long long diff;            //Subframe difference
    size_t numberSleepingSubframes = 0; //Number of subframes to sleep until packets can timeout; If there is an ampty queue, set to 1.
    size_t desiredSleepingTime;         //Desired sleeping time for each queue in number of subframes

    while(currentParameters->getMacMode()!=STOP_MODE){
        stop = timerSubframe->getSubframeNumber();    //Get current Subframe

        dataMutex.lock();   //Lock Data SDU Buffer mutex to possibly make alterarions on queue
        for(int index=0;index<currentParameters->getNumberUEs();index++){
            //Look for late IP packets
            while(1){
                //Check if there are Data SDUs to analyze
                if(!(dataSduQueue[index].size()>0)){
                    numberSleepingSubframes = 1;
                    break;
                }

                //Calculate Subframe difference (considering subframe counter can extrapolate maximum unsigned long long)
                if(dataTimestamp[index][0]>stop){   
                    diff = (18446744073709551615ULL - dataTimestamp[index][0]);
                    diff += stop;
                }
                else 
                    diff = stop - dataTimestamp[index][0];

                //Verify timeout
                if(diff<currentParameters->getIpTimeout()){
                    //Nothing to do: set desired sleeping time and wait
                    desiredSleepingTime = currentParameters->getIpTimeout()-diff;
                    if(numberSleepingSubframes==0) numberSleepingSubframes = desiredSleepingTime;
                    else if(desiredSleepingTime<numberSleepingSubframes) numberSleepingSubframes = desiredSleepingTime;
                    break;
                }
                else{
                    if(verbose) cout<<"[SduBuffers] IP Packet from queue "<<index<<" timed out with "<<diff<<" subframes as difference."<<endl;
                    
                    //Delete front positions and delete byte count
                    totalDataBufferBytes[index] -= dataSizes[index].front();
                    dataSizes[index].erase(dataSizes[index].begin());
                    dataSduQueue[index].erase(dataSduQueue[index].begin());
                    dataTimestamp[index].erase(dataTimestamp[index].begin());
                }
            }
        }
        //Unlock Mutex and sleep
        dataMutex.unlock();
        this_thread::sleep_for(chrono::microseconds((numberSleepingSubframes*SUBFRAME_DURATION)));
    }
}

void
SduBuffers::lockDataSduQueue(){
    dataMutex.lock();
}

void
SduBuffers::unlockDataSduQueue(){
    dataMutex.unlock();
}