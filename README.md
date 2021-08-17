# Engine Management 

In July 2021 my boat was hit by lightning. The MDI unit that manages the Engine, a Volvo Penta D2-40F failed badly leaving the starter motor engaged and nearly causing an engine fire.

To back to my home port I rewired the engine with relays and switches, which works perfectly. This project takes that further, replading the standard MDI unit with a unit that performs the same functions but is
designed, so that when it fails, it has more chance of failing safely and not turning the starter motor, glow plugs, and everything else on. The design is based on what electronics survived the strike, what electronics failed safely and what electronics failed unsafely. There were some common patterns. Lighning is unpredictable. Anything silicon based will probably fail. Diodes, MOSFETS, TVS's and even capacitors fail sorted. Fuses blow, but not after arcing. Inductors, coils, motors without capacitors survived. Surge proctection works, but only when it self distructs and is backed up by a fuse to prevent follow on damage.

The PCB and code is designed to work with the existing wiring loom and buttons, as well as the existing Volvo Penta Tachometer and engine sensors.


# Protection

The aim is to fail safe, not to survive a surge from a strike or pulse. There is no safe way of testing so the approach is all guesswork and theory based on damage done by the strike in July. The supply is protected by 600W 32V TVS diodes which fuse closed circuit on failure. This blows a 50A fuse supplying the starter motor solenoid and relays. A second TSV+5A fuse protects the more sensitive electronics. This setup worked at protecting some sensitive electronics in July. The fuses blew, and once the TSVs were replaced the board came back to life with no harm.

The board is energised through a relay controlled by an Arduino Pro Mini. The relays default to open circuit and with a blown or disabled Arduino that power to the relay controls and relay coils is disconnected. There is a risk that the contacts arc solid closed, however this should blow the TSV and fuse.

# Controls and feedback.

The existing Volvo Penta buttons and Tachometer appear ok and will be used. The buttons are simple membrane contacts, probably able to handle no more than 30mA to ground when pressed. These are pulled up to 5V with 2.2K resisitors. The pullup is strong to deal with the long wires and potential noise levels.

The interface to the Taco is a CAN Bus interface talkign J1939. The control unit will send J1939 messages on through a MCP2515 CAN Tranduce and driver, simulating the messages from the MDI. 

In the vent of failure, manual buttons for power, start, stop and glow that go direct to the relay coils can be used, bypassing all electronics. This mirrors the get-home setup.

A 20x4 LCD shows information and status.

# Sensors

The standard flywheel pickup produces 415Hz 20V pk puses with sharp edges at idle and so can be fed through passive conditioning to procuse a 5V square wave. The standard oil pressure switch is pulled up to 5V to produce a signal when closed. The standard coolant temperature sensor energised with 8V through a 1K resistor to produce a maximum of 5V to the ADC through a 100K resistor to allow the internal diode protection in the ADC to limit the voltage should the sensor go open circuit.  Fule level uses a 0-190R sensor. Excitation, AlternatorB+, Engine Battery and Service Battery are all monitored.

# Outputs

As mentioned the unit emits J1939 messages over a 5V CAN at 250Kbit for the existing Volvo Penta instruments. A RS485 isolated interface emits NMEA-0183 proprietary messages for onward processign and transmission onto the main NMEA2000 bus. That processing is done outside of the engine context. 

# Connections.

The VP harness uses Deutch 6 way and 8 way connectors for all the existing connections except for the Glow plugs and Starter motor solenoid with are m6 studs. So the unit can be a plug replacement the same loom is being used, allowing reverting to a MDI unit should a future owner want to switch back.

# Feedback

Feedback on any part of the circuit or code is always welcome, especially the circiut. if you see a safer or better way of implementing something please open an issue.

# Todo/Status

[x] Build prototype PCB and populate.
[x] Test on/off sequence.
[x] Test 20x4 I2C display.
[ ] Test port extender.
[ ] Connect 8 way relay board and test.
[ ] Connect test harness sensors and test.
[ ] Connect VP Taco and test with simulated engine.
[ ] Connect relays and test.
[ ] Install and test.