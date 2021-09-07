# Engine Management 



In July 2021 my boat was hit by lightning. The Volvo Penta D2-40 MDI unit that manages the Engine, a Volvo Penta D2-40F failed badly leaving the starter motor engaged and nearly causing an engine fire.

# Main control board

(see pcb/EngineManagementSimpleWithRelays.sch )

This project is based on the standard wiring for the core engine which is a Perkins 100. In that form the engine is controlled by switches, relays and a few flyback diodes. The only part of the original engine control that survived the strike was the Volvo Penta buttons, so those are used by a control board with pnp transistors driving relays triggered by the original buttons pulling the base of the pnp to ground. PushOn/PushOff is provided by a latching relay unit based on a LED control chip that draws a few uA when off. The control circuit draws 0.5A max and is protected by a 5A fuse and 600W TVS which will blow the fuse in the event of a voltage surge.

The main relays for Start, Stop, Glow and aux are all standard iso 40A automotive relays, each curcuit fused with a suitable fuse. 

There are basic alarms for oil pressure, charge and temperature with the temperature alarm using a differntial op amp with high gain adjusted to turn on a pnp at a set temperature. These all drive a buzzer with LEDs to indicate the alarm. 

A standard Thachometer sensing from the alternator W+ provides rev and engine hours. These are almost disposable being 30 GBP on eBay from many sources, all 85mm same as the Volvo Penta Tachometer.

# NMEA2000 Interface


(see pcb/EngineManagement.sch)

The standard MDI talks J1939 which is the Diesel engne CAN standard protocol. Normally this communicates with other Volvo Penta instruments, but in my case this was only the Tachometer. The J1939 Volvo Penta Tachomter did not survive the strike. The CAN bus interface was destroyed, even though there was an attempt to protect if from surges. At over 500 GBP to replace, I decided to stick with the non J1939 tachometer from eBay. For details on how to drive J1939 see the main_deprecated.cpp file. Volvo Penta service technicians probably query the MDI for fault codes, but for owners there is limited benefit where the only unit is a tachometer or basic instruments.

Once I discovered the VP Tachometer was not repairable, a seperate mechanical diesel to NMEA2000 board was designed and built arround an Arduino Pro Mini (Atmel 328p MCU), MCP2515 CAN controller board and some passive components. There is just enough space on 328p to hold the code and run the controller and the data collected and emitted can be displayed on any NMEA2000 instrument (eg Raymarine e7 MFD).

Component cost of the boards is probably less than 30 GBP, hence spares boards can be carried and in the worst case the engine can be controlled manually.

Replacing the blown VP parts with OEM parts would have been 1500 GBP with a firther 1500 GBP requred for the spare set, safe in the knowledge that a EM pulse from a near strike would result in precisely the same failure and risk of engine fire.




# Sensors

The standard  VP flywheel pickup produces 415Hz 20V pk2pk puses with sharp edges at idle and so can be fed through passive conditioning to procuse a 5V square wave. The pk2pk voltage rises with rpm as does frequency. 

The standard VP oil pressure switch is a simple switch that closes when the oil pressure drops below a threashold.

The standard VP coolant temperature sensor is identical to almost every other coolant sensor, with about 1743R at 0C and 22R at 120C. When about 33R (105C adjustable) is measured the alarm is triggered.

The charge alarm uses the voltage across the resistor supplying current into the alternator excitation coil, alanagous to the charge light in cars long before there were computers.

The NMEA2000 interface board also measures fule level (0R-190R), battery voltage, alternator voltage, and 3 NCT inputs. Inputs are protected with capacitors and 100K resistors to limit current.


# Outputs

The relay board provides outputs for starter motor, stop solenoid, glow plugs and aux 12v.

The NMEA2000 board provides NMEA2000 messages.


# Connections.

The VP wiring loom has largely been replaced except for the connection to the VP buttons. Most of the original wiring loom was melted by the starter motor, but the rest was strapped tightly to the engine risking shorts when it melted.

The control box will be mounted outside the engine bay where temperatures are considerably lower (typically < 30C vs often > 85C next to the engine).



# Todo/Status

[x] Build prototype PCB and populate.
[x] Test on/off sequence.
[x] Test 20x4 I2C display.
[x] Test port extender. - discovered Can board backpowers 5vswitched line, added blocking diode.
[x] Connect 8 way relay board and test.
[x] Verify CAN board working.
[ ] Connect test harness sensors and test.
[FAIL] Connect VP Taco and test with simulated engine - the VP Taco is blown internally, CanH at 5V, CanL and 0V. Inside three is a large Asic which looks like it directly controls the Can bus. Very unlikely to be 
able to repair. Google for the numbers turns up no chips, and none of the other chips are CAN related.

[x] Design build and test simple relay controller unit with no MCU
[x] Design build NMEA2000 controller board
[ ] bench test NMEA2000 controller board
[ ] install
