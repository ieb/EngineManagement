// Demo: NMEA2000 library. Send battery status to the bus.
//
//
//      In this example are shown ways to minimize the size and RAM usage using two techniques:
//        1) Moving data strictures to PROGMEM vs. using inline constantans when calling a function
//        2) Reducing the size of NMEA CAN buffer to the min needed.  Use caution with this, as some functions
//           (specifically fast packet Messages) require bigger buffer. 
//


#include <Arduino.h>
#include <MemoryFree.h>

#define USE_MCP_CAN_CLOCK_SET 8
#define ENABLE_NMEA2000 1
// no chips
//#define DEBUG_NMEA2000 1 
// with chips forwarding to serial
//#define INFO_NMEA2000 1
//#define DEBUGGING 1
//#define INFOMSG 1


#ifdef DEBUGGING
#define DEBUG(x) Serial.print(x)
#define DEBUGLN(x) Serial.println(x)
#else
#define DEBUG(x) 
#define DEBUGLN(x) 
#endif

#ifdef INFOMSG
#define INFO(x) Serial.print(x)
#define INFOLN(x) Serial.println(x)
#else
#define INFO(x) 
#define INFOLN(x) 
#endif


// define the CS pin as the libraries assume otherwise.
#define USE_SNMEA 1
#ifdef USE_SNMEA
#define SNMEA_SPI_CS_PIN 10
#include "SmallNMEA2000.h"
#define CToKelvin(x) x+283.15
#else
#define N2k_SPI_CS_PIN 10
#include "NMEA2000_CAN.h"      
#include "N2kMessages.h"
#endif
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
extern void sendTemperatures();
extern void sendFuel();
extern void processMessages();



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
  Serial.println(F("Running..."));

}

void loop() {
  readEngineRPM();
  readCoolant();
  readFuelLevel();
  readVoltages();
  readNTC();
#ifdef ENABLE_NMEA2000
  //sendRapidEngineData();
  //sendEngineData();
  //sendVoltages();
  //sendTemperatures();
  sendFuel();
  processMessages();
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
volatile int8_t pulseCount = 0;
volatile unsigned long lastPulse=0;
volatile unsigned long thisPulse=0;
// record the time take for 10 pulses in us using micros, Should be fine 
// for < 50KHz which would read 200uS.
// 50Hz will be 200000.
void flywheelPuseHandler() {
  pulseCount++;
  if ( pulseCount == 10 ) {
    lastPulse = thisPulse;
    thisPulse = micros();
    pulseCount = 0;
  }
}

double calculateFrequency() {
  noInterrupts();
  unsigned long lastP = lastPulse;
  unsigned long thisP = thisPulse;
  interrupts();
  unsigned long period = thisP - lastP;
  if ( (period == 0) || (micros() - thisP) > 1000000 ) {
    // no pulses, > 10MHz or < 10Hz, assume not running.
    return 0;
  }
  INFOLN(period);
  // 10 pulses.
  // 1 micro for 10 pulses would be 10MHz
  // unsigned long
  return 10000000.0/period;
}

void initRPM() {
  pinMode(PINS_FLYWHEEL, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PINS_FLYWHEEL), flywheelPuseHandler, FALLING);
  DEBUG(F("ISR enabled"));

}

double engineRPM = 0;
#define FLYWHEEL_READ_PERIOD 2000
// pulses  Perms -> RPM conversion.
// 415Hz at idle below needs adjusting.//
// 415Hz == 850 rpm
// 
// pulses*850*1000/(duration)*415
#define RPM_FACTOR 2.054 // 2048.1927711 // 850000/415


void readEngineRPM() {
  static unsigned long lastFlywheelReadTime = 0;
  unsigned long now = millis();
  if ( now > lastFlywheelReadTime+FLYWHEEL_READ_PERIOD ) {
    lastFlywheelReadTime = now;
    double frequency = calculateFrequency();
    INFO(F("RPM:"));
    INFO(frequency);
    INFO(",");
    engineRPM = (RPM_FACTOR*frequency);
    INFOLN(engineRPM); 
  }
}


#define FUEL_RESISTOR 1000
#define ADC_READING_EMPTY 263 // measured for 190R
#define ADC_FUEL_SENSOR 1
#define FUEL_SENSOR_MAX 190
#define FUEL_READ_PERIOD 10000

// Fuel sensor goes 0-190, 0 being full, 190 being empty.
// tested ok.
int fuelLevel = 0;
void readFuelLevel() {
  static unsigned long lastFuelReadTime = 0;
  unsigned long now = millis();
  if ( now > lastFuelReadTime+FUEL_READ_PERIOD ) {
    lastFuelReadTime = now;
    // probably need a long to do this calc
    INFO(F("Fuel:"));
    int16_t fuelReading = analogRead(ADC_FUEL_SENSOR);
    INFO(fuelReading);
    INFO(",");
    // Rtop = 1000
    // Rempty = 190
    // Rfull = 0
    // ADCempty = 8*190/(190+1000) =  1.277 = 1024*1.277/5 = 261
    // Measured at 263

    // ADCfull = 0 = 0v 0
    fuelReading  = 100-(100*(ADC_READING_EMPTY-fuelReading))/(ADC_READING_EMPTY);
    // the restances may be out of spec so deal with > 100 or < 0.
    if (fuelReading > 100 ) {
      fuelLevel = 100;
    } else if ( fuelReading < 0) {
      fuelLevel = 0;
    } else {
      fuelLevel = fuelReading;
    }
    INFOLN(fuelLevel);
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
  int16_t cvp = ((int16_t)pgm_read_dword(&curve[0]));
  if ( reading > cvp ) {
    DEBUG(minCurveValue);
    DEBUG(",");
    return minCurveValue;
  }
  for (int i = 1; i < curveLength; i++) {
    int16_t cv = ((int16_t)pgm_read_dword(&curve[i]));
    if ( reading > cv ) {
      DEBUG(i);
      DEBUG(",");
      DEBUG(curve[i]);
      DEBUG(",");
      DEBUG(curve[i-1]);
      DEBUG(",");
      return minCurveValue+((i-1)*step)+((cvp-reading)*step)/(cvp-cv);
    }
    cvp = cv;
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


const int16_t coolantTable[] PROGMEM= {
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


#define ADC_COOLANT_SENSOR 0
#define ADC_COOLANT_SUPPLY 7
#define COOLANT_SUPPLY_ADC_12V 780 // calibrated
#define MIN_SUPPLY_VOLTAGE 261
#define COOLANT_READ_PERIOD 1000
// C in degrees, not interested in 0.1 of a degree.
int coolantTemperature;

void readCoolant() {
    static unsigned long lastCoolantReadTime = 0;
  // probably need a long to do this calc
  unsigned long now = millis();
  if ( now > lastCoolantReadTime+COOLANT_READ_PERIOD ) {
    lastCoolantReadTime = now;
    // need 32 bits, 1024*1024 = 1M
    DEBUG(F("Coolant:"));
    int32_t coolantSupply = (int32_t) analogRead(ADC_COOLANT_SUPPLY);
    int32_t coolantReading = (int32_t)analogRead(ADC_COOLANT_SENSOR);

//    // fake up 12v supply to testing purposes
//    coolantSupply = COOLANT_SUPPLY_ADC_12V;
    
    
    
    if ( coolantSupply < MIN_SUPPLY_VOLTAGE) {
      DEBUGLN(F("no power"));
      coolantTemperature = 0;
      return;
    }
    DEBUG(coolantSupply);
    DEBUG(",");
    DEBUG(coolantReading);
    DEBUG(",");
    coolantReading = (coolantReading * COOLANT_SUPPLY_ADC_12V)/coolantSupply;
    DEBUG(coolantReading);
    DEBUG(",");
    coolantTemperature = interpolate(coolantReading,COOLANT_MIN_TEMPERATURE, COOLANT_MAX_TEMPERATURE, COOLANT_STEP, coolantTable, COOLANT_TABLE_LENGTH);
    DEBUGLN(coolantTemperature);
  }
}

#define START_ADC 5
#define N_ADCS 3
// stored as voltage
double voltages[3];
// to scale the voltage V =  V*5*(100+47)/(47*1024);
// to keep the calculation integer split
#define VOLTAGE_SCALE 0.01537150931 // 5*(100+47)/(47*1024), calibrated for resistors.
#define VOLTAGE_READ_PERIOD 5000

#define ENGINE_BATTERY_VOLTAGE 2
#define SERVICE_BATTERY_VOLTAGE 1
#define ALTERNATOR_VOLTAGE 0

// tested ok and calibrated.
void readVoltages() {
  static unsigned long lastVotageRead = 0;
  // probably need a long to do this calc
  unsigned long now = millis();
  if ( now > lastVotageRead+VOLTAGE_READ_PERIOD ) {
    lastVotageRead = now;
    DEBUG(F("Voltage"));
    for (int i = 0; i < N_ADCS; i++) {
      voltages[i] = VOLTAGE_SCALE*analogRead(i+START_ADC);
      DEBUG(",");
      DEBUG(voltages[i]);
    }
    DEBUGLN(" ");
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

const int16_t tcurveNMF5210K[] PROGMEM= {
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





#define EXHAUST_TEMP 0
#define ALTERNATOR_TEMP 1
#define ENGINEROOM_TEMP 2

#define START_NTC 2
#define N_NTC 3
#define NTC_READ_PERIOD 2000
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






#ifdef USE_SNMEA
// These are the byte[] of the NMEA messages for ISO responses to the Product Information
// Configuration Information. They are sent as a fastpacket and only consume
// 8 bytes of ram at runtime to process.
// Doing it this way is harder, but avoids multiple buffer copies.


const SNMEA2000ProductInfo productInfomation PROGMEM={
                                       1300,                        // N2kVersion
                                       44,                         // Manufacturer's product code
                                       "EMS",    // Manufacturer's Model ID
                                       "1.1.0.14 (2017-06-11)",     // Manufacturer's Software version code
                                       "1.1.0.14 (2017-06-11)",      // Manufacturer's Model version
                                       "0000001",                  // Manufacturer's Model serial code
                                       0,                           // SertificationLevel
                                       1                            // LoadEquivalency
};
const SNMEA2000ConfigInfo configInfo PROGMEM={
      "Engine Monitor",
      "Luna Technical Area",
      "https://github.com/ieb/EngineManagement"
};


const unsigned long txPGN[] PROGMEM = { 
    127488L, // Rapid engine ideally 0.1s
    127489L, // Dynamic engine 0.5s
    127505L, // Tank Level 2.5s
    130316L, // Extended Temperature 2.5s
    127508L,
  SNMEA200_DEFAULT_TX_PGN
};
const unsigned long rxPGN[] PROGMEM = { 
  SNMEA200_DEFAULT_RX_PGN
};


EngineMonitor engineMonitor(24, 3, &productInfomation, &configInfo, &txPGN[0], &rxPGN[0]);

#else


#define NMEA2000_DEV_ENGINE      0
#define NMEA2000_DEV_BATTERIES   0


const tNMEA2000::tProductInformation EMSProductInformation PROGMEM={
                                       1300,                        // N2kVersion
                                       44,                         // Manufacturer's product code
                                       "EMS",    // Manufacturer's Model ID
                                       "1.1.0.14 (2017-06-11)",     // Manufacturer's Software version code
                                       "1.1.0.14 (2017-06-11)",      // Manufacturer's Model version
                                       "0000001",                  // Manufacturer's Model serial code
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
  const unsigned long engineMessages[] PROGMEM = {  
    127488L, // Rapid engine ideally 0.1s
    127489L, // Dynamic engine 0.5s
    127505L, // Tank Level 2.5s
    130316L, // Extended Temperature 2.5s
    127508L,
    0L
  };

const char EMSManufacturerInformation [] PROGMEM = "ieb"; 
const char EMSInstallationDescription1 [] PROGMEM = "EMS"; 
const char EMSInstallationDescription2 [] PROGMEM = "Luna"; 
#endif

void setupNMEA2000() {

#ifdef USE_SNMEA
  Serial.println(F("Opening CAN"));
  engineMonitor.open();
  Serial.println(F("Opened"));
#else


  NMEA2000.SetDeviceCount(1);
  // Set Configuration information
  NMEA2000.SetProgmemConfigurationInformation(EMSManufacturerInformation,EMSInstallationDescription1,EMSInstallationDescription2);

  // Set Product information
  NMEA2000.SetProductInformation(&EMSProductInformation, NMEA2000_DEV_ENGINE );
  //NMEA2000.SetProductInformation(&BatteriesProductInformation, NMEA2000_DEV_BATTERIES );


  NMEA2000.SetDeviceInformation(3, // Unique number. Use e.g. Serial number.
                                160, // Device function=Engine Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                50, // Device class=Propulsion. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                2085, // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf                               
                                4, // Marine
                                NMEA2000_DEV_ENGINE
                                );
  //NMEA2000.SetDeviceInformation(4, // Unique number. Use e.g. Serial number.
  //                              170, // Device function=Engine Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
  //                              35, // Device class=Propulsion. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
  //                              2085, // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf                               
  //                              4, // Marine
  //                              NMEA2000_DEV_BATTERIES
  //                              );

  // debugging with no chips connected
  //

  // this is a node, we are not that interested in other traffic on the bus.
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndSend,24);
#if defined(INFO_NMEA2000)
  NMEA2000.SetForwardStream(&Serial);
  NMEA2000.EnableForward(true);
  NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); 
#elif defined(DEBUG_NMEA2000)
  NMEA2000.SetForwardStream(&Serial);
  NMEA2000.SetDebugMode(tNMEA2000::dm_ClearText);
  NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); 
#else
  NMEA2000.EnableForward(false);
#endif
  NMEA2000.ExtendTransmitMessages(engineMessages,NMEA2000_DEV_ENGINE);
//  const unsigned long batteryMessages[] = {  127508L,0L };
//  NMEA2000.ExtendTransmitMessages(batteryMessages,NMEA2000_DEV_BATTERIES);
  NMEA2000.SetN2kCANMsgBufSize(2); // any more than 1 and there is no ram left
  NMEA2000.SetN2kCANSendFrameBufSize(20); // this must be 20 to send product info, 60 bytes left.
  Serial.println(F("Opening CAN"));
  NMEA2000.Open();
  Serial.println(F("Opened"));
#endif
}

void processMessages() {
#ifdef USE_SNMEA
  engineMonitor.processMessages();
#else 
  NMEA2000.ParseMessages();
#endif  
}



#define RAPID_ENGINE_UPDATE_PERIOD 100
int diff = 0;
void sendRapidEngineData() {
  static unsigned long lastRapidEngineUpdate=0;
  unsigned long now = millis();
  if ( now > lastRapidEngineUpdate+RAPID_ENGINE_UPDATE_PERIOD ) {
    diff = now-lastRapidEngineUpdate;
    diff = diff - RAPID_ENGINE_UPDATE_PERIOD;
    lastRapidEngineUpdate = now;
    // PGN127488

#ifdef USE_SNMEA
    engineMonitor.sendRapidEngineDataMessage(0,engineRPM);
#else
    tN2kMsg N2kMsg;
    SetN2kEngineParamRapid(N2kMsg, 0, engineRPM);
    NMEA2000.SendMsg(N2kMsg, NMEA2000_DEV_ENGINE);
#endif
  }
}



#define ENGINE_UPDATE_PERIOD 2000

void sendEngineData() {
  static unsigned long lastEngineUpdate=0;
  unsigned long now = millis();
  if ( now > lastEngineUpdate+ENGINE_UPDATE_PERIOD ) {
    lastEngineUpdate = now;
    // PGN127489

#ifdef USE_SNMEA
    engineMonitor.sendEngineDynamicParamMessage(0,
        ENGINE_HOURS,
        CToKelvin(coolantTemperature),
        voltages[ALTERNATOR_VOLTAGE]);
#else
    tN2kMsg N2kMsg;
    SetN2kEngineDynamicParam(N2kMsg, 0, N2kDoubleNA, N2kDoubleNA, CToKelvin(coolantTemperature), voltages[ALTERNATOR_VOLTAGE],
                      N2kDoubleNA, ENGINE_HOURS, N2kDoubleNA, N2kDoubleNA, N2kInt8NA, N2kInt8NA,0,0);
    NMEA2000.SendMsg(N2kMsg, NMEA2000_DEV_ENGINE);
#endif
    Serial.println(freeMemory());


  }
}




#define VOLTAGE_UPDATE_PERIOD 9996

void sendVoltages() {
  static unsigned long lastVoltageUpdate=0;
  unsigned long now = millis();
  if ( now > lastVoltageUpdate+VOLTAGE_UPDATE_PERIOD ) {
    lastVoltageUpdate = now;

#ifdef USE_SNMEA
    engineMonitor.sendDCBatterStatusMessage(0,0,voltages[SERVICE_BATTERY_VOLTAGE],CToKelvin(0.1*temperature[ENGINEROOM_TEMP]));
    engineMonitor.sendDCBatterStatusMessage(1,1,voltages[ENGINE_BATTERY_VOLTAGE],CToKelvin(0.1*temperature[ENGINEROOM_TEMP]));
    engineMonitor.sendDCBatterStatusMessage(2,3,voltages[ALTERNATOR_VOLTAGE],CToKelvin(0.1*temperature[ALTERNATOR_TEMP]));
#else


    tN2kMsg N2kMsg;
    SetN2kDCBatStatus(N2kMsg,0, voltages[SERVICE_BATTERY_VOLTAGE], N2kDoubleNA, CToKelvin(0.1*temperature[ALTERNATOR_TEMP]), 0);
    NMEA2000.SendMsg(N2kMsg, NMEA2000_DEV_BATTERIES);
    SetN2kDCBatStatus(N2kMsg,1, voltages[ENGINE_BATTERY_VOLTAGE], N2kDoubleNA, CToKelvin(0.1*temperature[ENGINEROOM_TEMP]), 1);
    NMEA2000.SendMsg(N2kMsg, NMEA2000_DEV_BATTERIES);
    // send the Alternator information as if it was a 3rd battery, becuase there is no other way.
    SetN2kDCBatStatus(N2kMsg,2, voltages[ALTERNATOR_VOLTAGE], N2kDoubleNA, CToKelvin(0.1*temperature[ALTERNATOR_TEMP]), 2);
    NMEA2000.SendMsg(N2kMsg, NMEA2000_DEV_BATTERIES);
#endif
  }
}




#define FUEL_UPDATE_PERIOD 10000

void sendFuel() {
  static unsigned long lastFuelUpdate=0;
  unsigned long now = millis();
  if ( now > lastFuelUpdate+FUEL_UPDATE_PERIOD ) {
    lastFuelUpdate = now;

#ifdef USE_SNMEA
  engineMonitor.sendFluidLevelMessage(0,0,fuelLevel,60);
#else

    tN2kMsg N2kMsg;
    // always send the fuel level data.
    SetN2kFluidLevel(N2kMsg,0,N2kft_Fuel,fuelLevel,60);
    NMEA2000.SendMsg(N2kMsg, NMEA2000_DEV_ENGINE);
#endif
  }
}

#define TEMPERATURE_UPDATE_PERIOD 4850

void sendTemperatures() {
  static unsigned long lastTempUpdate=0;
  static byte n =24;
  unsigned long now = millis();
  if ( now > lastTempUpdate+TEMPERATURE_UPDATE_PERIOD ) {
    lastTempUpdate = now;
    n++;
    if ( n == 25 ) {
      INFOLN(F("hours,rpm,Et,Av,Sv,Ev,Xt,At,Rt,d"));
      n = 0;
    }
    //char comma = ',';
    INFO(ENGINE_HOURS);
    INFO(comma);
    INFO(engineRPM);
    INFO(comma);
    INFO(coolantTemperature);
    INFO(comma);
    for(int i = 0; i < 3; i++) {
      INFO(voltages[i]);
      INFO(comma);
    }
    for(int i = 0; i < 3; i++) {
      INFO(temperature[i]);
      INFO(comma);
    }
    INFOLN(diff);


#ifdef USE_SNMEA
    engineMonitor.sendTemperatureMessage(0,0,14,CToKelvin(0.1*temperature[EXHAUST_TEMP]));
    engineMonitor.sendTemperatureMessage(1,1,3,CToKelvin(0.1*temperature[ENGINEROOM_TEMP]));
    engineMonitor.sendTemperatureMessage(2,3,15,CToKelvin(0.1*temperature[ALTERNATOR_TEMP]));
#else
    tN2kMsg N2kMsg;


    // send extended temperatures
    SetN2kTemperatureExt(N2kMsg, 1, 1, N2kts_ExhaustGasTemperature, CToKelvin(0.1*temperature[EXHAUST_TEMP]));
    NMEA2000.SendMsg(N2kMsg);
    SetN2kTemperatureExt(N2kMsg, 2, 2, N2kts_ExhaustGasTemperature, CToKelvin(0.1*temperature[ENGINEROOM_TEMP]));
    NMEA2000.SendMsg(N2kMsg);
    SetN2kTemperature(N2kMsg, 3, 3, N2kts_EngineRoomTemperature, CToKelvin(0.1*temperature[ALTERNATOR_TEMP]));
    NMEA2000.SendMsg(N2kMsg);
#endif
  }
}

