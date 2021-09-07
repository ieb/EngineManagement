EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:EngineManagement-cache
EELAYER 25 0
EELAYER END
$Descr A3 16535 11693
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L GND #PWR01
U 1 1 6102626D
P 4850 1950
F 0 "#PWR01" H 4850 1950 30  0001 C CNN
F 1 "GND" H 4850 1880 30  0001 C CNN
F 2 "" H 4850 1950 60  0001 C CNN
F 3 "" H 4850 1950 60  0001 C CNN
	1    4850 1950
	1    0    0    -1  
$EndComp
$Comp
L DIODE D1
U 1 1 61026462
P 4850 1550
F 0 "D1" H 4850 1650 40  0000 C CNN
F 1 "SMBJ30CA-TR TSV" H 4850 1450 40  0000 C CNN
F 2 "Diodes_SMD:DO-214AB_Handsoldering" H 4850 1550 60  0001 C CNN
F 3 "https://uk.rs-online.com/web/p/tvs-diodes/7147197" H 4850 1550 60  0001 C CNN
	1    4850 1550
	0    -1   -1   0   
$EndComp
Text GLabel 13250 5100 0    60   Output ~ 0
Flywheel+
Text GLabel 13250 5200 0    60   Output ~ 0
Flywheel-
Text GLabel 13250 5350 0    60   Output ~ 0
CoolantTemp
Text GLabel 13250 5800 0    60   Output ~ 0
12V
$Comp
L ArdinoMinPro U3
U 1 1 61028CF3
P 6250 4800
F 0 "U3" H 6550 4800 60  0000 C CNN
F 1 "ArdinoMinPro" H 6250 4800 60  0000 C CNN
F 2 "Divers:ArduinoProMini" H 6550 4800 60  0001 C CNN
F 3 "" H 6550 4800 60  0001 C CNN
	1    6250 4800
	1    0    0    -1  
$EndComp
Text GLabel 13250 5450 0    60   Output ~ 0
FuelLevel+
Text GLabel 13250 5000 0    60   Output ~ 0
AlternatorB+
$Comp
L R R23
U 1 1 6102D900
P 8450 4300
F 0 "R23" V 8530 4300 40  0000 C CNN
F 1 "1K" V 8457 4301 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 8380 4300 30  0001 C CNN
F 3 "" H 8450 4300 30  0000 C CNN
	1    8450 4300
	1    0    0    -1  
$EndComp
$Comp
L R R22
U 1 1 6102D98B
P 7700 5050
F 0 "R22" V 7780 5050 40  0000 C CNN
F 1 "100K" V 7707 5051 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 7630 5050 30  0001 C CNN
F 3 "" H 7700 5050 30  0000 C CNN
	1    7700 5050
	0    -1   -1   0   
$EndComp
$Comp
L C C4
U 1 1 6102D9E9
P 8450 6400
F 0 "C4" H 8450 6500 40  0000 L CNN
F 1 "10nF" H 8456 6315 40  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 8488 6250 30  0001 C CNN
F 3 "" H 8450 6400 60  0000 C CNN
	1    8450 6400
	1    0    0    -1  
$EndComp
Text GLabel 11050 4950 2    60   Input ~ 0
FuelLevel+
$Comp
L R R24
U 1 1 61030102
P 8700 4300
F 0 "R24" V 8780 4300 40  0000 C CNN
F 1 "1K" V 8707 4301 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 8630 4300 30  0001 C CNN
F 3 "" H 8700 4300 30  0000 C CNN
	1    8700 4300
	1    0    0    -1  
$EndComp
$Comp
L R R21
U 1 1 61030108
P 7700 4950
F 0 "R21" V 7780 4950 40  0000 C CNN
F 1 "100K" V 7707 4951 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 7630 4950 30  0001 C CNN
F 3 "" H 7700 4950 30  0000 C CNN
	1    7700 4950
	0    -1   -1   0   
$EndComp
$Comp
L C C5
U 1 1 6103010E
P 8700 6400
F 0 "C5" H 8700 6500 40  0000 L CNN
F 1 "10nF" H 8706 6315 40  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 8738 6250 30  0001 C CNN
F 3 "" H 8700 6400 60  0000 C CNN
	1    8700 6400
	1    0    0    -1  
$EndComp
Text GLabel 11050 5050 2    60   Input ~ 0
CoolantTemp
$Comp
L R R25
U 1 1 6103127B
P 10300 5650
F 0 "R25" V 10380 5650 40  0000 C CNN
F 1 "1M" V 10307 5651 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 10230 5650 30  0001 C CNN
F 3 "" H 10300 5650 30  0000 C CNN
	1    10300 5650
	0    1    1    0   
$EndComp
$Comp
L R R26
U 1 1 61031281
P 4750 6200
F 0 "R26" V 4830 6200 40  0000 C CNN
F 1 "470K" V 4757 6201 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 4680 6200 30  0001 C CNN
F 3 "" H 4750 6200 30  0000 C CNN
	1    4750 6200
	-1   0    0    1   
$EndComp
$Comp
L C C6
U 1 1 61031287
P 9450 6400
F 0 "C6" H 9450 6500 40  0000 L CNN
F 1 "10nF" H 9456 6315 40  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 9488 6250 30  0001 C CNN
F 3 "" H 9450 6400 60  0000 C CNN
	1    9450 6400
	1    0    0    -1  
$EndComp
Text GLabel 3250 5550 0    60   Input ~ 0
ServiceBattery+
$Comp
L GND #PWR02
U 1 1 61037DA9
P 8450 6850
F 0 "#PWR02" H 8450 6850 30  0001 C CNN
F 1 "GND" H 8450 6780 30  0001 C CNN
F 2 "" H 8450 6850 60  0001 C CNN
F 3 "" H 8450 6850 60  0001 C CNN
	1    8450 6850
	1    0    0    -1  
$EndComp
$Comp
L R R27
U 1 1 61045741
P 4150 5550
F 0 "R27" V 4230 5550 40  0000 C CNN
F 1 "1M" V 4157 5551 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 4080 5550 30  0001 C CNN
F 3 "" H 4150 5550 30  0000 C CNN
	1    4150 5550
	0    1    1    0   
$EndComp
$Comp
L R R28
U 1 1 61045747
P 9900 6350
F 0 "R28" V 9980 6350 40  0000 C CNN
F 1 "470K" V 9907 6351 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 9830 6350 30  0001 C CNN
F 3 "" H 9900 6350 30  0000 C CNN
	1    9900 6350
	-1   0    0    1   
$EndComp
$Comp
L C C7
U 1 1 6104574D
P 9700 6400
F 0 "C7" H 9700 6500 40  0000 L CNN
F 1 "1nF" H 9706 6315 40  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 9738 6250 30  0001 C CNN
F 3 "" H 9700 6400 60  0000 C CNN
	1    9700 6400
	1    0    0    -1  
$EndComp
$Comp
L R R30
U 1 1 6104586E
P 5200 6200
F 0 "R30" V 5280 6200 40  0000 C CNN
F 1 "470K" V 5207 6201 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 5130 6200 30  0001 C CNN
F 3 "" H 5200 6200 30  0000 C CNN
	1    5200 6200
	-1   0    0    1   
$EndComp
$Comp
L C C8
U 1 1 61045874
P 4550 6250
F 0 "C8" H 4550 6350 40  0000 L CNN
F 1 "1nF" H 4556 6165 40  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 4588 6100 30  0001 C CNN
F 3 "" H 4550 6250 60  0000 C CNN
	1    4550 6250
	1    0    0    -1  
$EndComp
Text GLabel 2700 4800 0    60   Input ~ 0
Flywheel+
Text GLabel 2700 5100 0    60   Input ~ 0
Flywheel-
$Comp
L GND #PWR03
U 1 1 6104F995
P 4150 5350
F 0 "#PWR03" H 4150 5350 30  0001 C CNN
F 1 "GND" H 4150 5280 30  0001 C CNN
F 2 "" H 4150 5350 60  0001 C CNN
F 3 "" H 4150 5350 60  0001 C CNN
	1    4150 5350
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR04
U 1 1 610510C5
P 6250 6050
F 0 "#PWR04" H 6250 6050 30  0001 C CNN
F 1 "GND" H 6250 5980 30  0001 C CNN
F 2 "" H 6250 6050 60  0001 C CNN
F 3 "" H 6250 6050 60  0001 C CNN
	1    6250 6050
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR05
U 1 1 61051238
P 7350 4500
F 0 "#PWR05" H 7350 4500 30  0001 C CNN
F 1 "GND" H 7350 4430 30  0001 C CNN
F 2 "" H 7350 4500 60  0001 C CNN
F 3 "" H 7350 4500 60  0001 C CNN
	1    7350 4500
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR06
U 1 1 61051B42
P 5350 4700
F 0 "#PWR06" H 5350 4700 30  0001 C CNN
F 1 "GND" H 5350 4630 30  0001 C CNN
F 2 "" H 5350 4700 60  0001 C CNN
F 3 "" H 5350 4700 60  0001 C CNN
	1    5350 4700
	1    0    0    -1  
$EndComp
Text GLabel 7550 4350 2    60   Input ~ 0
8V
$Comp
L LM7808CT U1
U 1 1 610539A3
P 6050 1200
F 0 "U1" H 5850 1400 40  0000 C CNN
F 1 "LM7808CT" H 6050 1400 40  0000 L CNN
F 2 "TO_SOT_Packages_SMD:TO-263-3Lead" H 6050 1300 30  0000 C CIN
F 3 "" H 6050 1200 60  0000 C CNN
	1    6050 1200
	1    0    0    -1  
$EndComp
$Comp
L CAPAPOL C2
U 1 1 61053A5B
P 5350 1600
F 0 "C2" H 5400 1700 40  0000 L CNN
F 1 "100uF" H 5400 1500 40  0000 L CNN
F 2 "Capacitors_ThroughHole:C_Radial_D8_L11.5_P3.5" H 5450 1450 30  0001 C CNN
F 3 "" H 5350 1600 300 0000 C CNN
	1    5350 1600
	1    0    0    -1  
$EndComp
$Comp
L C C3
U 1 1 61053BCD
P 6750 1600
F 0 "C3" H 6750 1700 40  0000 L CNN
F 1 "10nF" H 6756 1515 40  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 6788 1450 30  0001 C CNN
F 3 "" H 6750 1600 60  0000 C CNN
	1    6750 1600
	1    0    0    -1  
$EndComp
$Comp
L DIODE D3
U 1 1 61053C97
P 5150 1550
F 0 "D3" H 5150 1650 40  0000 C CNN
F 1 "1N5819" H 5150 1450 40  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Horizontal_RM7mm" H 5150 1550 60  0001 C CNN
F 3 "" H 5150 1550 60  0000 C CNN
	1    5150 1550
	0    -1   -1   0   
$EndComp
Text GLabel 6900 1150 2    60   Output ~ 0
8V
$Comp
L GND #PWR07
U 1 1 610553E6
P 6050 2050
F 0 "#PWR07" H 6050 2050 30  0001 C CNN
F 1 "GND" H 6050 1980 30  0001 C CNN
F 2 "" H 6050 2050 60  0001 C CNN
F 3 "" H 6050 2050 60  0001 C CNN
	1    6050 2050
	1    0    0    -1  
$EndComp
Text Notes 2850 7700 0    60   ~ 0
Monitoring
Text Notes 6300 7000 0    60   ~ 0
Fuel Sensor\n0R == Empty\n190R ==  Full\nSupply 8V Use 1K, Full 1.2V empty 0v\nCurrent  max 8mA, 64mW\n
Text Notes 8000 3600 0    60   ~ 0
Coolant Sensor\n120C == 22R\n0C = 1743R\n120C == 8*22/(1000+22)=0.17V\n80C = 8*70/1000+70)=0.52V \n50C = 8*197/1000+70)=1.3V\n8mA\n\n\n
$Comp
L MCP2515Shield U5
U 1 1 6106FED1
P 3800 7800
F 0 "U5" H 3800 7800 60  0000 C CNN
F 1 "MCP2515Shield" H 3800 7450 60  0000 C CNN
F 2 "Divers:MCP2515Board" H 3800 7800 60  0001 C CNN
F 3 "" H 3800 7800 60  0001 C CNN
	1    3800 7800
	1    0    0    -1  
$EndComp
Text GLabel 7450 5150 2    60   Output ~ 0
SCK
Text GLabel 7450 5250 2    60   Input ~ 0
MISO
Text GLabel 7450 5350 2    60   Output ~ 0
MOSI
Text GLabel 2550 7700 0    60   Input ~ 0
SCK
Text GLabel 2550 7900 0    60   Output ~ 0
MISO
Text GLabel 2550 7800 0    60   Input ~ 0
MOSI
$Comp
L GND #PWR08
U 1 1 610738E2
P 3000 8300
F 0 "#PWR08" H 3000 8300 30  0001 C CNN
F 1 "GND" H 3000 8230 30  0001 C CNN
F 2 "" H 3000 8300 60  0001 C CNN
F 3 "" H 3000 8300 60  0001 C CNN
	1    3000 8300
	1    0    0    -1  
$EndComp
Text Notes 3250 8350 0    60   ~ 0
Can board with trancever built in. \nUsing Onboard connector\n
Text GLabel 7450 5450 2    60   Output ~ 0
CANCS
Text GLabel 2550 8000 0    60   Input ~ 0
CANCS
Text Notes 4550 3900 0    60   ~ 0
ProMini only has interrupts on pins 2 and 3\nThere is a 10K pullup on the reset on board.
NoConn ~ 6100 3950
NoConn ~ 6200 3950
NoConn ~ 6300 3950
NoConn ~ 6400 3950
NoConn ~ 6500 3950
NoConn ~ 6600 3950
$Comp
L CONN_3 K5
U 1 1 6108951E
P 14300 5100
F 0 "K5" V 14250 5100 50  0000 C CNN
F 1 "FuelCool" V 14350 5100 40  0000 C CNN
F 2 "Connect:bornier3" H 14300 5100 60  0001 C CNN
F 3 "" H 14300 5100 60  0001 C CNN
	1    14300 5100
	1    0    0    -1  
$EndComp
$Comp
L CONN_3 K7
U 1 1 61089CB1
P 14300 5800
F 0 "K7" V 14250 5800 50  0000 C CNN
F 1 "Supply" V 14350 5800 40  0000 C CNN
F 2 "Connect:bornier3" H 14300 5800 60  0001 C CNN
F 3 "" H 14300 5800 60  0001 C CNN
	1    14300 5800
	1    0    0    -1  
$EndComp
$Comp
L CONN_3 K6
U 1 1 6108DA48
P 14300 5450
F 0 "K6" V 14250 5450 50  0000 C CNN
F 1 "FuelCoolAlt" V 14350 5450 40  0000 C CNN
F 2 "Connect:bornier3" H 14300 5450 60  0001 C CNN
F 3 "" H 14300 5450 60  0001 C CNN
	1    14300 5450
	1    0    0    -1  
$EndComp
Text GLabel 3100 1150 0    60   Input ~ 0
12V
$Comp
L C C1
U 1 1 610D9E21
P 3100 4800
F 0 "C1" H 3100 4900 40  0000 L CNN
F 1 "10nF" H 3106 4715 40  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 3138 4650 30  0001 C CNN
F 3 "" H 3100 4800 60  0000 C CNN
	1    3100 4800
	0    -1   -1   0   
$EndComp
$Comp
L C C10
U 1 1 610D9F60
P 3100 5100
F 0 "C10" H 3100 5200 40  0000 L CNN
F 1 "10nF" H 3106 5015 40  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 3138 4950 30  0001 C CNN
F 3 "" H 3100 5100 60  0000 C CNN
	1    3100 5100
	0    -1   -1   0   
$EndComp
$Comp
L R R3
U 1 1 610DA16D
P 3700 4800
F 0 "R3" V 3780 4800 40  0000 C CNN
F 1 "47K" V 3707 4801 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 3630 4800 30  0001 C CNN
F 3 "" H 3700 4800 30  0000 C CNN
	1    3700 4800
	0    -1   -1   0   
$EndComp
$Comp
L R R4
U 1 1 610DA400
P 3700 5100
F 0 "R4" V 3780 5100 40  0000 C CNN
F 1 "47K" V 3707 5101 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 3630 5100 30  0001 C CNN
F 3 "" H 3700 5100 30  0000 C CNN
	1    3700 5100
	0    -1   -1   0   
$EndComp
$Comp
L DIODE D6
U 1 1 610DB325
P 4400 5050
F 0 "D6" H 4400 5150 40  0000 C CNN
F 1 "1N5819" H 4400 4950 40  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Horizontal_RM7mm" H 4400 5050 60  0001 C CNN
F 3 "" H 4400 5050 60  0000 C CNN
	1    4400 5050
	0    -1   -1   0   
$EndComp
$Comp
L DIODE D5
U 1 1 610DB487
P 4400 4400
F 0 "D5" H 4400 4500 40  0000 C CNN
F 1 "1N5819" H 4400 4300 40  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Horizontal_RM7mm" H 4400 4400 60  0001 C CNN
F 3 "" H 4400 4400 60  0000 C CNN
	1    4400 4400
	0    -1   -1   0   
$EndComp
Text GLabel 3250 5650 0    60   Input ~ 0
12V
Text GLabel 3800 4250 0    60   Input ~ 0
5V150mA
Text Notes 1200 4550 0    60   ~ 0
Signal at idle is 32v pkp, 415Hz, and rises \nfrom there as the engine speed picks up.\n\nAt idle the power disipated in the diodes is uW. \nAt 3KHz, 150V pkp currents in diodes a 1mA or less\nAt 415Hz 32v voltage across caps is < 20v\n
$Comp
L R R5
U 1 1 61121604
P 4850 4450
F 0 "R5" V 4930 4450 40  0000 C CNN
F 1 "10K" V 4857 4451 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 4780 4450 30  0001 C CNN
F 3 "" H 4850 4450 30  0000 C CNN
	1    4850 4450
	-1   0    0    1   
$EndComp
NoConn ~ 3200 7600
NoConn ~ 7050 4550
Text GLabel 8150 3900 0    60   Input ~ 0
8V
NoConn ~ 4450 7750
NoConn ~ 4450 7950
Text GLabel 11050 4850 2    60   Input ~ 0
NTC1
$Comp
L R R7
U 1 1 612B31A0
P 7700 4850
F 0 "R7" V 7780 4850 40  0000 C CNN
F 1 "100K" V 7707 4851 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 7630 4850 30  0001 C CNN
F 3 "" H 7700 4850 30  0000 C CNN
	1    7700 4850
	0    -1   -1   0   
$EndComp
$Comp
L R R6
U 1 1 612B3231
P 7700 4750
F 0 "R6" V 7780 4750 40  0000 C CNN
F 1 "100K" V 7707 4751 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 7630 4750 30  0001 C CNN
F 3 "" H 7700 4750 30  0000 C CNN
	1    7700 4750
	0    -1   -1   0   
$EndComp
$Comp
L R R2
U 1 1 612B32C0
P 7650 5550
F 0 "R2" V 7730 5550 40  0000 C CNN
F 1 "100K" V 7657 5551 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 7580 5550 30  0001 C CNN
F 3 "" H 7650 5550 30  0000 C CNN
	1    7650 5550
	0    -1   -1   0   
$EndComp
Wire Wire Line
	4850 1950 4850 1750
Wire Wire Line
	4850 1350 4850 1150
Wire Wire Line
	3100 1150 4100 1150
Wire Wire Line
	7050 4950 7450 4950
Wire Wire Line
	8450 4550 8450 6200
Wire Wire Line
	7950 5050 11050 5050
Connection ~ 8450 5050
Wire Wire Line
	8150 3900 8700 3900
Wire Wire Line
	8950 3900 9450 3900
Wire Wire Line
	8450 4050 8450 3900
Wire Wire Line
	8450 6600 8450 6850
Connection ~ 8450 6700
Wire Wire Line
	7450 5050 7050 5050
Wire Wire Line
	6250 6050 6250 5950
Wire Wire Line
	7350 4500 7350 4450
Wire Wire Line
	7350 4450 7050 4450
Wire Wire Line
	5550 4650 5350 4650
Wire Wire Line
	5350 4650 5350 4700
Wire Wire Line
	7550 4350 7050 4350
Wire Wire Line
	4600 1150 5650 1150
Wire Wire Line
	5350 1400 5350 1150
Connection ~ 5350 1150
Wire Wire Line
	5150 1850 6750 1850
Wire Wire Line
	6750 1850 6750 1800
Wire Wire Line
	5350 1850 5350 1800
Wire Wire Line
	6050 1450 6050 2050
Connection ~ 6050 1850
Wire Wire Line
	6450 1150 6900 1150
Wire Wire Line
	6750 1150 6750 1400
Wire Wire Line
	8450 6700 9900 6700
Wire Notes Line
	800  6000 900  6000
Wire Wire Line
	7450 5150 7050 5150
Wire Wire Line
	7450 5250 7050 5250
Wire Wire Line
	7450 5350 7050 5350
Wire Wire Line
	7450 5450 7050 5450
Wire Wire Line
	3800 7250 3800 7350
Wire Wire Line
	3000 8300 3000 8100
Wire Wire Line
	3000 8100 3200 8100
Wire Wire Line
	2550 8000 3200 8000
Wire Wire Line
	5150 1350 5150 1150
Connection ~ 5150 1150
Wire Wire Line
	5150 1750 5150 1850
Connection ~ 5350 1850
Wire Wire Line
	4850 4800 3950 4800
Wire Wire Line
	4400 4600 4400 4850
Connection ~ 4400 4800
Wire Wire Line
	4400 5250 4400 5300
Wire Wire Line
	4400 5300 4150 5300
Wire Wire Line
	4150 5100 4150 5350
Wire Wire Line
	4150 5100 3950 5100
Connection ~ 4150 5300
Wire Wire Line
	3450 5100 3300 5100
Wire Wire Line
	3450 4800 3300 4800
Wire Wire Line
	2900 4800 2700 4800
Wire Wire Line
	2900 5100 2700 5100
Wire Wire Line
	4850 4700 4850 4800
Wire Wire Line
	4400 4150 4400 4200
Wire Wire Line
	3900 4150 4850 4150
Wire Wire Line
	3900 4150 3900 4250
Wire Wire Line
	3900 4250 3800 4250
Wire Wire Line
	4850 4150 4850 4200
Connection ~ 4400 4150
Wire Wire Line
	2550 7900 3200 7900
Wire Wire Line
	2550 7800 3200 7800
Wire Wire Line
	2550 7700 3200 7700
Connection ~ 8450 3900
Connection ~ 4850 4750
Wire Wire Line
	3400 7250 3800 7250
Wire Wire Line
	7050 5650 10050 5650
Wire Wire Line
	7050 5550 7400 5550
Wire Wire Line
	7050 4850 7450 4850
Wire Wire Line
	7050 4750 7450 4750
$Comp
L C C12
U 1 1 612B43B9
P 8950 6400
F 0 "C12" H 8950 6500 40  0000 L CNN
F 1 "10nF" H 8956 6315 40  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 8988 6250 30  0001 C CNN
F 3 "" H 8950 6400 60  0000 C CNN
	1    8950 6400
	1    0    0    -1  
$EndComp
$Comp
L C C13
U 1 1 612B4452
P 9200 6400
F 0 "C13" H 9200 6500 40  0000 L CNN
F 1 "10nF" H 9206 6315 40  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 9238 6250 30  0001 C CNN
F 3 "" H 9200 6400 60  0000 C CNN
	1    9200 6400
	1    0    0    -1  
$EndComp
$Comp
L R R8
U 1 1 612B45A5
P 8950 4300
F 0 "R8" V 9030 4300 40  0000 C CNN
F 1 "22K" V 8957 4301 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 8880 4300 30  0001 C CNN
F 3 "" H 8950 4300 30  0000 C CNN
	1    8950 4300
	1    0    0    -1  
$EndComp
$Comp
L R R10
U 1 1 612B463F
P 9200 4300
F 0 "R10" V 9280 4300 40  0000 C CNN
F 1 "22K" V 9207 4301 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 9130 4300 30  0001 C CNN
F 3 "" H 9200 4300 30  0000 C CNN
	1    9200 4300
	1    0    0    -1  
$EndComp
$Comp
L R R11
U 1 1 612B46DE
P 9450 4300
F 0 "R11" V 9530 4300 40  0000 C CNN
F 1 "22K" V 9457 4301 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 9380 4300 30  0001 C CNN
F 3 "" H 9450 4300 30  0000 C CNN
	1    9450 4300
	1    0    0    -1  
$EndComp
Wire Wire Line
	8700 3900 8700 4050
Connection ~ 8700 3900
Wire Wire Line
	8700 4550 8700 6200
Wire Wire Line
	8950 4550 8950 6200
Wire Wire Line
	9200 4550 9200 6200
Wire Wire Line
	8700 6700 8700 6600
Wire Wire Line
	8950 6700 8950 6600
Connection ~ 8700 6700
Wire Wire Line
	9200 6700 9200 6600
Connection ~ 8950 6700
Wire Wire Line
	9450 6700 9450 6600
Connection ~ 9200 6700
Wire Wire Line
	9450 4550 9450 6200
Wire Wire Line
	8950 3800 8950 4050
Wire Wire Line
	9200 3900 9200 4050
Connection ~ 8950 3900
Wire Wire Line
	9450 3900 9450 4050
Connection ~ 9200 3900
Wire Wire Line
	7950 4950 11050 4950
Connection ~ 8700 4950
Wire Wire Line
	7950 4850 11050 4850
Connection ~ 8950 4850
Wire Wire Line
	7950 4750 11050 4750
Connection ~ 9200 4750
Wire Wire Line
	7900 5550 10950 5550
Connection ~ 9450 5550
Wire Wire Line
	9700 6700 9700 6600
Connection ~ 9450 6700
Wire Wire Line
	9900 6700 9900 6600
Connection ~ 9700 6700
Wire Wire Line
	9700 5650 9700 6200
Wire Wire Line
	9900 5650 9900 6100
Connection ~ 9900 5650
Connection ~ 9700 5650
$Comp
L C C9
U 1 1 612B7B6B
P 5000 6250
F 0 "C9" H 5000 6350 40  0000 L CNN
F 1 "1nF" H 5006 6165 40  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 5038 6100 30  0001 C CNN
F 3 "" H 5000 6250 60  0000 C CNN
	1    5000 6250
	1    0    0    -1  
$EndComp
Wire Wire Line
	4400 5550 5550 5550
$Comp
L R R1
U 1 1 612B7D6A
P 4150 5650
F 0 "R1" V 4230 5650 40  0000 C CNN
F 1 "1M" V 4157 5651 40  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 4080 5650 30  0001 C CNN
F 3 "" H 4150 5650 30  0000 C CNN
	1    4150 5650
	0    1    1    0   
$EndComp
Wire Wire Line
	4400 5650 5550 5650
Text GLabel 10950 5650 2    60   Input ~ 0
AlternatorB+
Wire Wire Line
	3250 5550 3900 5550
Wire Wire Line
	3250 5650 3900 5650
Text GLabel 11050 4750 2    60   Input ~ 0
NTC2
Wire Wire Line
	10550 5650 10950 5650
Text GLabel 10950 5550 2    60   Input ~ 0
NTC3
$Comp
L GND #PWR010
U 1 1 612B95DA
P 5200 6700
F 0 "#PWR010" H 5200 6700 30  0001 C CNN
F 1 "GND" H 5200 6630 30  0001 C CNN
F 2 "" H 5200 6700 60  0001 C CNN
F 3 "" H 5200 6700 60  0001 C CNN
	1    5200 6700
	1    0    0    -1  
$EndComp
Wire Wire Line
	5200 6450 5200 6700
Wire Wire Line
	4550 6600 5200 6600
Wire Wire Line
	4550 6600 4550 6450
Connection ~ 5200 6600
Wire Wire Line
	4750 6450 4750 6600
Connection ~ 4750 6600
Wire Wire Line
	5000 6450 5000 6600
Connection ~ 5000 6600
Wire Wire Line
	5200 5950 5200 5550
Connection ~ 5200 5550
Wire Wire Line
	5000 6050 5000 5550
Connection ~ 5000 5550
Wire Wire Line
	4750 5950 4750 5650
Connection ~ 4750 5650
Wire Wire Line
	4550 6050 4550 5650
Connection ~ 4550 5650
Wire Wire Line
	13950 5900 13150 5900
Wire Wire Line
	13250 5800 13950 5800
Text GLabel 13150 5900 0    60   Output ~ 0
ServiceBattery+
Wire Wire Line
	12600 5700 13950 5700
$Comp
L CONN_3 K1
U 1 1 612BBB9F
P 14300 4700
F 0 "K1" V 14250 4700 50  0000 C CNN
F 1 "Temps" V 14350 4700 40  0000 C CNN
F 2 "Connect:bornier3" H 14300 4700 60  0001 C CNN
F 3 "" H 14300 4700 60  0001 C CNN
	1    14300 4700
	1    0    0    -1  
$EndComp
Text GLabel 13250 4800 0    60   Output ~ 0
NTC1
Text GLabel 13250 4700 0    60   Output ~ 0
NTC2
Text GLabel 13250 4600 0    60   Output ~ 0
NTC3
$Comp
L GND #PWR011
U 1 1 612BBFFF
P 13150 5600
F 0 "#PWR011" H 13150 5600 30  0001 C CNN
F 1 "GND" H 13150 5530 30  0001 C CNN
F 2 "" H 13150 5600 60  0001 C CNN
F 3 "" H 13150 5600 60  0001 C CNN
	1    13150 5600
	1    0    0    -1  
$EndComp
Wire Wire Line
	13250 4600 13950 4600
Wire Wire Line
	13250 4700 13950 4700
Wire Wire Line
	13250 4800 13950 4800
Wire Wire Line
	13250 5000 13950 5000
Wire Wire Line
	13250 5100 13950 5100
Wire Wire Line
	13250 5200 13950 5200
Wire Wire Line
	13250 5350 13950 5350
Wire Wire Line
	13250 5450 13950 5450
Wire Wire Line
	13950 5550 13150 5550
Wire Wire Line
	13150 5550 13150 5600
Text Notes 9800 4950 0    60   ~ 0
NTC MF52-10K\n100K at 0C  5*100/(100+10)=4.5v \n25K at 5C  5*25/(25+10)=3.5v\n10K at 20C  5*10/(10+10)=2.5v\n506R at 110C 5*0.506/(0.506+10)=0.179v\n\nx =22K\n\n\n\n\n\n
Wire Wire Line
	4850 4750 5550 4750
Text GLabel 7250 4650 2    60   Output ~ 0
5V150mA
Wire Wire Line
	7050 4650 7250 4650
Text GLabel 8300 3800 0    60   Input ~ 0
5V150mA
Wire Wire Line
	8300 3800 8950 3800
Connection ~ 6750 1150
Connection ~ 4850 1150
$Comp
L THERMISTOR TH1
U 1 1 612C2B50
P 4350 1150
F 0 "TH1" V 4450 1200 50  0000 C CNN
F 1 "500mA Thermistor" V 4250 1150 50  0000 C CNN
F 2 "Resistors_ThroughHole:Resistor_Horizontal_RM7mm" H 4350 1150 60  0001 C CNN
F 3 "" H 4350 1150 60  0000 C CNN
	1    4350 1150
	0    1    1    0   
$EndComp
Text GLabel 3400 7250 0    60   Input ~ 0
5V150mA
Text GLabel 12600 5700 0    60   Input ~ 0
8V
$EndSCHEMATC
