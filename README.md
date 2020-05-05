# MAC 5G-RANGE

L2 layer for POC of 5G-RANGE project

## 1. Prerequisites
 - Operational System: Ubuntu 18.04 LTS
 - Ubuntu packages: `$ sudo apt-get install libboost-all-dev cmake`
 - On BS, execute: `$ sudo sysctl net.ipv4.ip_forward = 1` to turn on IP forwarding

## 2. Stub of PHY layer
StubPHY was built to support tests during MAC development. Its communication is based on UDP sockets between different machines under the same subnetwork, that is, the physical link is replaced by an Ethernet link with IPv4 packets.

Its basic functions are:
- Receiving and decoding Interlayer Messages from MAC;
- Receiving Subframes containing PDUs from MAC;
- Forwarding these Subframes to 5G-RANGE equipments through UDP sockets;
- Sending Interlayer Messages to MAC when necessary;
- Dropping Subframes received if destination does not match;
- Forwarding Subframes received to MAC otherwise.

### 2.1. StubPHY code
StubPHY code is located under directory `build/src/coreL1` and is composed by 3 simple files containing `main` funcion at `CoreL1.cpp` and a Class `CoreL1` declared and implemented in `StubPHYLayer.h` and `StubPHYLayer.cpp`. Also, StubPHY uses libraries from `common` repository: `lib5grange.h` and `libMac5gRange.h`.

### 2.2. StubPHY compilation
To compile StubPHY, this sequence of commands must be run:
```bash
$ cd build/src/coreL1
$ sudo make
```

### 2.3. StubPHY use
To initialize StubPHY module, a command like the following must be typed on BS or on UE. Note that the three values with " * " can be repeated in groups for each UE attached and commands between " < > " are optional:
```bash
$ sudo ./coreL1.o currentUeId numberEquipments *ipN *portN *ueIdN <--v>
```
where:
* `currentUeId`: UeID of current equipment. If it is BS, currentUeId is 0;
* `numberEquipments`: number of equipments attached. If it is BS, this is the number of UEs attached. If it is UE, this number must be 1 (only BS is "attached" to UE);
* `ipN`: Ethernet IP address of n-th UE attached;
* `portN`: Port to be used in UDP socket connection at the n-th UE;
* `ueIdN`: UeID of n-th UE attached;
* `--v`: Set optional verbose to active state;

For example, suppose a BS (192.168.0.10) with 2 UES attached UE1 (192.168.0.11) and UE2 (192.168.0.12) and these UEs are going to connect to ports 8096 and 8097 of BS, respectively. So, these commands must be typed to run PHY on 3 equipments:

On BS:
```bash
$ sudo ./coreL1.o 0 2 192.168.0.11 8096 1 192.168.0.12 8097 2 --v
```

On UE1:

```bash
sudo ./coreL1.o 1 1 192.168.0.10 8096 0 --v
```

On UE2:

```bash
sudo ./coreL1.o 2 1 192.168.0.10 8097 0 --v
```

Right after running PHY, it will wait for `PHYConfig.Request` Interlayer Message from MAC. Then, StubPHY will send `PHYTx.Indication` messages every 4.6ms time interval to ask for subframes for transmission.

## 3. MAC 5G-RANGE
### 3.1 MAC Code
MAC code can be located under `build/src/coreL2` directory, with modules oganized in subdirectories.

### 3.2 MAC Compilation
To compile MAC, the following commands must be done:
```sh
$ cd build/
$ cmake ..
$ make
```

### 3.3 MAC use
Before using MAC, a default file must be filled with static/default information for MAC to initialize its modules. This default file is located at `build/Default.txt` and follows rules of construction. After filling this file, MAC can be executed and configured.

#### 3.3.1 Structure of Default information file
---
**NOTE**

This file must be editted before running MAC. The default configuration (this version on GitHub) only works for BS.

---
The file `Default.txt` is composed only by numbers, respecting ranges, as defined in the table below. The first column indicates if the attribute os necessary for BS and UE (BS/UE) or only for BS. The second column shows the range of the number to be put in the file. Finally, the third column presents the name and/or a short explanation about the parameter.
| BS/UE or BS  | Range | Parameter  |
|:-:|:-:|---|
|BS/UE|[0..1]|Flag BS(1) or UE(0)
|BS		|[1..15]	|	Number of UEs 
|BS/UE 	|[0..5]	|	Numerology
|BS/UE 	|[0/1]		|OFDM(0) or GFDM(1)
|BS     |  [0..15] |    Fusion LUT Matrix 4 bits [0/1] compressed into 1[0..15] Byte.
|BS/UE	|[1..10]	    |Rx Metrics Periodicity in number of subframes
|BS/UE	|[0..1500]|	MTU
|BS/UE|	[0..65536]	|IP timeout
|BS		|[1..10]	    |SS Report timeout
|BS		|[1..10]	    |Ack timeout
|BS		|[1..33]	    |Resource Block Group (RBG) size
|BS/UE	|[0..15]		|*User Equipment Identification
|BS/UE	|[1..132]	|*RBStart
|BS/UE	|[1..132]	|*NumRBs
|BS		|[0..15]		|*MCS Downlink
|BS/UE	|[0..15]	|	*MCS Uplink
|BS/UE	|[0/1]		|*MIMO on(1) or off(0)
|BS/UE	|[0/1]		|*MIMO diversity(0) or multiplexing(1)
|BS/UE	|[0/1]		|*MIMO antennas 2x2(0) or 4x4(1)
|BS/UE	|[0/1]		|*MIMO OL(0) or CL(1)
|BS/UE	|[0..15]	|	*MIMO precoding matrix index
|BS/UE| 	[0..40]	 |   *Transmission Power Control
 
`*` These informations are read Number of UEs times (for BS), these are Uplink reservations for each UE. 

#### 3.3.2 Execution of MAC
After filling `Default.txt` and leaving it into `build/` directory, MAC can be executed using the comand below (parameter with `< >` is optional; do not use `<` or `>`).
```sh
$ cd build/
$ sudo ./coreL2.o <--v>
```

#### 3.3.3 Stub of Command Line Interface (CLI)
The code does not have a Command Line Interface for now, as it is being developed by USP. On the other hand, three simple commands were implemented to simulate the 3 basic CLI orders. The three comands are shown right after executing MAC:
```
Press + for MacStart, / for MacStop and * for MacConfigRequest
```
- `+` MacStart command initializes the system;
- `/` MacStop stops execution of system;
- `*` MacConfigRequest simulates an alteration on System parameters during program execution.


#### 3.3.4 Linux System configuration
After commanding the system to start (`MacStartCommand`), the IPv4 address of MAC 5G-RANGE interface must be configured. While MAC is running, it can be observed that a new network interface is created: `tun0`. This interface must have IPv4 configured as one of the below:
| UE ID | IPv4 address of `tun0` interface|
|:-:    |:-:          |
|0*|10.0.0.10|
|1 |10.0.0.11|
|2 |10.0.0.12|
|3 |10.0.0.13|

*: UE ID 0 is reserved for BS.

For now, the system does not support more than 3 UEs due this hard code configuration.

For example, to configure TUN IP address in BS, the command below has to be run in the terminal.
```sh
sudo ifconfig tun0 10.0.0.10/24
```

After configuring both BS and UE, when a packet addresses to an IP address of `10.0.0.0` network is created on one of the systems, this packet will be forwarded to TUN interface by default. Then, this packet will be treated by MAC and sent to the other side.


