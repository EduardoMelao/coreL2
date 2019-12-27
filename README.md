# MAC-5G-RANGE

L2 layer for POC of 5G-RANGE project

Structure of Default information file. Just leave numbers in the file, respecting the ranges indicated below:


 - BS/UE	[0/1]		Flag to indicate if it is UE (0) or BS (1)
 - BS		[1..15]		Number of UEs 
 - BS		[1..132]	Number of RBs available for transmission
 - BS/UE	[0..15]		*User Equipment Identification
 - BS/UE	[1..132]	*RBStart
 - BS/UE	[1..132]	*NumRBs
 - BS/UE 	[0..5]		Numerology
 - BS/UE 	[0/1]		OFDM(0) or GFDM(1)
 - BS		[0..15]		MCS Downlink
 - BS/UE	[0..15]		MCS Uplink
 - BS/UE	[0/1]		MIMO on(1) or off(0)
 - BS/UE	[0/1]		MIMO diversity(0) or multiplexing(1)
 - BS/UE	[0/1]		MIMO antennas 2x2(0) or 4x4(1)
 - BS/UE	[0/1]		MIMO OL(0) or CL(1)
 - BS/UE	[0..15]		MIMO precoding matrix index
 - BS/UE 	[0..40]	    Transmission Power Control
 - BS		[0/1]		Fusion LUT Matrix values for all RBs
 - BS/UE	[1..10]	    Rx Metrics Periodicity in ms
 - BS/UE	[0..1500]	MTU
 - BS		[0..65536]	IP timeout
 - BS		[1..10]	    SS Report timeout
 - BS		[1..10]	    Ack timeout


`*` These three information are read Number of UEs times (for BS), these are Uplink reservations for each UE. 
