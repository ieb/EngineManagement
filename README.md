# Engine Management 



In July 2021 my boat was hit by lightning. The Volvo Penta D2-40 MDI unit that manages the Engine, a Volvo Penta D2-40F failed badly leaving the starter motor engaged and nearly causing an engine fire.

# Main control board

(see pcb/EngineManagementSimpleWithRelays.sch )

This project is based on the standard wiring for the core engine which is a Perkins 100. In that form the engine is controlled by switches, relays and a few flyback diodes. The only part of the original engine control that survived the strike was the Volvo Penta buttons, so those are used by a control board with pnp transistors driving relays triggered by the original buttons pulling the base of the pnp to ground. PushOn/PushOff is provided by a latching relay unit based on a LED control chip that draws a few uA when off. The control circuit draws 0.5A max and is protected by a 5A fuse and 600W TVS which will blow the fuse in the event of a voltage surge.

The main relays for Start, Stop, Glow and aux are all standard iso 40A automotive relays, each curcuit fused with a suitable fuse. 

There are basic alarms for oil pressure, charge and temperature with the temperature alarm using a differntial op amp with high gain adjusted to turn on a pnp at a set temperature. These all drive a buzzer with LEDs to indicate the alarm. 

A standard Thachometer sensing from the alternator W+ provides rev and engine hours. These are almost disposable being 30 GBP on eBay from many sources, all 85mm same as the Volvo Penta Tachometer.

## Mosfet alternative

Although the original MDI failed due to the MOSFETs going short circuit, and in this respect relays may be more robust, it should be possble to protect MOSFETs or at least fail safe.

A 1.5KW TVS (eg SMCJ14A) with a 14.0V revers standoff, 15.6V minimum breakdown will conduct 5A@16V, 10A@18V and 40A@20V, which makes it viable as protection for excessive voltages upto 40A current, aiming to blow a fuse at above those voltages. Each mosfet needs its own TVS + suitably sized fuse or breaker. In the event of a stike or surge the TVS will short and blow the fuse before damage is done even if the MOSFET is shorted. It wont work for currents greater than 40A without exceeding the max Vds voltage of the MOSFET below.

A SI7137DP-T1-GE3 has an on resitance of 0.0016 Ohms and will dispate < 640mW for 20A when fully on. It, unfortunately has a Max Vds of 20V and Vgs of 12v so needs protection, but provided its protected represents a robust solution as a high level switch. 

In the relay board on/off was provided by a eBay relay board which used a LED controler. To keep the number of daughter boards to a minimum and save space this is replaced by a MAX15054 on-off controller driving a N-Channel mosfet which when turned on draws the gate of one of the SI7137's to near zero turning it fully on. The same N-Channel mosfet enables P-Channel mosfets connected to the gates of other SI7137s. When the VP buttons are pressed the gates of these mosfets are dragged down to 0v turning them on, and since the N-Channel mosfet is conducting the gate of the respective SI7137 is also dragged to 0v switching that mosfet on. All P-Channel gates are pulled to +ve, and all N-Channel gates are pulled to 0v. The SI7137 has a max source gate voltage of 12v and so a 10v zener with forward diode ensures tht nevery gets over 11.5v. 

Simulations indicate that the power dissipation in the SI7137 at 20A is 2.5W, so a pair are used for the glow plugs. At 8A for the start and stop solenoids power disipation is closer ot 500mW. Board cooling pads are used, although it migh be necessary to add chip heat sinks.

Since the SI7137 are 1/10th the size of the relays, the switching circuit and NMEA2000 interface controller will all fit onto a single 10cm x 15cm board (which is the size I have in stock).

All of the silicon components were available at Farnell except the SI7137's which were ordered from Mouser, airmailed from California. For some reaon RS in the UK had no stock of any mosfets.

see pcb/EngineManagementWithMosfets.sch which includes a subsheet of the NMEA interface. There are LTSpice simulations in pcb/simulations/ of the mosfet layout confirming transient behaviour, currents and power disapation.

Since its likely that when there is a voltage spike something on the board will blow, assuming this board works, I'll probably make a batch and keep a few spares onborad the boat.


# NMEA2000 Interface


(see pcb/EngineManagement.sch)

The standard MDI talks J1939 which is the Diesel engne CAN standard protocol. Normally this communicates with other Volvo Penta instruments, but in my case this was only the Tachometer. The J1939 Volvo Penta Tachomter did not survive the strike. The CAN bus interface was destroyed, even though there was an attempt to protect if from surges. At over 500 GBP to replace, I decided to stick with the non J1939 tachometer from eBay. For details on how to drive J1939 see the main_deprecated.cpp file. Volvo Penta service technicians probably query the MDI for fault codes, but for owners there is limited benefit where the only unit is a tachometer or basic instruments.

Once I discovered the VP Tachometer was not repairable, a seperate mechanical diesel to NMEA2000 board was designed and built arround an Arduino Pro Mini (Atmel 328p MCU), MCP2515 CAN controller board and some passive components. There is just enough space on 328p to hold the code and run the controller and the data collected and emitted can be displayed on any NMEA2000 instrument (eg Raymarine e7 MFD).

Component cost of the boards is probably less than 30 GBP, hence spares boards can be carried and in the worst case the engine can be controlled manually.

Replacing the blown VP parts with OEM parts would have been 1500 GBP with a firther 1500 GBP requred for the spare set, safe in the knowledge that a EM pulse from a near strike would result in precisely the same failure and risk of engine fire.




# Sensors



The standard  VP flywheel pickup produces 415Hz 20V pk2pk puses with sharp edges at idle, however the signal is noisy and so it needs to be conditioned. A Schmitt trigger using an OpAmp set up positive feedback to enduce histeresis. The positive input uses a RC network acting as a low pass filter reducing the sensitivity as the rpm rises, eliminating high frequencies ( > 10KHz). On the negative a RC network smooths the input voltage to find the mean allowing automatic offset compensation. The time period constant of the -ve input is >> than the +ve.  The inputs are via 22K resistors with the voltage measured over 100K. There are 2 chopper diodes to limit the input voltage to no more than 1V.

Using a 358 opamp with 5V supply the square wave out is 3.7V high and 0.7v low, which is just enough (3.5v being logic high), 1.5 below Vcc isnt great for an OpAmp, however 3v is high on an Arduino.

The frequency is measured by timing 10 pulses using micros().

The previous attempt without a Schmitt Trigger failed to work reliably due to noise.


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

# Code

Code is all in main.cpp rather than a classes since there is little to it and I needed tight control over memory usage (less risk of hidden bloat). NMEA support uses 

	https://github.com/ttlappalainen/NMEA2000.git
	https://github.com/ttlappalainen/NMEA2000_mcp.git
	https://github.com/ttlappalainen/CAN_BUS_Shield.git

however there are some patches required, see src/*.patch. 
src/CAN_BUS_Shield.patch. The SPI clk was heavilly distorted at 8MHz with a Pro Mini so I dropped the SPI to 1MHz to get a clean clk square wave. 

src/NMEA2000-library.patch The memory savings (see NMEA2000_CompilerDefns.h) which are required to run on a 382p dont complie without this patch. Additionally there I needed feedback on serial of failed packets without enabling all packet output.

Both patches must be applied if pulling fresh dependencies. 

# Memory usage

Building .pio/build/pro16MHzatmega328/firmware.hex
Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
RAM:   [==        ]  18.4% (used 377 bytes from 2048 bytes)
Flash: [========= ]  89.4% (used 27458 bytes from 30720 bytes)

If debugging of the sensors is enabled, then NMEA2000 output may have to be disabled to get the code to fit into FLASH. No problems seen with RAM at the moment, but it hasnt been tested on a live NMEA2000 bus yet.




# Todo/Status Relay Board

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
[x] Test NCT readings,  - adjusted to 4.7K top R and rebuilt NTC table from base equations.
[x] Test coolant readings - tested with fake NTC sensor wiring, need full test on real sensor
[x] Test fuel level readings
[x] Test voltage readings
[x] Test coolant with variable voltage.
[x] Test RPM with signal generator
[x] Test NMEA2000 output
[ ] install
[ ] Test output on MFD



# Todo Status Mosfet board

[x] Simulate everything in LTSpice
[x] Layout PCB
[ ] Mill prototype
[ ] Order Silicon
[ ] Build prototype
[ ] Test on/off
[ ] Test other buttons and circuits, under load ideally.
[ ] Test warning curcuits
[ ] Test sensors
[ ] Test NMEA2000 output
[ ] Install
[ ] Test running
[ ] Order boards and build spares.