// Demo: NMEA2000 library. Send battery status to the bus.
//
//
//      In this example are shown ways to minimize the size and RAM usage using two techniques:
//        1) Moving data strictures to PROGMEM vs. using inline constantans when calling a function
//        2) Reducing the size of NMEA CAN buffer to the min needed.  Use caution with this, as some functions
//           (specifically fast packet Messages) require bigger buffer. 
//


#include <Arduino.h>

#define USE_MCP_CAN_CLOCK_SET 8
//#define ENABLE_NMEA2000 1
//#define DEBUG_NMEA2000 1
#define DEBUGGING 1


#ifdef DEBUGGING
#define DEBUG(x) Serial.print(x)
#define DEBUGLN(x) Serial.print(x)
#else
#define DEBUG(x) 
#define DEBUGLN(x) 
#endif



//#define N2k_CAN_INT_PIN 21
#include "NMEA2000_CAN.h"       // This will automatically choose right CAN library and create suitable NMEA2000 object
#include "N2kMessages.h"
#include <util/crc16.h>
#include <EEPROM.h>



extern void initRPM();
extern void setupNMEA2000();
extern void readEngineRPM();
extern void readCoolant();
extern void readVoltages();
extern void readFuelLevel();
extern void sendRapidEngineData();
extern void sendEngineData();
extern void sendVoltages();
extern void loadEngineHours();
extern void saveEngineHours();
extern void readNTC();




void setup() {
  Serial.begin(115200);
  Serial.println(F("Luna monitor start"));
  // ensure SCL and SDA dont power anything during startup.
  pinMode(3, INPUT); // RPM
  digitalWrite(3, LOW);
  pinMode(5, INPUT); // OIL PRESSURE SWITCH
  digitalWrite(5, LOW);
  initRPM();
  loadEngineHours();
#ifdef ENABLE_NMEA2000
  setupNMEA2000();
#endif

}

void loop() {
  readEngineRPM();
  readCoolant();
  readFuelLevel();
  readVoltages();
  readNTC();
#ifdef ENABLE_NMEA2000
  sendRapidEngineData();
  sendEngineData();
  sendVoltages();
  NMEA2000.ParseMessages();
#endif
  saveEngineHours();
}

union EngineHoursStorage {
  uint8_t engineHoursBytes[4];
  uint32_t engineHoursPeriods;
};
union CRCStorage {
  uint8_t crcBytes[2];
  uint16_t crc;
};

EngineHoursStorage engineHours;
uint32_t baseEngineHoursPeriods = 0;
//  engine periods to hours factor 15000/3600000 = 0.004166666667 
#define ENGINE_HOURS 0.004166666667*engineHours.engineHoursPeriods

void loadEngineHours() {
  uint16_t crc = 0;
  for (int i = 0; i < 4; i++) {    
    crc = _crc16_update(crc, EEPROM.read(i+2));
  }
  CRCStorage crcStore;
  crcStore.crcBytes[0] = EEPROM.read(0);
  crcStore.crcBytes[1] = EEPROM.read(1);
  if ( crcStore.crc == crc) {
    // data is valid
    for (int i = 0; i < 4; i++) {    
      engineHours.engineHoursBytes[i] = EEPROM.read(i+2);
    }
    baseEngineHoursPeriods = engineHours.engineHoursPeriods;
  }
  Serial.print(F("Hours: "));
  Serial.println(ENGINE_HOURS);
}

#define ENGINE_HOURS_PERIOD_MS 15000
void saveEngineHours() {
  // the EMS will only be on for a very short time before the engine is started,
  // so assume the engine hours is the runtime of the EMS.
  uint32_t engineHoursPeriods = baseEngineHoursPeriods+millis()/ENGINE_HOURS_PERIOD_MS;
  if ( engineHoursPeriods > engineHours.engineHoursPeriods ) {
    // minutes has changed.
    engineHours.engineHoursPeriods = engineHoursPeriods;
    CRCStorage crcStore;
    crcStore.crc = 0;
    for (int i = 0; i < 4; i++) {    
      crcStore.crc = _crc16_update(crcStore.crc, engineHours.engineHoursBytes[i]);
    }
    EEPROM.write(0, crcStore.crcBytes[0]);
    EEPROM.write(1, crcStore.crcBytes[1]);
    for (int i = 0; i < 4; i++) {
      EEPROM.write(i+2, engineHours.engineHoursBytes[i]);
    }
  }
}




#define PINS_FLYWHEEL 2
volatile unsigned long flywheelPulses = 0;

void flywheelPuseHandler() {
  flywheelPulses++;
}

void initRPM() {
  pinMode(PINS_FLYWHEEL, INPUT);
  attachInterrupt(digitalPinToInterrupt(PINS_FLYWHEEL), flywheelPuseHandler, RISING);
}

int engineRPM = 0;
#define FLYWHEEL_READ_PERIOD 500
// pulses  Perms -> RPM conversion.
// 415Hz at idle below needs adjusting.
// reading every 500ms, RPM at 900 with say 50 notche, 15*50 Hz, 750 Hz
// pps == pulses*1000/timeInMs
// rps = pps/teeth
// rpm = rps/60
// 1000/(teeth*60)
// 20 teeth means 1000/(20*60)
#define FLYWHEEL_CONVERSION_U 1000
#define FLYWHEEL_CONVERSION_L 1200 //20*60, adjust once we know the number of pulses per revolution.
void readEngineRPM() {
  static unsigned long lastFlywheelReadTime = 0;
  static unsigned long lastFlywheelPulses = 0;
  unsigned long now = millis();
  if ( now > lastFlywheelReadTime+FLYWHEEL_READ_PERIOD ) {
    unsigned long measuredFlywheelPulses = flywheelPulses;
    engineRPM = ((measuredFlywheelPulses-lastFlywheelPulses)*FLYWHEEL_CONVERSION_U)/((now-lastFlywheelReadTime)*FLYWHEEL_CONVERSION_L);
    lastFlywheelReadTime = now;
    lastFlywheelPulses = measuredFlywheelPulses;
    Serial.print(F("RPM:"));
    Serial.println(engineRPM);
 
  }
}


#define FUEL_RESISTOR 1000
#define FUEL_VOLTAGE_ADC 16384 // 8*1024/5 = 1638.4, multply voltages by 10
#define ADC_FUEL_SENSOR 0
#define FUEL_SENSOR_MAX 190
#define FUEL_READ_PERIOD 10000

// Fuel sensor goes 0-190, 0 being full, 190 being empty.
int fuelLevel = 0;
void readFuelLevel() {
  static unsigned long lastFuelReadTime = 0;
  unsigned long now = millis();
  if ( now > lastFuelReadTime+FUEL_READ_PERIOD ) {
    lastFuelReadTime = now;
    // probably need a long to do this calc
    long fuelReading = analogRead(ADC_FUEL_SENSOR);
    // relationship betweem reading and level can be described mathematically, so use that
    // (8*R)/(R+100) = V whats R ?
    // 8*R = V*(R+100)
    // 8*R = V*R + V*100
    // 8*R - V*R = V*100
    // R*(8-V) = V*100
    // R = V*100/(8-V)
    // 
    // We multiply the top line by 10 so the voltage on the divider can be expressed as an long FUEL_VOLTAGE_ADC in 10xADC uints.
    // We multiple the top line by 100 to give percentage as an long
    long fuelLevelLong = (fuelReading*10*FUEL_RESISTOR*100)/(FUEL_VOLTAGE_ADC-fuelReading)*FUEL_SENSOR_MAX;
    // the restances may be out of spec so deal with > 100 or < 0.
    if (fuelLevelLong > 100 ) {
      fuelLevelLong = 100;
    } else if ( fuelLevelLong < 0) {
      fuelLevelLong = 0;
    }
    fuelLevel = 100-fuelLevelLong;
    Serial.print(F("Fuel:"));
    Serial.println(fuelLevel);
  }
}

/**
 *  convert a reading from a ntc into a temperature using a curve.
 */ 
// tested ok 20210909
int16_t interpolate(
      int16_t reading, 
      int16_t minCurveValue, 
      int16_t maxCurveValue,
      int step, 
      const int16_t *curve, 
      int curveLength
      ) {
  if ( reading > curve[0] ) {
    DEBUG(minCurveValue);
    DEBUG(",");
    return minCurveValue;
  }
  for (int i = 1; i < curveLength; i++) {
    if ( reading > curve[i] ) {
      DEBUG(i);
      DEBUG(",");
      DEBUG(curve[i]);
      DEBUG(",");
      DEBUG(curve[i-1]);
      DEBUG(",");
      return minCurveValue+((i-1)*step)+((curve[i-1]-reading)*step)/(curve[i-1]-curve[i]);
    }
  }
  DEBUG(minCurveValue);
  DEBUG("^,");
  return maxCurveValue;
}


/*
Thermistor temperature curve from Volvo Penta workshop manual is below.


Assume a 12V supply, but also measure it and adjust based on the current reading by scaling the reading.

12*700/(700+1000)=4.9, so min temperature that can be read is about 18C for a 12V supply.
12/(22+1000)=11mA max current through temperature sensor.

Coolant supply voltage uses a 100K/470K divider so 12V == 12*470/(1470) = 3.836V
ADC reading at 12V = 1024*(12*470/(1470))/5 = 786
ADC reading at 4v = 1024*(4*470/(1470))/5 = 261

Scaling of coolant reading to 12V = CoolantReading*786/SupplyVoltage.

The reason we are doing this is so that the relay board alarms can operate with no 
regulated 8V supply from the NMEA200 board, which may fail. If the coolant supply was
8v regulated it would depend on the MCU supply voltage (and the 8V regulator)

Model	Volvo Penta Standard Coolant Sensor				
R top	1000				
Vsupply	12				
					
					
					
					
C	R	V	ADC	Current (mA)	Power (mW)
0	1743	7.625227853	1562	4.374772147	33.35863443
10	1076	6.219653179	1274	5.780346821	35.95175248
20	677	4.844364937	992	7.155635063	34.6645076
30	439	3.660875608	750	8.339124392	30.52849708
40	291	2.704879938	554	9.295120062	25.14218378
50	197	1.974937343	404	10.02506266	19.79887061
60	134	1.417989418	290	10.58201058	15.00517903
70	97	1.061075661	217	10.93892434	11.60702637
80	70	0.785046729	161	11.21495327	8.804262381
90	51	0.582302569	119	11.41769743	6.648554546
100	38	0.4393063584	90	11.56069364	5.078686224
110	29	0.3381924198	69	11.66180758	3.943934925
120	22	0.2583170254	53	11.74168297	3.03307662*/

// ADC readings at 8V supply voltage, 120 to 0 in steps of 10C


const int16_t coolantTable[] = {
  1274,
  992,
  750,
  554,
  404,
  290,
  217,
  161,
  119,
  90,
  69,
  53
  };
#define COOLANT_TABLE_LENGTH 12
#define COOLANT_MIN_TEMPERATURE 10
#define COOLANT_MAX_TEMPERATURE 120
#define COOLANT_STEP 10


#define ADC_COOLANT_SENSOR 1
#define ADC_COOLANT_SUPPLY 7
#define COOLANT_SUPPLY_ADC_12V 786
#define MIN_SUPPLY_VOLTAGE 261
#define COOLANT_READ_PERIOD 3000
// C in degrees, not interested in 0.1 of a degree.
int coolantTemperature;

void readCoolant() {
    static unsigned long lastCoolantReadTime = 0;
  // probably need a long to do this calc
  unsigned long now = millis();
  if ( now > lastCoolantReadTime+COOLANT_READ_PERIOD ) {
    lastCoolantReadTime = now;
    // need 32 bits, 1024*1024 = 1M
    Serial.print(F("Coolant:"));
    int32_t coolantSupply = (int32_t) analogRead(ADC_COOLANT_SUPPLY);
    int32_t coolantReading = (int32_t)analogRead(ADC_COOLANT_SENSOR);
    if ( coolantSupply < MIN_SUPPLY_VOLTAGE) {
      Serial.println(F("no power"));
      coolantTemperature = 0;
      return;
    }
    Serial.print(coolantSupply);
    Serial.print(",");
    Serial.print(coolantReading);
    Serial.print(",");
    coolantReading = (coolantReading * coolantSupply)/COOLANT_SUPPLY_ADC_12V;
    Serial.print(coolantReading);
    Serial.print(",");
    coolantTemperature = interpolate(coolantReading,COOLANT_MIN_TEMPERATURE, COOLANT_MAX_TEMPERATURE, COOLANT_STEP, coolantTable, COOLANT_TABLE_LENGTH);
    Serial.println(coolantTemperature);
  }
}

#define START_ADC 5
#define N_ADCS 3
// stored as voltage
double voltages[3];
// to scale the voltage V =  V*5*(100+47)/(47*1024);
// to keep the calculation integer split
#define VOLTAGE_SCALE 0.01527177527 // 5*(100+47)/(47*1024)
#define VOLTAGE_READ_PERIOD 5000

#define ENGINE_BATTERY_VOLTAGE 0
#define SERVICE_BATTERY_VOLTAGE 1
#define ALTERNATOR_VOLTAGE 2


void readVoltages() {
  static unsigned long lastVotageRead = 0;
  // probably need a long to do this calc
  unsigned long now = millis();
  if ( now > lastVotageRead+VOLTAGE_READ_PERIOD ) {
    lastVotageRead = now;
    Serial.print(F("Voltage"));
    for (int i = 0; i < N_ADCS; i++) {
      voltages[i] = VOLTAGE_SCALE*analogRead(i+START_ADC);
      Serial.print(",");
      Serial.print(voltages[i]);
    }
    Serial.println(" ");
  }
}

/*
https://docs.google.com/spreadsheets/d/1cH6MjYFLKQYQMPswFqU3-U8cdn9rH_bujdkhxfRw2cY/edit#gid=1843834819
Using https://en.wikipedia.org/wiki/Thermistor
Model	MT52-10K		Voltage	5		
B	3950		Top R	4700		
To	298.15		Max Current	1.063829787	mA	
Ro	10000		Max allowed power	50		
rinf	0.01763226979					
C	K	R	V	ADC	current (mA)	Power (mW)
-20	253.15	105384.6902	4.786527991	980	0.04541957642	0.2174020739
-15	258.15	77898.10758	4.71548985	966	0.06053407453	0.285447814
-10	263.15	58245.71279	4.626662421	948	0.07943352738	0.3675121161
-5	268.15	44026.04669	4.517711746	925	0.1026145222	0.4635828322
0	273.15	33620.60372	4.386752877	898	0.1304781114	0.5723752304
5	278.15	25924.56242	4.232642097	867	0.1632676389	0.6910534817
10	283.15	20174.57702	4.055260317	831	0.2010084431	0.8151415627
15	288.15	15837.14766	3.855732043	790	0.2434612674	0.9387214101
20	293.15	12535.32581	3.636521279	745	0.2901018556	1.054961571
25	298.15	10000	3.401360544	697	0.3401360544	1.156925355
30	303.15	8037.140687	3.155001929	646	0.3925527811	1.238504782
35	308.15	6505.531126	2.902821407	594	0.4462082113	1.295262748
40	313.15	5301.466859	2.650344661	543	0.4999266678	1.324977975
45	318.15	4348.13685	2.40278022	492	0.5525999532	1.327776237
50	323.15	3588.182582	2.164637752	443	0.6032685635	1.305857907
55	328.15	2978.435896	1.939480863	397	0.6511742844	1.262940063
60	333.15	2486.164751	1.72982727	354	0.6957814318	1.203581695
65	338.15	2086.372143	1.537177817	315	0.7367706772	1.132547541
70	343.15	1759.837009	1.362137316	279	0.7740133371	1.054312449
75	348.15	1491.682246	1.204585593	247	0.8075349802	0.9727450031
80	353.15	1270.320235	1.063862728	218	0.8374760152	0.8909595185
85	358.15	1086.670769	0.9389429714	192	0.8640546869	0.8112980752
90	363.15	933.5770578	0.828582842	170	0.8875355655	0.7353967412
95	368.15	805.3667326	0.7314378603	150	0.9082047106	0.6642953102
100	373.15	697.519773	0.6461484185	132	0.9263514003	0.5985604923
105	378.15	606.4157635	0.5713986526	117	0.9422556058	0.5384035836
110	383.15	529.1404013	0.5059535226	104	0.9561801016	0.4837826907
115	388.15	463.3365226	0.4486793767	92	0.9683660901	0.4344858937
120	393.15	407.0887766	0.398552673	82	0.9790313462	0.39019556
125	398.15	358.8338772	0.3546606648	73	0.9883700713	0.3505359866
130	403.15	317.290402	0.3161969675	65	0.9965538367	0.3151073011
135	408.15	281.403613	0.2824541383	58	1.003733162	0.2835085854
140	413.15	250.301877	0.2528147608	52	1.010039413	0.2553528725
145	418.15	223.2620904	0.2267420323	46	1.015586802	0.2302762154
*/

const int16_t tcurveNMF5210K[] = {
    980,
    966,
    948,
    925,
    898,
    867,
    831,
    790,
    745,
    697,
    646,
    594,
    543,
    492,
    443,
    397,
    354,
    315,
    279,
    247,
    218,
    192,
    170,
    150,
    132,
    117,
    104,
    92,
    82,
    73,
    65,
    58,
    52,
    46
};
// in 0.1C steps.
#define NMF5210K_MIN -200
#define NMF5210K_MAX 1450
#define NMF5210K_STEP 50
#define NMF5210K_LENGTH 33
// ADC value that indicates a NTC is not connected.
#define DISCONNECTED_NTC 1000






#define START_NTC 2
#define N_NTC 3
#define NTC_READ_PERIOD 10000
// degrees Cx10
int16_t temperature[3];
// tested ok 20210909
void readNTC() {
  static unsigned long lastNTCRead = 0;
  // probably need a long to do this calc
  unsigned long now = millis();
  if ( now > lastNTCRead+NTC_READ_PERIOD ) {
    lastNTCRead = now;
    DEBUGLN(F("Temp"));
    for (int i = 0; i < N_NTC; i++) {
      DEBUG(i);
      DEBUG(",");
      int ntcReading = analogRead(i+START_NTC);
      if ( ntcReading > DISCONNECTED_NTC ) {
        DEBUGLN("disconnected");
        temperature[i] = 0;
      } else {
        DEBUG(ntcReading);
        DEBUG(",");
        DEBUG(5.0*ntcReading/1024.0);
        DEBUG(",");
        temperature[i] = interpolate(ntcReading, NMF5210K_MIN, NMF5210K_MAX, NMF5210K_STEP, tcurveNMF5210K, NMF5210K_LENGTH);
        DEBUGLN(temperature[i]);

      }
    }
  }
}








#define NMEA2000_DEV_ENGINE      0
#define NMEA2000_DEV_BATTERIES   1

const tNMEA2000::tProductInformation EMSProductInformation PROGMEM={
                                       1300,                        // N2kVersion
                                       100,                         // Manufacturer's product code
                                       "EMS",    // Manufacturer's Model ID
                                       "1.1",     // Manufacturer's Software version code
                                       "1.1",      // Manufacturer's Model version
                                       "1",                  // Manufacturer's Model serial code
                                       0,                           // SertificationLevel
                                       1                            // LoadEquivalency
                                      };                                      

const tNMEA2000::tProductInformation BatteriesProductInformation PROGMEM={
                                       1300,                        // N2kVersion
                                       100,                         // Manufacturer's product code
                                       "BMS",    // Manufacturer's Model ID
                                       "1.1",     // Manufacturer's Software version code
                                       "1.1",      // Manufacturer's Model version
                                       "1",                  // Manufacturer's Model serial code
                                       0,                           // SertificationLevel
                                       1                            // LoadEquivalency
                                      };                                      

const char EMSManufacturerInformation [] PROGMEM = "me"; 
const char EMSInstallationDescription1 [] PROGMEM = "EMS"; 
const char EMSInstallationDescription2 [] PROGMEM = "NA"; 


void setupNMEA2000() {


  NMEA2000.SetDeviceCount(2);
  // Set Configuration information
  NMEA2000.SetProgmemConfigurationInformation(EMSManufacturerInformation,EMSInstallationDescription1,EMSInstallationDescription2);

  // Set Product information
  NMEA2000.SetProductInformation(&EMSProductInformation, NMEA2000_DEV_ENGINE );
  NMEA2000.SetProductInformation(&BatteriesProductInformation, NMEA2000_DEV_BATTERIES );


  NMEA2000.SetDeviceInformation(3, // Unique number. Use e.g. Serial number.
                                160, // Device function=Engine Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                50, // Device class=Propulsion. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                2085, // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf                               
                                4, // Marine
                                NMEA2000_DEV_ENGINE
                                );
  NMEA2000.SetDeviceInformation(4, // Unique number. Use e.g. Serial number.
                                170, // Device function=Engine Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                35, // Device class=Propulsion. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                2085, // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf                               
                                4, // Marine
                                NMEA2000_DEV_BATTERIES
                                );

  // debugging with no chips connected.
#ifdef DEBUG_NMEA2000
  NMEA2000.SetForwardStream(&Serial);
  NMEA2000.SetDebugMode(tNMEA2000::dm_ClearText);
  NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); 
  NMEA2000.SetDebugMode(tNMEA2000::dm_ClearText); // Uncomment this, so you can test code without CAN bus chips on Arduino Mega
#endif

  // this is a node, we are not that interested in other traffic on the bus.
  NMEA2000.SetMode(tNMEA2000::N2km_NodeOnly,23);
  NMEA2000.EnableForward(false);
  const unsigned long engineMessages[] = {  
    127488L, // Rapid engine ideally 0.1s
    127489L, // Dynamic engine 0.5s
    127505L, // Tank Level 2.5s
    130316L, // Extended Temperature 2.5s
  0};
  NMEA2000.ExtendTransmitMessages(engineMessages,NMEA2000_DEV_ENGINE);
  const unsigned long batteryMessages[] = {  127508L,0L };
  NMEA2000.ExtendTransmitMessages(batteryMessages,NMEA2000_DEV_BATTERIES);
  NMEA2000.SetN2kCANMsgBufSize(2);
  NMEA2000.SetN2kCANSendFrameBufSize(15);
  NMEA2000.Open();
}


#define RAPID_ENGINE_UPDATE_PERIOD 1000

void sendRapidEngineData() {
  static unsigned long lastRapidEngineUpdate=0;
  unsigned long now = millis();
  if ( now > lastRapidEngineUpdate+RAPID_ENGINE_UPDATE_PERIOD ) {
    lastRapidEngineUpdate = now;
    // PGN127488
    tN2kMsg N2kMsg;
    SetN2kEngineParamRapid(N2kMsg, 0, engineRPM);
    NMEA2000.SendMsg(N2kMsg, NMEA2000_DEV_ENGINE);
  }
}

#define ENGINE_UPDATE_PERIOD 10000

void sendEngineData() {
  static unsigned long lastEngineUpdate=0;
  unsigned long now = millis();
  if ( now > lastEngineUpdate+ENGINE_UPDATE_PERIOD ) {
    lastEngineUpdate = now;
    // PGN127489

    tN2kMsg N2kMsg;
    SetN2kEngineDynamicParam(N2kMsg, 0, N2kDoubleNA, N2kDoubleNA, CToKelvin(coolantTemperature), voltages[ALTERNATOR_VOLTAGE],
                      N2kDoubleNA, ENGINE_HOURS, N2kDoubleNA, N2kDoubleNA, N2kInt8NA, N2kInt8NA,0,0);
    NMEA2000.SendMsg(N2kMsg, NMEA2000_DEV_ENGINE);

    // always send the fuel level data.
    SetN2kFluidLevel(N2kMsg,0,N2kft_Fuel,fuelLevel,60);
    NMEA2000.SendMsg(N2kMsg, NMEA2000_DEV_ENGINE);
  }
}



#define VOLTAGE_UPDATE_PERIOD 5000

void sendVoltages() {
  static unsigned long lastVoltageUpdate=0;
  unsigned long now = millis();
  if ( now > lastVoltageUpdate+VOLTAGE_UPDATE_PERIOD ) {
    lastVoltageUpdate = now;



    tN2kMsg N2kMsg;
    SetN2kDCBatStatus(N2kMsg,0, voltages[SERVICE_BATTERY_VOLTAGE], N2kDoubleNA, N2kDoubleNA, 0);
    NMEA2000.SendMsg(N2kMsg, NMEA2000_DEV_BATTERIES);
    SetN2kDCBatStatus(N2kMsg,1, voltages[ENGINE_BATTERY_VOLTAGE], N2kDoubleNA, N2kDoubleNA, 1);
    NMEA2000.SendMsg(N2kMsg, NMEA2000_DEV_BATTERIES);
    // send the Alternator information as if it was a 3rd battery, becuase there is no other way.
    SetN2kDCBatStatus(N2kMsg,2, voltages[ALTERNATOR_VOLTAGE], N2kDoubleNA, N2kDoubleNA, 2);
    NMEA2000.SendMsg(N2kMsg, NMEA2000_DEV_BATTERIES);
  }
}


