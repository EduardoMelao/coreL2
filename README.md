# MAC-5G-RANGE

L2 layer for POC of 5G-RANGE project

## Compiling instructions (pre-requisite: CMake version 3.0.0)
Enter `build` directory, configure CMake and run `make` command:
```sh
$ cd build/
$ cmake ..
$ make
```

## Running instructions
To run L2 module, first you need to run PHY module and leave it waiting for MAC commands. To compile and run stub PHY module, follow these instructions:
```sh
$ cd build/src/coreL1/
$ sudo make
$ sudo ./coreL1.o <numberEquipments> <ipEquipmentN> <portEquipmentN> <macEquipmentN> [...] --v
```
where N indicates these three informations need to be provided for each equipment, without brackets. `--v` activates verbose mode.

With StubL1 set, MAC can be set up with
```
$ cd build/
$ sudo ./coreL2.o --v
```
where `--v` also activates verbose mode.


## Structure of Default information file. 
Just leave numbers in the file `build/Default.txt`, respecting the ranges indicated below:
 - BS/UE    [0..1]      Flag BS(1) or UE(0)
 - BS		[1..15]		Number of UEs 
 - BS/UE 	[0..5]		Numerology
 - BS/UE 	[0/1]		OFDM(0) or GFDM(1)
 - BS       [0..15]     Fusion LUT Matrix 4 bits [0/1] compressed into 1[0..15] Byte.
 - BS/UE	[1..10]	    Rx Metrics Periodicity in number of subframes
 - BS/UE	[0..1500]	MTU
 - BS/UE	[0..65536]	IP timeout
 - BS		[1..10]	    SS Report timeout
 - BS		[1..10]	    Ack timeout
 - BS		[1..33]	    Resource Block Group (RBG) size
 - BS/UE	[0..15]		*User Equipment Identification
 - BS/UE	[1..132]	*RBStart
 - BS/UE	[1..132]	*NumRBs
 - BS		[0..15]		*MCS Downlink
 - BS/UE	[0..15]		*MCS Uplink
 - BS/UE	[0/1]		*MIMO on(1) or off(0)
 - BS/UE	[0/1]		*MIMO diversity(0) or multiplexing(1)
 - BS/UE	[0/1]		*MIMO antennas 2x2(0) or 4x4(1)
 - BS/UE	[0/1]		*MIMO OL(0) or CL(1)
 - BS/UE	[0..15]		*MIMO precoding matrix index
 - BS/UE 	[0..40]	    *Transmission Power Control


`*` These informations are read Number of UEs times (for BS), these are Uplink reservations for each UE. 
