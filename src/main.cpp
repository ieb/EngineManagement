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
//#define DEBUG_NMEA2000 1

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
  setupNMEA2000();
}

void loop() {
  readEngineRPM();
  readCoolant();
  readFuelLevel();
  readVoltages();
  readNTC();
  sendRapidEngineData();
  sendEngineData();
  sendVoltages();
  saveEngineHours();
  NMEA2000.ParseMessages();
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
}

#define ENGINE_HOURS_PERIOD_MS 15000
//  engine periods to hours factor 15000/3600000 = 0.004166666667 
#define ENGINE_HOURS 0.004166666667*engineHours.engineHoursPeriods
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
  }
}


/*
Thermistor temperature curve from Volvo Penta workshop manual is below.
supply V is 8V, top R is 1K
C   K   R2  ADC                 
C	  K	  R	  ADC Reading	ADC/C	      Resolution in C
120	393	22	35.26888454		
110	383	29	46.17453839	1.090565385	0.9169555664
100	373	38	59.97996146	1.380542308	0.7243530273
90	363	51	79.50371075	1.952374929	0.5121967022
80	353	70	107.1850467	2.768133598	0.3612542403
70	343	97	144.8721969	3.768715017	0.2653424298
60	333	134	193.6028219	4.873062497	0.205209763
50	323	197	269.6447786	7.604195674	0.1315063477
40	313	291	369.3062742	9.966149559	0.1003396542
30	303	439	499.8315497	13.05252755	0.07661351384
20	293	677	661.4172928	16.15857431	0.06188664797
10	283	1076	849.1899807	18.77726879	0.0532558814
0	273	1743	1041.097776	19.19077954	0.05210835744
*/

// ADC readings, 120 to 0 in steps of 10C
const int coolantTable[] = {35, 46, 60, 79, 107, 145, 194, 194, 270, 369, 500, 661, 849, 1041};
#define COOLANT_TABLE_LENGTH 13
#define coolantInterpolate(idx,reading) (12-idx)*10+((coolantTable[idx]-reading)*10/(coolantTable[idx]-coolantTable[idx-1]))

#define ADC_COOLANT_SENSOR 1
#define COOLANT_READ_PERIOD 3000
// C in degrees, not interested in 0.1 of a degree.
int coolantTemperature;

void readCoolant() {
    static unsigned long lastCoolantReadTime = 0;
  // probably need a long to do this calc
  unsigned long now = millis();
  if ( now > lastCoolantReadTime+COOLANT_READ_PERIOD ) {
    lastCoolantReadTime = now;
    int coolantReading = analogRead(ADC_COOLANT_SENSOR);
    for (int i = 1; i < COOLANT_TABLE_LENGTH; i++) {
      if ( coolantReading < coolantTable[i] ) {
          coolantTemperature = coolantInterpolate(i,coolantReading);
          return;
      }
    }
    // < 0 and off the ADC scale, so cant interpolate. Should never get here.
    coolantTemperature = 0;
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
    for (int i = 0; i < N_ADCS; i++) {
      voltages[i] = VOLTAGE_SCALE*analogRead(i+START_ADC);
    }
  }
}

// NTCLG100E2 10K
int16_t tcurveNTCLG100E2[] = {
  783, //0C = 1024*32554/(32554+10000)
  734, //5C = 1024*25339/(25339+10000)
  681, //10C =  1024*19872/(19872+10000)
  625, //15C 1024*15698/(15698+10000)
  568, //20 1024*12488/(12488+10000)
  512, //25 1024*10000/(10000+10000)
  456, //30 1024*8059/(8059+10000)
  404, //35 1024*6535/(6535+10000)
  356, //40 1024*5330/(5330+10000)
  311, //45 1024*4372/(4372+10000)
  271, //50 1024*3605/(3605+10000)
  235, //55 1024*2989/(2989+10000)
  204, //60 1024*2490/(2490+10000)
  203, //65 1024*2476/(2476+10000)
  203, //70 1024*2476/(2476+10000)
  203, //75 1024*2476/(2476+10000)
  203, //80 1024*2476/(2476+10000)
  203, //85 1024*2476/(2476+10000)
  203, //90 1024*2476/(2476+10000)
  203, //95 1024*2476/(2476+10000)
  203, //100 1024*2476/(2476+10000)
  203, //105 1024*2476/(2476+10000)
  203 //110 1024*2476/(2476+10000)
};
// https://www.gotronic.fr/pj2-mf52type-1554.pdf
// 5v supply, 10K resistor
int16_t tcurveNMF5210K[] = {
  930, //0C = 1024*98960/(98960+10000)
  736, //5C = 1024*25580/(25580+10000)
  682, //10C =  1024*20000/(20000+10000)
  626, //15C 1024*15760/(15760+10000)
  569, //20 1024*12510/(12510+10000)
  512, //25 1024*10000/(10000+10000)
  456, //30 1024*8048/(8048+10000)
  404, //35 1024*6518/(6518+10000)
  355, //40 1024*5312/(5312+10000)
  310, //45 1024*4354/(4354+10000)
  270, //50 1024*3588/(3588+10000)
  235, //55 1024*2989/(2989+10000)
  203, //60 1024*2476/(2476+10000)
  176, //65 1024*2072/(2072+10000)
  152, //70 1024*1743 /(1743+10000)
  131, //75 1024*1473 /(1473 +10000)
  114, //80 1024*1250/(1250+10000)
  99, //85 1024*1065/(1065+10000)
  85, //90 1024*911/(911+10000)
  74, //95 1024*782.4/(782.4+10000)
  65, //100 1024*674.4 /(674.4 +10000)
  56, //105 1024*583.6/(583.6+10000)
  49 //110 1024*506.6/(506.6+10000)
};

#define tcurve tcurveNMF5210K


int16_t ntcTemperature(int16_t reading) {
  if ( reading > tcurve[0] ) {
    return 0;
  }
  for (int i = 1; i < 23; i++) {
    if ( reading > tcurve[i] ) {
      return (i*50)+((tcurve[i-1]-reading)*50)/(tcurve[i-1]-tcurve[i]);
    }
  }
  return 1150;
}


#define START_NTC 2
#define N_NTC 3
#define NTC_READ_PERIOD 10000
// Cx10
uint16_t temperature[3];
void readNTC() {
  static unsigned long lastNTCRead = 0;
  // probably need a long to do this calc
  unsigned long now = millis();
  if ( now > lastNTCRead+NTC_READ_PERIOD ) {
    lastNTCRead = now;
    for (int i = 0; i < N_NTC; i++) {
      temperature[i] = ntcTemperature(analogRead(i+START_NTC));
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


