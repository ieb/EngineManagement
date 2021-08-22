# Engine Management 



In July 2021 my boat was hit by lightning. The MDI unit that manages the Engine, a Volvo Penta D2-40F failed badly leaving the starter motor engaged and nearly causing an engine fire.

To get back to my home port I rewired the engine with relays and switches, housed in a nunch box, which works perfectly. This project takes that further, replacing the standard MDI unit with a unit that performs the same functions but is
designed, so that when it fails, it has more chance of failing safely and not turning the starter motor, glow plugs, and everything else on. I never want to try and stop an engine fire devloping because of MOSFET shorting again. The design is based on what electronics survived the strike, what electronics failed safely and what electronics failed unsafely. There were some common patterns. Lighning is unpredictable. Anything silicon based will probably fail. Diodes, MOSFETS, TVS's and even capacitors fail sorted. Fuses blow, but not after transporting the surge. Inductors, coils, motors without capacitors survived. Surge proctection works, but only when it self distructs and is backed up by a fuse to prevent follow on damage.

The plan was to build a PCB which would use the existing Vplvo Penta buttons and Tacho. However the Tacho was found to be damaged internally with the CanH and CanL at supply and GND voltages. The rest of the Tacho seemed ok, however its internal protection to surges (capacitors to ground, line inductor) didnt help protect it. With 1 large custom chip controlling CanH and CanL its not repairable, and since these cost 500 GBP to replace I have reverted to planB, and simple switch and relay based approach. No J1939 as this isnt necessary and very simple analogue oil, charge and temperature alarms.

I might write a NMEA2000 interface later, but thats probably going to be in a different project using a Mega with enough ram to run the NMEA2000 code.

Information may help someone who has a working VP Taco.



# Sensors

The standard flywheel pickup produces 415Hz 20V pk2pk puses with sharp edges at idle and so can be fed through passive conditioning to procuse a 5V square wave. The pk2pk voltage rises with rpm as does frequency. The The standard oil pressure switch is pulled up to 5V to produce a signal when closed. The standard coolant temperature sensor energised with 8V through a 1K resistor to produce a maximum of 5V to the ADC through a 100K resistor to allow the internal diode protection in the ADC to limit the voltage should the sensor go open circuit.  Fuel level uses a 0-190R sensor. Excitation, AlternatorB+, Engine Battery and Service Battery are all monitored by spare ADC's

# Outputs

As mentioned the unit emits J1939 messages over a 5V CAN at 250Kbit for the existing Volvo Penta instruments. A RS485 isolated interface emits NMEA-0183 proprietary messages for onward processign and transmission onto the main NMEA2000 bus. That processing is done outside of the engine context. 

The Pro mini has insufficient digital IO pins to drive the necessary relays so a MCP23008 port extender is used to provide 8 IO ports to drive relays over i2c. 

# Connections.

The VP harness uses Deutch 6 way and 8 way connectors for all the existing connections except for the Glow plugs and Starter motor solenoid with are m6 studs. So the unit can be a plug replacement the same loom is being used, allowing reverting to a MDI unit should a future owner want to switch back.

# Power on/off

The standard VP on/off button normally turns the engine power on and off. This is acieved by powering the core of the pro-mini drawing 10mA. On button press the pripherals are turned on draing 170 - 200mA enabling other buttons. A dedicated one button bistable flip flop circuit could have been used, but when it failed the behaviour might not be as expected. If the arduino fails, the peripherals are expected to fail off. Most flip flops are biased to one side to be "off" when energised, but given how unpredictable damage is, something that fails off is safer.

# Paranoid ?

Perhaps this is all paranoia, and I had certainly not though it would matter, and assimed the MDI was fail safe, but when it did there was no warning given, and no time to make anything safe. Only very quick actions by the crew disabled the engine and prevented an elctrical fire that would have lost the boat. There are plenty of examples of others who were not so lucky, taking the evidence to the bottom with the boat.

# Feedback

Feedback on any part of the circuit or code is always welcome, especially the circiut. if you see a safer or better way of implementing something please open an issue.

#
Priority
6, 
0x00EE00 = 60928 == claim source address

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


[ ] Connect relays and test.
[ ] Install and test.