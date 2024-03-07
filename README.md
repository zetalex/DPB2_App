




<h3 align="center">Trabajo Hyper-Kamiokande</h3>






<h1 align="center"><b>Descripción detallada del trabajo realizado</b></h1>


<h4 align="center"><b>Borja Martínez Sánchez</b></br></h4>


<h3 align="center">Grado en Ingeniería de Tecnologías y Servicios de Telecomunicación</h3>

# Introduction

## The neutrino itself
It is well known that telecommunications has played a pivotal role in the course of society since its emergence, being a discipline that today is necessary in almost any field, whether day-to-day or professional.

Telecommunications encompasses several branches of knowledge, in which I specialise in the field of electronic systems.
That is why this Final Project of the Degree in Telecommunication Technologies and Services Engineering is a clear evidence of the importance of electronic systems in the advancement of human beings to explore and investigate the behaviour of the universe.

Within the field of physics there are countless subfields that study different aspects of everything around us. The project on which this dissertation is based is based on the speciality of physics called *Particle physics*, which is also known as high-energy physics, because many of these particles can only be seen in large collisions provoked in particle accelerators. This discipline of physics is responsible for demonstrating the existence of particles classified according to certain characteristics as bosons or fermions. Nonetheless, it also encounters the difficulty of having been able to demonstrate particles that are almost non-detectable to this day.

Within these elusive particles lies the neutrino, a subatomic entity generated during a radioactive decay and scattering phenomenon. In this instance, the neutrino arises from beta decay, as proposed in Fermi's theory, wherein a sizeable neutral particle (n) disintegrates into a proton (p<sup>+</sup>), an electron (e<sup>-</sup>), and a neutrino (ν<sub>e</sub>).

$$
n -> p⁺ + e⁻ + v_e
$$(1.1)

The first person to postulate the existence of the neutrino theoretically was Wolfgang Pauli in 1930, but it remained undetected for 25 years because this hypothetically predicted particle had to be massless, chargeless and without strong interaction.

Finally, in 1956,  Clyde Cowan, Frederick Reines, Francis B. "Kiko" Harrison, Herald W. Kruse, and Austin D. McGuire were able to demonstrate the existence of the neutrino experimentally by using a beam of neutrons to pump a tank of pure water. By observing the subsequent emission of the protons, they were able to demonstrate the existence of the neutrino. This test was called the neutrino experiment.

Over the years, different types of radioactive decays have been discovered that can give rise to neutrinos, such as natural and artificial nuclear reactions, supernova events or the spin-down of a neutron star. Furthermore, it has been discovered that there are different leptonic flavours of neutrinos originating from the weak interactions, electron neutrino, muon neutrino and tau neutrino,each flavor is associated with the correspondingly named charged lepton and similar to some other neutral particles, neutrinos oscillate between different flavors in flight as a consequence.



## Hyper-Kamiokande Project
Hyper-Kamiokande is a neutrino observatory project still under construction (estimated to start operation in 2027), which takes place in the Kamioka mines in Japan. Although the project is based in Japan, it involves research institutes from 22 different countries. The aim of the project is to search for proton decays and detect neutrinos from natural sources such as the Earth, the atmosphere, the Sun and the cosmos, as well as to study neutrino oscillations from the neutrino beam of the artificial accelerator.

Hyper-Kamiokande is planned to be the world's largest neutrino observatory, surpassing its predecessor Super-Kamiokande, which is 71 metres high and 68 metres in diameter. The observatory, filled with ultrapure water, will have about 40,000 photomultiplier tubes as detectors inside the observatory and 10,000 detectors outside the observatory.



![Interior of the Super-Kamiokande, predecessor of the Hyper-Kamiokande](/DBP2_App/doc/figures/SK-Detector.png)
<figcaption>Interior of the Super-Kamiokande, predecessor of the Hyper-Kamiokande</figcaption>.
<br>

![Structure of the Hyper-Kamiokande](/DBP2_App/doc/figures/HK-SEC.png)
<figcaption>Structure of the Hyper-Kamiokande</figcaption>.
<br>

The PMTs, along with the rest of the electronics, will be housed in hermetically sealed vessels submerged in the water inside the observatory.

![Interior of the vessel](/DBP2_App/doc/figures/Vasija.jpg)
<figcaption>Interior of the vessel</figcaption>
<br>

![Diagram of communication between the different modules of the vessel](/DBP2_App/doc/diag/Bloques_Vasija.jpg)
<figcaption>Diagram of communication between the different modules of the vessel</figcaption>.
<br> 

As can be seen in the previous figures, the electronics are concentrated inside the vessel, where the information from the PMTs passes through digitisers to the DPB. The DPB is responsible for communicating the different modules both outside and inside the vessel, it acts as a hub inside the vessel.

Since the electronics are located in a place that is difficult to access, as it would mean emptying the observatory of water, high reliability is required in this project, at least 10 years. For this reason, robust systems have been chosen and the electronics used must be monitored.

In our group we are dedicated to the DPB module and specifically my work is dedicated to the reliability of the DPB, as it consists of developing an application that uses all the sensing and measurement subsystems available in the DPB to monitor its status and establish alarms and warnings for critical cases.


# Technology with which is going to be used

Due to the amount of data we intend to work with and the need to customise our board for our application, we have chosen to use a SoM with the AMD Zynq UltraScale+ MPSoC as the processing system, which is combined in the SoM together with logic devices which form a user-programmable logic. The MPSoC includes various controllers such as UART, I2C or eMMC ports which provide communication with the peripherals and integrates a monitoring system for the chip itself and its subsystems. In addition, the Zynq UltraScale+ has support for lightweight operating systems, which can be a great benefit if you take advantage of the functionality of the operating system's own drivers.

The SoM will be integrated on a board designed exclusively for our project with the necessary peripherals. With this SoM implementation, we will enjoy great flexibility and customisation in our design without sacrificing the processing power of a high-performance chip such as the one offered by AMD.

# Initialisation of the environment to be used on the board
Starting with the environment to work on the DPB (Data Processing Board) or DPM (Data Processing Module), we will use PetaLinux, a Xilinx software development tool based on a light version of Linux.

The universal availability of the Linux source code and the infinite number of drivers available in Linux gives us greater flexibility and ease of working at the application level on the DPB.

To implement this operating system (OS) on the DPB we have used the Xilinx software, Vivado, and through the JTAG port we have loaded on a 16 GB eMMC as non-volatile memory, both the relevant boot files and the custom image of the PetaLinux project, then we have selected, from the board, the eMMC as the main boot option. In the boot process, the OS is loaded onto the RAM and the RAM is worked on.

Once the OS has been implemented, the connection with the DPB has to be configured. Despite the possibility of maintaining the connection via JTAG, the main source of communication of the DPB is going to be via Ethernet, so one of the SFP ports of the DPB has been used to make an Ethernet connection with the equipment by means of an SFP transceiver. For this purpose, the configuration of a 125 MHz PLL for the corresponding Ethernet clock signal was included in the customisation of the PetaLinux version.


Once the connection has been configured, a local DHCP server has been set up to assign a fixed IP to the DPB and facilitate the connection via SSH to the board. For this purpose, the subnet has been declared with a very basic configuration on the server:

```bash
subnet 20.0.0.0 netmask 255.255.255.0 {
  range 20.0.0.2 20.0.0.30;
  option routers 20.0.0.1;
}
```
The network interface in question has been assigned the address 20.0.0.1 and the subnet has been declared with a small arbitrary range, and the DBP has been assigned the fixed IP 20.0.0.33, an address outside the range, since otherwise the server would return an error. It should be noted that the SFP ports of the DBP are designed to use fibre optic ports, so sometimes the equipment is not able to detect the connection on the Ethernet port using Ethernet cable with RJ-45, so the interface has to be deactivated and then re-activated and assigned the address 20.0.0.1 and the problem is solved, in the case of using a fibre optic port, this problem does not arise.

With the fixed IP address already assigned, it is now possible to access the board via SSH and communicate with it using the following command:
```bash
ssh root@20.0.0.33
#Here we would enter the relevant password
```
To finish with the establishment of the working environment, we only have to create the application project that is going to be developed on a customised platform of our project, in the Vitis IDE software of Xilinx. The application project has been named DBP2_App.

# I<sup>2</sup>C protocol and how it is implemented on our board
To achieve communication between the different components on the board and the terminal, the I<sup>2</sup>C protocol is used, a communication protocol based on a Master-Slave system where the communication bus is divided into 2 lines, SCL for the clock and SDA for the data, which are connected to a pull-up resistor each, so the default level is high level.

The operation of this protocol consists of the start of the transmission by the Master which jointly indicates the address of the slave to which it is directed with an address of 7 bits (we have sensors that have an address of 6 bits plus a reserved bit that we use to differentiate in a physical way), in addition it is indicated with a bit if the operation to be carried out is reading or writing. The data transmission is guided by the clock line and the data is transmitted in byte size, transmitting from MSB to LSB.

![I<sup>2</sup>C Address and Data Frames](/DBP2_App/doc/figures/I2C_ADD_DAT_FRAME.png)
<figcaption>Addressing and data frames I<sup>2</sup>C</figcaption>.
<br>

For the write operation on the slave, once communication has been established, the register to be written to and the data to be written must be indicated. The master is responsible for receiving the corresponding ACK and NACK during communication and the end of communication sequence.

The read operation follows a similar process to the write operation, indicating the register to be read and the master is in charge of sending the corresponding ACKs and NACKs during the communication and the end of communication sequence.

In our case the communication process will be based on the functions provided by the Linux libraries that allow us to open/close the communication and read/write registers simply by calling defined functions and indicating the necessary arguments. In addition, these functions allow us to operate with vectors in order to read or write consecutive data with a single function.

![I<sup>2</sup>C Address and Data Frames](/DBP2_App/doc/diag/DBLOQUES_I2C.png)
<figcaption>Structure of the I<sup>2</sup>C of our DPB</figcaption>.
<br> 

In the previous block diagram you can see how the I<sup>2</sup>C buses of our DPB are structured, the corresponding filename of each of the I<sup>2</sup>C bus outputs designated by the multiplexers and the slave addresses of each module with which we intend to communicate.

As can be seen, the current sensors, the SFP connectors and the temperature sensor that we intend to use all use the I<sup>2</sup>C protocol to communicate. However, the temperature sensor and the current sensors use 16-bit registers, while the SFPs use 1-byte sized registers. The I<sup>2</sup>C protocol carries byte-sized frames, so in the case of 16-bit registers it involves performing 2 consecutive operations (either read or write) on the same register address, whereas for 8-bit registers it will involve a single operation per register address. In our case, the Linux I<sup>2</sup>C driver will make it much easier to perform this type of consecutive operations.

# Detailed information about the available sensors and their usefulness for our interests.

Regarding the sensor units available in our DPB we find, as previously mentioned in the I<sup>2</sup>C section, a temperature sensor (MCP9844), several current sensors and voltage monitoring (INA3221) for the SFP transceivers and the SoM and the SFP transceivers themselves that provide us with very relevant information about their operating status and that we should keep track of.

## Current sensors INA3221

The current sensors installed in the DPB provide us with the possibility of monitoring up to 3 different channels from the same sensor. In addition, it allows us to measure the bus voltage with respect to GND(*Bus Voltage*) or the voltage difference between the IN+ and IN- terminals of each channel(*Shunt Voltage*). In our case, a resistive element with a value of 0.05 $Omega$ has been placed between IN+ and IN-, which is useful for obtaining both the current and the power consumed in each channel.

It should be noted that this sensor allows us to configure alerts and warnings for voltage values obtained in *Shunt Voltage* measurement mode to detect if the voltage difference between terminals of our resistor exceeds or does not reach certain values and to be able to act accordingly. We also have an alert if in *Bus Voltage* measurement mode, which informs us if all the channels being measured have a voltage higher than that marked by the limits or if any of the channels has a voltage lower than the lower limit. We are also provided with the option to obtain the sum of the *Shunt Voltage* of all channels and set a limit to configure an alert. All named alerts and warnings are collected in the *Mask/Enable* register where the sum of the *Shunt Voltage*, warnings and alarms can also be enabled or disabled.


In the following table you can see the most influential registers for our application, a short description of these registers, their default value and the type of register it is, whether it is read-only or read-write.

<!---
Current.Sens
-->


| POINTER ADDRESS (Hex) | REGISTER NAME               | DESCRIPTION                                                                                           | BINARY (Power-On Reset)          | HEX (Power-On Reset)    | TYPE | 
|-----------------------|-----------------------------|-------------------------------------------------------------------------------------------------------|-------------------|---------|---------| 
| 0                     | Configuration               | All-register reset, shunt and bus voltage ADC conversion times and operating mode.                    | 01110001 00100111 | 7127    | R/W     | 
| 1                     | Channel-1 Shunt Voltage    | Averaged shunt voltage value.                                                                        | 00000000 00000000 | 0000    | R       | 
| 2                     | Channel-1 Bus Voltage      | Averaged bus voltage value.                                                                          | 00000000 00000000 | 0000    | R       | 
| 3                     | Channel-2 Shunt Voltage    | Averaged shunt voltage value.                                                                        | 00000000 00000000 | 0000    | R       | 
| 4                     | Channel-2 Bus Voltage      | Averaged bus voltage value.                                                                          | 00000000 00000000 | 0000    | R       | 
| 5                     | Channel-3 Shunt Voltage    | Averaged shunt voltage value.                                                                        | 00000000 00000000 | 0000    | R       | 
| 6                     | Channel-3 Bus Voltage      | Averaged bus voltage value.                                                                          | 00000000 00000000 | 0000    | R       | 
| 7                     | Channel-1 Critical Alert   |  Contains limit value to compare each conversion value to determine if the corresponding limit has been exceeded.| 01111111 11111000 | 7FF8    | R/W     | 
| 8                     | Channel-1 Warning Alert    |  Contains limit value to compare to averaged measurement to determine if the corresponding limit has been exceeded.| 01111111 11111000 | 7FF8    | R/W     | 
| 9                     | Channel-2 Critical Alert   |  Contains limit value to compare each conversion value to determine if the corresponding limit has been exceeded.| 01111111 11111000 | 7FF8    | R/W     | 
| A                     | Channel-2 Warning Alert    |  Contains limit value to compare to averaged measurement to determine if the corresponding limit has been exceeded.| 01111111 11111000 | 7FF8    | R/W     | 
| B                     | Channel-3 Critical Alert   |  Contains limit value to compare each conversion value to determine if the corresponding limit has been exceeded.| 01111111 11111000 | 7FF8    | R/W     | 
| C                     | Channel-3 Warning Alert    |  Contains limit value to compare to averaged measurement to determine if the corresponding limit has been exceeded.| 01111111 11111000 | 7FF8    | R/W     | 
| D                     | Shunt-Voltage Sum          | Contains the summed value of the each of the selected shunt voltage conversions.                       | 00000000 00000000 | 0000    | R       | 
| E                     | Shunt-Voltage Sum Limit    | Contains limit value to compare to the Shunt Voltage Sum register to determine if the corresponding limit has been exceeded.| 01111111 11111110 | 7FFE    | R/W     | 
| F                     | Mask/Enable                | Alert configuration, alert status indication, summation control and status.                           | 00000000 00000010 | 0002    | R/W     | 
| 10                    | Power-Valid Upper Limit    | Contains limit value to compare all bus voltage conversions to determine if the Power Valid level has been reached.| 00100111 00010000 | 2710    | R/W     | 
| 11                    | Power-Valid Lower Limit    | Contains limit value to compare all bus voltage conversions to determine if the any voltage rail has dropped below the Power Valid range.| 00100011 00101000 | 2328    | R/W     | 
| FE                    | Manufacturer ID            | Contains unique manufacturer identification number.                                                    | 01010100 01001001 | 5449    | R       | 
| FF                    | Die ID                      | Contains unique die identification number.                                                             | 00110010 00100000 | 3220    | R       |
<figcaption>INA3221 Current Sensor Registers</figcaption>
<br>

It is worth mentioning that all voltage data is given in 2's complement and uses 13 bits, bit 15 of the register (MSB) determines the sign and bit 14-3 the voltage data.
For *Shunt Voltage* the full scale range is 163.8 mV and the LSB is 40 $mu$V, in the case of *Bus Voltage* the LSB is 8 mV and although the full scale range of the ADC is 32.76 V, the full scale range in the case of *Bus Voltage* is 26 V since it is not recommended to apply more voltage.

## Temperature sensor MCP9844
<!---
Registers Temp.Sens
-->
The temperature sensor MCP9844 is a great tool to monitor the temperature of the environment where our DPB works, an essential magnitude to ensure a correct conditioning for the operation of our electronics.

This temperature sensor provides us with the events tool that facilitates the monitoring of the ambient temperature. The MCP9844 allows us to set temperature limits, only modifiable if enabled in the configuration register, both upper and lower and even critical temperature (only higher than the upper limit). Once the limits have been established, from the configuration register you can enable or disable the events and you can configure the event as an interruption or as a comparison, decide whether the event is active at high or low level and decide whether only the critical temperature limit is taken into account or all the limits are taken into account.

In addition, the sensor has several functionalities such as the option to include a certain hysteresis value to the temperature limits (only applicable in case of temperature drop), the possibility to modify the measurement resolution (lower resolution value will imply a longer conversion time) or the possibility to switch off the sensor if desired.

Below is a table of the registers presented by this temperature sensor and its default value.

| Register Address (Hexadecimal) | Register Name         | Default Register Data (Hexadecimal) | Power-Up Default Register Description                                                                                           |
|-----------------------|-----------------------|--------------------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| 0x00                  | Capability            | 0x00EF   | Event output deasserts in shutdown I<sup>2</sup>C™ time out 25 ms to 35 ms. Accepts VHV at A0 Pin 0.25°C Resolution . Measures temperature below 0°C ±1°C accuracy over active range Temperature event output |
| 0x01                  | CONFIG                | 0x0000                               | Comparator mode Active-Low output Event and critical output Output disabled Event not asserted Interrupt cleared Event limits unlocked Critical limit unlocked Continuous conversion 0°C Hysteresis |
| 0x02                  | T<sub>UPPER</sub>                 | 0x0000                               | 0°C                                                                                                                              |
| 0x03                  | T<sub>LOWER</sub>                | 0x0000                               | 0°C                                                                                                                              |
| 0x04                  | T<sub>CRIT</sub>                  | 0x0000                               | 0°C                                                                                                                              |
| 0x05                  | T<sub>A</sub>                     | 0x0000                               | 0°C                                                                                                                              |
| 0x06                  | Manufacturer ID       | 0x0054                               | —                                                                                                                       |
| 0x07                  | Microchip Device ID/ Device Revision  | 0x0601                         | —                                                                                                                                       
| 0x09                  | Resolution  | 0x8001                               | Most Significant bit is set by default 0.25°C Measurement Resolution                                                            |

<figcaption>MCP9844 Temperature Sensor Registers</figcaption>
<br>

In this case the temperature data is encoded in 2's complement and is presented as a 13-bit data, 1 bit determining the sign and 12 determining the temperature data, so the manufacturer provides us with the following equations to obtain the data in degrees Celsius.


If Temperature $\ge$ 0°C
$$
T_{A}(ºC) = (UpperByte * 2^{4} + LowerByte* 2^{4}) 
$$(2.2.1)

If Temperature < 0°C
$$
T_{A}(ºC) = (UpperByte * 2^{4} + LowerByte* 2^{4})-256
$$(2.2.2)

Where *UpperByte* are bits 15-8 of the T<sub>A</sub> register and *LowerByte* are bits 7-0 of the same register.

In the case of the temperature limits these are defined by 11 bits, 1 bit determining the sign and 10 bits to encode the absolute temperature data.

## SFP Transceiver AFBR-5715ALZ

Los transceptores SFP de fibra óptica tienen la función principal ser los puertos de comunicación de la placa. These transceivers have an EEPROM memory that is divided into two pages, which correspond to the slave addresses I<sup>2</sup>C 0x50 and 0x51 in our case.

The SFPs collect information on highly relevant real-time quantities and are located on the second page of the EEPROM (0x51), such as the temperature at which they are located, the supply voltage supplied to them, the laser bias current and both the transmitted and received optical power. 

On the same second page of the SFP EEPROM is the possibility to use alerts and warnings based on a range already determined by the manufacturer to monitor the status of the SFP transceivers.

Although the first page of the EEPROM is mainly based on transceiver identification characters such as part number and revision or vendor name, we can also find relevant information about the status and operation of the transceiver as we can find in this memory space the wavelength of the laser to know in which window it is working and the register that tells us if the status signals TX_DISABLE, TX_FAULT and RX_LOS have been configured by hardware. 

In both pages of the EEPROM we find one or more registers dedicated to a Checksum that will allow us to check the status of the EEPROM itself.

Below are several tables representing the EEPROM registers of the SFP transceivers.
<!---
Registers SFP 0x50
-->

| Byte Decimal | Data Notes |
|--------------|------------|
| 0 | SFP physical device |
| 1 | SFP function defined by serial ID only |
| 2 | LC optical connector |
| 6 | 1000BaseSX |
| 11 | Compatible with 8B/10B encoded data |
| 12 | 1200Mbps nominal bit rate (1.25Gbps) |
| 16 | 550m of 50/125mm fiber @ 1.25Gbps |
| 17 | 275m of 62.5/125mm fiber @ 1.25Gbps |
| 20-35 | 'AVAGO' - Vendor Name ASCII character |
| 37 | Vendor OUI |
| 38 | Vendor OUI |
| 39 | Vendor OUI |
| 40-55 | 'AFBR-5715ALZ' - Vendor Part Number ASCII characters |
| 56-59 | Vendor Revision Number ASCII character |
| 60 | Hex Byte of Laser Wavelength |
| 61 | Hex Byte of Laser Wavelength |
| 63 | Checksum for bytes 0-62 |
| 65 | Hardware SFP TX_DISABLE, TX_FAULT, & RX_LOS |
| 68-83 | Vendor Serial Number, ASCII |
| 84-91 | Vendor Date Code, ASCII |
| 95 | Checksum for bytes 64-94 |

<figcaption>SFP transceiver EEPROM page 1 registers</figcaption>
<br>

<!---
Registers SFP 0x51
-->
| Byte Decimal | Notes                    | Byte Decimal | Notes                      | Byte Decimal | Notes                           |
|--------------|--------------------------|--------------|----------------------------|--------------|---------------------------------|
| 0            | Temp H Alarm MSB        | 26           | Tx Pwr L Alarm MSB        | 104          | Real Time Rx P<sub>AV</sub> MSB          |
| 1            | Temp H Alarm LSB        | 27           | Tx Pwr L Alarm LSB        | 105          | Real Time Rx P<sub>AV</sub> LSB          |
| 2            | Temp L Alarm MSB        | 28           | Tx Pwr H Warning MSB      | 106          |                                |
| 3            | Temp L Alarm LSB        | 29           | Tx Pwr H Warning LSB      | 107          |                                |
| 4            | Temp H Warning MSB      | 30           | Tx Pwr L Warning MSB      | 108          |                                |
| 5            | Temp H Warning LSB      | 31           | Tx Pwr L Warning LSB      | 109          |                                |
| 6            | Temp L Warning MSB      | 32           | Rx Pwr H Alarm MSB        | 110          | Status/Control                  |
| 7            | Temp L Warning LSB      | 33           | Rx Pwr H Alarm LSB        | 111          |                                |
| 8            | VCC H Alarm MSB         | 34           | Rx Pwr L Alarm MSB        | 112          | Flag Bits                       |
| 9            | VCC H Alarm LSB         | 35           | Rx Pwr L Alarm LSB        | 113          | Flag Bit                        |
| 10           | VCC L Alarm MSB         | 36           | Rx Pwr H Warning MSB      | 114          |                                |
| 11           | VCC L Alarm LSB         | 37           | Rx Pwr H Warning LSB      | 115          |                                |
| 12           | VCC H Warning MSB       | 38           | Rx Pwr L Warning MSB      | 116          | Flag Bits                               |
| 13           | VCC H Warning LSB       | 39           | Rx Pwr L Warning LSB      | 117          | Flag Bits                               |
| 16           | Tx Bias H Alarm MSB     | 95           | Checksum for Bytes 0-94    | 120          |                                |
| 17           | Tx Bias H Alarm LSB     | 96           | Real Time Temperature MSB | 121          |                                |
| 18           | Tx Bias L Alarm MSB    | 97           | Real Time Temperature LSB | 122          |                                |
| 19           | Tx Bias L Alarm LSB     | 98           | Real Time Vcc MSB         | 123          |                                |
| 20           | Tx Bias H Warning MSB   | 99           | Real Time Vcc LSB         | 124          |                                |
| 21           | Tx Bias H Warning LSB   | 100          | Real Time Tx Bias MSB     | 125          |                                |
| 22           | Tx Bias L Warning MSB   | 101          | Real Time Tx Bias LSB     | 126          |                                |
| 23           | Tx Bias L Warning LSB   | 102          | Real Time Tx Power MSB    | 127          |                                |
| 24           | Tx Pwr H Alarm MSB      | 103          | Real Time Tx Power LSB    | 128          |                                |
| 25           | Tx Pwr H Alarm LSB      |              |                            |              |                                |
<figcaption>SFP transceiver EEPROM page 2 registers</figcaption>
<br>


To interpret the data of the magnitudes on page 2 in bit format, the following clarifications from the manufacturer must be taken into account depending on the magnitude to be interpreted:

 - **Temperature (Temp):** Temperature values are encoded as 16-bit integers in two's complement, which allows both positive and negative values to be represented. Each unit in this representation is equivalent to 1/256 of a degree Celsius (ºC).
  
- **Power Supply Voltage (VCC):** This parameter is represented as a 16-bit unsigned integer, which means that it can only have positive values. Each increment in this value corresponds to 100 microvolts (µV).
  
- **Laser Bias Current (Tx Bias):** The laser bias current is decoded as a 16-bit unsigned integer, which means that it can only be positive. Each increment in this value represents 2 microamperes (µA).
  
- **Average Transmitted Optical Power (Tx Pwr):** This parameter is represented as a 16-bit unsigned integer, where each increment corresponds to 0.1 microwatt (µW) of transmitted optical power.
  
- **Average Optical Power Received (Rx Pwr):** Similar to the previous parameter, the average optical power received is encoded as a 16-bit unsigned integer. Each unit of this value represents 0.1 microwatt (µW) of received optical power.

As can be seen in the register table on the second page of the EEPROM, there is a status register and this describes the following cases.

<!---
SFP Status Table
-->
| Bit # | Status/Control Name | Description |
|-------|----------------------|-------------|
| 7     | Tx Disable State     | Digital state of SFP Tx Disable Input Pin (1 = Tx_Disable asserted) |
| 6     | Soft Tx Disable      | Read/write bit for changing digital state of SFP Tx_Disable function |
| 4     | Rx Rate Select State | Digital state of SFP Rate Select Input Pin (1 = full bandwidth of 155 Mbit) |
| 2     | Tx Fault State       | Digital state of the SFP Tx Fault Output Pin (1 = Tx Fault asserted) |
| 1     | Rx LOS State         | Digital state of the SFP LOS Output Pin (1 = LOS asserted) |
| 0     | Data Ready (Bar)     | Indicates transceiver is powered and real-time sense data is ready (0 = Ready) |

<figcaption>Breakdown of SFP transceiver status bits</figcaption>
<br>
<!---
SFP Flags Table
-->
As for the registers dedicated to the <i>flags</i>, these contain the indicator bits of the previously mentioned alerts and warnings. The following table shows their distribution in the relevant registers.

<br>

| Byte | Bit # | Flag Bit Name | Description |
|------|-------|---------------|-------------|
| 112  | 7     | Temp High Alarm | Set when transceiver internal temperature exceeds high alarm threshold. |
|      | 6     | Temp Low Alarm  | Set when transceiver internal temperature exceeds low alarm threshold. |
|      | 5     | VCC High Alarm  | Set when transceiver internal supply voltage exceeds high alarm threshold. |
|      | 4     | VCC Low Alarm   | Set when transceiver internal supply voltage exceeds low alarm threshold. |
|      | 3     | Tx Bias High Alarm | Set when transceiver laser bias current exceeds high alarm threshold. |
|      | 2     | Tx Bias Low Alarm | Set when transceiver laser bias current exceeds low alarm threshold. |
|      | 1     | Tx Power High Alarm | Set when transmitted average optical power exceeds high alarm threshold. |
|      | 0     | Tx Power Low Alarm | Set when transmitted average optical power exceeds low alarm threshold. |
|------|-------|---------------|-------------|
| 113  | 7     | Rx Power High Alarm | Set when received P_Avg optical power exceeds high alarm threshold. |
|      | 6     | Rx Power Low Alarm | Set when received P_Avg optical power exceeds low alarm threshold. |
|------|-------|---------------|-------------|
| 116  | 7     | Temp High Warning | Set when transceiver internal temperature exceeds high warning threshold. |
|      | 6     | Temp Low Warning | Set when transceiver internal temperature exceeds low warning threshold. |
|      | 5     | VCC High Warning | Set when transceiver internal supply voltage exceeds high warning threshold. |
|      | 4     | VCC Low Warning | Set when transceiver internal supply voltage exceeds low warning threshold. |
|      | 3     | Tx Bias High Warning | Set when transceiver laser bias current exceeds high warning threshold. |
|      | 2     | Tx Bias Low Warning | Set when transceiver laser bias current exceeds low warning threshold. |
|      | 1     | Tx Power High Warning | Set when transmitted average optical power exceeds high warning threshold. |
|      | 0     | Tx Power Low Warning | Set when transmitted average optical power exceeds low warning threshold. |
|------|-------|---------------|-------------|
| 117  | 7     | Rx Power High Warning | Set when received P_Avg optical power exceeds high warning threshold. |
|      | 9     | Rx Power Low Warning | Set when received P_Avg optical power exceeds low warning threshold. |

<figcaption>Breakdown of the <i>flags</i> of SFP transceivers</figcaption>
<br>


 
# Obtención de datos del AMS, PS y PL SYSMON y diferenciación por canales

Due to the sensors together with ADC converters with which Xilinx has equipped our module and its system monitoring tools (SYSMON), we can access a large amount of real-time information from the AMS, the PS and the PL via the linux driver "xilinx-ams". 

This information is differentiated into different channels which are explained in the following table.

| Sysmon Block | Channel | Details                                                     | Measurement | File Descriptor                    |
|--------------|---------|-------------------------------------------------------------|-------------|-----------------------------|
| AMS CTRL     | 0       | System PLLs voltage measurement, VCC_PSPLL.                 | Voltage     | *in_voltage0_raw, in_voltage0_scale*        |
|              | 1       | Battery voltage measurement, VCC_PSBATT.                    | Voltage     | *in_voltage1_raw, in_voltage1_scale*        |
|              | 2       | PL Internal voltage measurement, VCCINT.                    | Voltage     | *in_voltage2_raw, in_voltage2_scale*         |
|              | 3       | Block RAM voltage measurement, VCCBRAM.                     | Voltage     | *in_voltage3_raw, in_voltage3_scale*         |
|              | 4       | PL Aux voltage measurement, VCCAUX.                         | Voltage     | *in_voltage4_raw, in_voltage4_scale*         |
|              | 5       | Voltage measurement for six DDR I/O PLLs, VCC_PSDDR_PLL.    | Voltage     | *in_voltage5_raw, in_voltage5_scale*        |
|              | 6       | VCC_PSINTFP_DDR voltage measurement.                        | Voltage     | *in_voltage6_raw, in_voltage6_scale*        |
|--------------|---------|-------------------------------------------------------------|-------------|-----------------------------|
| PS Sysmon    | 7       | LPD temperature measurement.                                | Temperature | *in_temp7_raw, in_temp7_scale, in_temp7_offset, in_temp7_input* |
|              | 8       | FPD temperature measurement (REMOTE).                       | Temperature | *in_temp8_raw, in_temp8_scale, in_temp8_offset, in_temp8_input* |
|              | 9       | VCC PS LPD voltage measurement (supply1).                   | Voltage     | *in_voltage9_raw, in_voltage9_scale*        |
|              | 10      | VCC PS FPD voltage measurement (supply2).                   | Voltage     | *in_voltage10_raw, in_voltage10_scale*         |
|              | 11      | PS Aux voltage reference (supply3).                         | Voltage     | *in_voltage11_raw, in_voltage11_scale*         |
|              | 12      | DDR I/O VCC voltage measurement.                            | Voltage     | *in_voltage12_raw, in_voltage12_scale*         |
|              | 13      | PS IO Bank 503 voltage measurement (supply5).               | Voltage     | *in_voltage13_raw, in_voltage13_scale*        |
|              | 14      | PS IO Bank 500 voltage measurement (supply6).               | Voltage     | *in_voltage14_raw, in_voltage14_scale*        |
|              | 15      | VCCO_PSIO1 voltage measurement.                             | Voltage     | *in_voltage15_raw, in_voltage15_scale*         |
|              | 16      | VCCO_PSIO2 voltage measurement.                             | Voltage     | *in_voltage16_raw, in_voltage16_scale*         |
|              | 17      | VCC_PS_GTR voltage measurement (VPS_MGTRAVCC).              | Voltage     | *in_voltage17_raw, in_voltage17_scale*           |
|              | 18      | VTT_PS_GTR voltage measurement (VPS_MGTRAVTT).              | Voltage     | *in_voltage18_raw, in_voltage18_scale*            |
|              | 19      | VCC_PSADC voltage measurement.                              | Voltage     | *in_voltage19_raw, in_voltage19_scale*            |
|--------------|---------|-------------------------------------------------------------|-------------|-----------------------------|
| PL Sysmon    | 20      | PL temperature measurement.                                 | Temperature | *in_temp20_raw, in_temp20_scale, in_temp20_offset, in_temp20_input* |
|              | 21      | PL Internal voltage measurement, VCCINT.                    | Voltage     | *in_voltage21_raw, in_voltage21_scale*            |
|              | 22      | PL Auxiliary voltage measurement, VCCAUX.                   | Voltage     | *in_voltage22_raw, in_voltage22_scale*           |
|              | 23      | ADC Reference P+ voltage measurement.                      | Voltage     | *in_voltage23_raw, in_voltage23_scale*            |
|              | 24      | ADC Reference N- voltage measurement.                      | Voltage     | *in_voltage24_raw, in_voltage24_scale*            |
|              | 25      | PL Block RAM voltage measurement, VCCBRAM.                 | Voltage     | *in_voltage25_raw, in_voltage25_scale*            |
|              | 26      | LPD Internal voltage measurement, VCC_PSINTLP (supply4).   | Voltage     | *in_voltage26_raw, in_voltage26_scale*            |
|              | 27      | FPD Internal voltage measurement, VCC_PSINTFP (supply5).   | Voltage     | *in_voltage27_raw, in_voltage27_scale*            |
|              | 28      | PS Auxiliary voltage measurement (supply6).                 | Voltage     | *in_voltage28_raw, in_voltage28_scale*            |
|              | 29      | PL VCCADC voltage measurement (vccams).                     | Voltage     | *in_voltage29_raw, in_voltage29_scale*            |
|              | 30      | Differential analog input signal voltage measurment.        | Voltage     | *in_voltage30_raw, in_voltage30_scale*            |
|              | 31      | VUser0 voltage measurement (supply7).                       | Voltage     | *in_voltage31_raw, in_voltage31_scale*            |
|              | 32      | VUser1 voltage measurement (supply8).                       | Voltage     | *in_voltage32_raw, in_voltage32_scale*            |
|              | 33      | VUser2 voltage measurement (supply9).                       | Voltage     | *in_voltage33_raw, in_voltage33_scale*            |
|              | 34      | VUser3 voltage measurement (supply10).                      | Voltage     | *in_voltage34_raw, in_voltage34_scale*            |

The information obtained is displayed in ADC code in the *_raw* file and has to be scaled with the value obtained in the *_scale* file. In the case of temperature, an offset from the *_offset* file must also be applied. 

The expressions used to pass the values read to the corresponding magnitude are shown below.

$$
V_{XX}(V) = (in\_voltageXX\_raw * in\_voltageXX\_scale) * \frac{1}{2^{n\_bits}}
$$(3.1)

$$
T_{XX}(ºC)= (in\_tempXX\_raw + in\_tempXX\_offset) * \frac{in\_tempXX\_scale}{2^{n\_bits}}
$$(3.2)

Where XX defines the selected channel number in voltage or temperature and "n_bits" defines the number of bits of the ADC used, in our case 10 bits. The offset in the case of temperature is added since a negative number is returned.

Xilinx also offers alarms applied to the voltages and temperatures measured on the previously mentioned channels and the Linux driver allows us to configure and read these alarms also using the *iio_event_monitor* tool of Linux itself. In the case of temperature, there are only alarms that are activated if a certain temperature is exceeded, while in the case of voltage, there are alarms for both overvoltage and undervoltage, but without specifying whether the limit exceeded is the lower or upper limit (shown as *either*).

# Start of programming

After an exhaustive study of all the sensing elements, the operation of each one, their corresponding characteristics and alerts and the communication channel with these devices, we can start programming the different functions that will allow us to communicate with these devices and configure them to our needs or obtain the desired information.

