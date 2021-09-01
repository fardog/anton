EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "SD Card SPI Breakout"
Date "2021-08-30"
Rev "v0.1.0"
Comp "nwittstock"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L power:+3V3 #PWR0101
U 1 1 612D8E2B
P 4050 2850
F 0 "#PWR0101" H 4050 2700 50  0001 C CNN
F 1 "+3V3" H 4065 3023 50  0000 C CNN
F 2 "" H 4050 2850 50  0001 C CNN
F 3 "" H 4050 2850 50  0001 C CNN
	1    4050 2850
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0102
U 1 1 612DAB16
P 1250 4300
F 0 "#PWR0102" H 1250 4050 50  0001 C CNN
F 1 "GND" H 1255 4127 50  0000 C CNN
F 2 "" H 1250 4300 50  0001 C CNN
F 3 "" H 1250 4300 50  0001 C CNN
	1    1250 4300
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR0103
U 1 1 612DB861
P 4750 3150
F 0 "#PWR0103" H 4750 3000 50  0001 C CNN
F 1 "+3V3" H 4765 3323 50  0000 C CNN
F 2 "" H 4750 3150 50  0001 C CNN
F 3 "" H 4750 3150 50  0001 C CNN
	1    4750 3150
	1    0    0    -1  
$EndComp
$Comp
L Device:R R3
U 1 1 612DCF8C
P 5050 3950
F 0 "R3" V 5257 3950 50  0000 C CNN
F 1 "4.7K" V 5166 3950 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric" V 4980 3950 50  0001 C CNN
F 3 "~" H 5050 3950 50  0001 C CNN
	1    5050 3950
	0    -1   -1   0   
$EndComp
$Comp
L Device:R R2
U 1 1 612E0013
P 5050 3650
F 0 "R2" V 5257 3650 50  0000 C CNN
F 1 "4.7K" V 5166 3650 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric" V 4980 3650 50  0001 C CNN
F 3 "~" H 5050 3650 50  0001 C CNN
	1    5050 3650
	0    -1   -1   0   
$EndComp
$Comp
L Device:R R1
U 1 1 612E0656
P 5050 3350
F 0 "R1" V 5257 3350 50  0000 C CNN
F 1 "4.7K" V 5166 3350 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric" V 4980 3350 50  0001 C CNN
F 3 "~" H 5050 3350 50  0001 C CNN
	1    5050 3350
	0    -1   -1   0   
$EndComp
$Comp
L Device:R R4
U 1 1 612E0A5A
P 5050 4250
F 0 "R4" V 5257 4250 50  0000 C CNN
F 1 "4.7K" V 5166 4250 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric" V 4980 4250 50  0001 C CNN
F 3 "~" H 5050 4250 50  0001 C CNN
	1    5050 4250
	0    -1   -1   0   
$EndComp
Wire Wire Line
	4900 3350 4750 3350
Wire Wire Line
	4900 3650 4750 3650
Wire Wire Line
	4750 3650 4750 3350
Connection ~ 4750 3350
Wire Wire Line
	4900 3950 4750 3950
Wire Wire Line
	4750 3950 4750 3650
Connection ~ 4750 3650
Wire Wire Line
	4900 4250 4750 4250
Wire Wire Line
	4750 4250 4750 3950
Connection ~ 4750 3950
Wire Wire Line
	4750 3150 4750 3350
Wire Notes Line
	4500 2850 5650 2850
Wire Notes Line
	5650 2850 5650 4400
Wire Notes Line
	5650 4400 4500 4400
Wire Notes Line
	4500 4400 4500 2850
Text Notes 4500 2850 0    50   ~ 0
Pullup
$Comp
L Connector_Generic:Conn_01x06 J2
U 1 1 612E38F0
P 6750 3550
F 0 "J2" H 6830 3542 50  0000 L CNN
F 1 "Header" H 6830 3451 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical" H 6750 3550 50  0001 C CNN
F 3 "~" H 6750 3550 50  0001 C CNN
	1    6750 3550
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR0104
U 1 1 612E4E73
P 6050 3350
F 0 "#PWR0104" H 6050 3200 50  0001 C CNN
F 1 "+3V3" H 6065 3523 50  0000 C CNN
F 2 "" H 6050 3350 50  0001 C CNN
F 3 "" H 6050 3350 50  0001 C CNN
	1    6050 3350
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0105
U 1 1 612E5744
P 6050 3450
F 0 "#PWR0105" H 6050 3200 50  0001 C CNN
F 1 "GND" H 6055 3277 50  0000 C CNN
F 2 "" H 6050 3450 50  0001 C CNN
F 3 "" H 6050 3450 50  0001 C CNN
	1    6050 3450
	1    0    0    -1  
$EndComp
Text Label 1550 4050 2    50   ~ 0
~CS
Text Label 3550 4150 0    50   ~ 0
SDI
Text Label 3550 3950 0    50   ~ 0
SCK
Text Label 3550 3750 0    50   ~ 0
SDO
Text Label 6450 3750 2    50   ~ 0
SDO
Text Label 6450 3550 2    50   ~ 0
SCK
Text Label 6450 3650 2    50   ~ 0
SDI
Text Label 6450 3850 2    50   ~ 0
~CS
Text Label 5350 3350 0    50   ~ 0
~CS
Text Label 5350 3650 0    50   ~ 0
SDI
Text Label 5350 3950 0    50   ~ 0
SDO
Text Label 5350 4250 0    50   ~ 0
SCK
Wire Wire Line
	5350 4250 5200 4250
Wire Wire Line
	5350 3950 5200 3950
Wire Wire Line
	5350 3650 5200 3650
Wire Wire Line
	5350 3350 5200 3350
Wire Wire Line
	6050 3450 6550 3450
Wire Wire Line
	6450 3750 6550 3750
Wire Wire Line
	6550 3650 6450 3650
Wire Wire Line
	6450 3550 6550 3550
Wire Wire Line
	6050 3350 6550 3350
Wire Wire Line
	6550 3850 6450 3850
$Comp
L Device:C C2
U 1 1 612ECDA3
P 3900 3450
F 0 "C2" V 3648 3450 50  0000 C CNN
F 1 "0.1uF" V 3739 3450 50  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 3938 3300 50  0001 C CNN
F 3 "~" H 3900 3450 50  0001 C CNN
	1    3900 3450
	0    1    1    0   
$EndComp
$Comp
L Device:C C1
U 1 1 612EDAB9
P 3900 3050
F 0 "C1" V 3648 3050 50  0000 C CNN
F 1 "10uF" V 3739 3050 50  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 3938 2900 50  0001 C CNN
F 3 "~" H 3900 3050 50  0001 C CNN
	1    3900 3050
	0    1    1    0   
$EndComp
$Comp
L molex_1051620001:1051620001 J1
U 1 1 612D71FF
P 1650 3550
F 0 "J1" H 2550 3915 50  0000 C CNN
F 1 "Card" H 2550 3824 50  0000 C CNN
F 2 "molex_1051620001:1051620001" H 1650 3550 50  0001 C CNN
F 3 "~" H 1650 3550 50  0001 C CNN
	1    1650 3550
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0106
U 1 1 61307036
P 3450 3050
F 0 "#PWR0106" H 3450 2800 50  0001 C CNN
F 1 "GND" H 3455 2877 50  0000 C CNN
F 2 "" H 3450 3050 50  0001 C CNN
F 3 "" H 3450 3050 50  0001 C CNN
	1    3450 3050
	1    0    0    -1  
$EndComp
Wire Wire Line
	3450 3050 3750 3050
Wire Wire Line
	3750 3050 3750 3450
Connection ~ 3750 3050
Wire Wire Line
	3750 3450 3750 3850
Wire Wire Line
	3750 3850 3450 3850
Connection ~ 3750 3450
Wire Wire Line
	4050 2850 4050 3050
Wire Wire Line
	4050 3050 4050 3450
Connection ~ 4050 3050
Wire Wire Line
	4050 3450 4050 4050
Wire Wire Line
	4050 4050 3450 4050
Connection ~ 4050 3450
Wire Wire Line
	3550 3750 3450 3750
Wire Wire Line
	3450 4150 3550 4150
Wire Wire Line
	3450 3950 3550 3950
Wire Wire Line
	1550 4050 1650 4050
NoConn ~ 1650 3950
NoConn ~ 3450 3550
NoConn ~ 3450 3650
Wire Wire Line
	1650 3550 1250 3550
Wire Wire Line
	1250 3550 1250 3650
Wire Wire Line
	1650 3650 1250 3650
Connection ~ 1250 3650
Wire Wire Line
	1250 3650 1250 3750
Wire Wire Line
	1650 3750 1250 3750
Connection ~ 1250 3750
Wire Wire Line
	1250 3750 1250 3850
Wire Wire Line
	1650 3850 1250 3850
Connection ~ 1250 3850
Wire Wire Line
	1250 3850 1250 4300
$EndSCHEMATC
