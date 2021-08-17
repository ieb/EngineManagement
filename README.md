# Engine Management 

In July 2021 my boat was hit by lightning. The MDI unit that manages the Engine, a Volvo Penta D2-40F failed badly leaving the starter motor engaged and nearly causing an engine fire.

To back to my home port I rewired the engine with relays and switches, which works perfectly. This project takes that further, replading the standard MDI unit with a unit that performs the same functions but is
designed, so that when it fails, it has more chance of failing safely and not turning the starter motor, glow plugs, and everything else on. The design is based on what electronics survived the strike, what electronics failed safely and what electronics failed unsafely. There were some common patterns. Lighning is unpredictable. Anything silicon based will probably fail. Diodes, MOSFETS, TVS's and even capacitors fail sorted. Fuses blow, but not after arcing. Inductors, coils, motors without capacitors survived. Surge proctection works, but only when it self distructs and is backed up by a fuse to prevent follow on damage.

The PCB and code is designed to work with the existing wiring loom and buttons, as well as the existing Volvo Penta Tachometer and engine sensors.

# Current status.
Work in Progress. Watch this space for updates.