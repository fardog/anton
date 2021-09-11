EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "PMS7003 2.54mm Breakout"
Date "2021-08-29"
Rev "0.1.0"
Comp "nwittstock"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L power:+5V #PWR0101
U 1 1 612C3E89
P 2550 2950
F 0 "#PWR0101" H 2550 2800 50  0001 C CNN
F 1 "+5V" H 2565 3123 50  0000 C CNN
F 2 "" H 2550 2950 50  0001 C CNN
F 3 "" H 2550 2950 50  0001 C CNN
	1    2550 2950
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0102
U 1 1 612C489A
P 2550 3250
F 0 "#PWR0102" H 2550 3000 50  0001 C CNN
F 1 "GND" H 2555 3077 50  0000 C CNN
F 2 "" H 2550 3250 50  0001 C CNN
F 3 "" H 2550 3250 50  0001 C CNN
	1    2550 3250
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x10 J1
U 1 1 612C6F72
P 3400 3350
F 0 "J1" H 3480 3342 50  0000 L CNN
F 1 "PMS7003" H 3480 3251 50  0000 L CNN
F 2 "Connector_PinHeader_1.27mm:PinHeader_2x05_P1.27mm_Vertical" H 3400 3350 50  0001 C CNN
F 3 "~" H 3400 3350 50  0001 C CNN
	1    3400 3350
	1    0    0    -1  
$EndComp
Wire Wire Line
	3200 2950 3000 2950
Wire Wire Line
	3200 3050 3000 3050
Wire Wire Line
	3000 3050 3000 2950
Connection ~ 3000 2950
Wire Wire Line
	3200 3250 3000 3250
Wire Wire Line
	3200 3150 3000 3150
Wire Wire Line
	3000 3150 3000 3250
Connection ~ 3000 3250
NoConn ~ 3200 3450
NoConn ~ 3200 3650
Text GLabel 3200 3350 0    50   Input ~ 0
RST
Text GLabel 3200 3550 0    50   Input ~ 0
RXD
Text GLabel 3200 3750 0    50   Input ~ 0
TXD
Text GLabel 3200 3850 0    50   Input ~ 0
SET
$Comp
L Connector_Generic:Conn_01x06 J2
U 1 1 612CDAA5
P 4900 3150
F 0 "J2" H 4980 3142 50  0000 L CNN
F 1 "Header" H 4980 3051 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical" H 4900 3150 50  0001 C CNN
F 3 "~" H 4900 3150 50  0001 C CNN
	1    4900 3150
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR0103
U 1 1 612CF633
P 4150 2950
F 0 "#PWR0103" H 4150 2800 50  0001 C CNN
F 1 "+5V" H 4165 3123 50  0000 C CNN
F 2 "" H 4150 2950 50  0001 C CNN
F 3 "" H 4150 2950 50  0001 C CNN
	1    4150 2950
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0104
U 1 1 612D0031
P 4150 3050
F 0 "#PWR0104" H 4150 2800 50  0001 C CNN
F 1 "GND" H 4155 2877 50  0000 C CNN
F 2 "" H 4150 3050 50  0001 C CNN
F 3 "" H 4150 3050 50  0001 C CNN
	1    4150 3050
	1    0    0    -1  
$EndComp
Wire Wire Line
	4150 2950 4700 2950
Wire Wire Line
	4150 3050 4700 3050
Text GLabel 4700 3150 0    50   Input ~ 0
RST
Text GLabel 4700 3250 0    50   Input ~ 0
TXD
Text GLabel 4700 3350 0    50   Input ~ 0
RXD
Text GLabel 4700 3450 0    50   Input ~ 0
SET
$Comp
L Device:C C1
U 1 1 612D311B
P 2650 3100
F 0 "C1" H 2765 3146 50  0000 L CNN
F 1 "10uF" H 2765 3055 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.18x1.45mm_HandSolder" H 2688 2950 50  0001 C CNN
F 3 "~" H 2650 3100 50  0001 C CNN
	1    2650 3100
	1    0    0    -1  
$EndComp
Connection ~ 2650 2950
Wire Wire Line
	2650 2950 2550 2950
Connection ~ 2650 3250
Wire Wire Line
	2650 3250 2550 3250
Wire Wire Line
	2650 2950 3000 2950
Wire Wire Line
	2650 3250 3000 3250
$EndSCHEMATC
