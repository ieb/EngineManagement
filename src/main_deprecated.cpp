#ifdef DONT_COMPILE

#include <Arduino.h>
#include <pins_arduino.h>
#include <stdlib.h>
#include <inttypes.h>
#include <SPI.h>
#include "ARD1939.h"



// forward declarations
extern char j1939Status();
extern byte canCheckError();
extern void readEngineRPM();
extern void readCoolant();
extern void readFuelLevel();
extern void readVoltages();
extern void sendJ1939();
extern bool isEnginePowerOn();
extern void initJ1939();




void setup() {

  Serial.begin(115200);
  Serial.println(F("Luna monitor start"));
  // ensure SCL and SDA dont power anything during startup.
  pinMode(3, INPUT); // RPM
  digitalWrite(3, LOW);
  pinMode(5, INPUT); // OIL PRESSURE SWITCH
  digitalWrite(5, LOW);
  initJ1939();
}


void loop() {
  readEngineRPM();
  readCoolant();
  readFuelLevel();
  readVoltages();
  sendJ1939();
}


#define PINS_FLYWHEEL 3
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

#define START_ADC 3
#define N_ADCS 4
// stored as voltage*100
int voltages[4];
// to scale the voltage V*100 =  V*500*(100+47)/(47*1024);
// to keep the calculation integer split
#define VOLTAGE_SCALE_U 73500 // 500*(100+47)
#define VOLTAGE_SCALE_L 48128 // (47*1024)
#define VOLTAGE_READ_PERIOD 5000

void readVoltages() {
  static unsigned long lastVotageRead = 0;
  // probably need a long to do this calc
  unsigned long now = millis();
  if ( now > lastVotageRead+VOLTAGE_READ_PERIOD ) {
    lastVotageRead = now;
    for (int i = 0; i < N_ADCS; i++) {
      unsigned long reading = analogRead(i+START_ADC);
      unsigned long v100 = (reading*VOLTAGE_SCALE_U)/VOLTAGE_SCALE_L; 
      voltages[i] = v100;
    }
  }
}







/*
 * VP runs J1939 at 250 kbit
 * Source address for engine is 0x00
 * lookup pgns in 1939-71 to get the SPN format.
 * pgn65262 page 371 ET1 - Engine temperature 1 every 1000ms SPNs, 110 Engine coolant temperature, 175 Engine oil temperature
 * pgn65253 HOURS - Engine hours, revolutions every 10s, 247 Engine total hours of operation
 * pgn61444 EEC1 - Electronic engine controller 1, 512 Drivers demand engine - percent torque, 
 * 
 * pgn65351 Engine information, see docs/fulltext01.pdf page 24, Vp custom.
 * 
 * 
 * Can DBC editor
 * https://docs.google.com/spreadsheets/d/1X6VQBkqRX0a6QLd4oFbAos_ps3zrpVc86CVGd67DoSs/edit
 * 
 * 
 */
#define EMS_ADDRESS 0
// 21 bit ID number
#define EMS_IDENTITY 123
//  
#define EMS_MANUFACTURER_CODE 1
#define EMS_FUNCTION_INSTANCE 0
#define EMS_ECU_INSTANCE 0
#define EMS_FUNCTION 0
#define EMS_VEHICLE_SYSTEM 0
#define EMS_VEHICLE_SYSTEM_INSTANCE 0
#define EMS_INDUSTRY_GROUP 0
#define EMS_ARBITRARY_ADDRESS_CAPABLE 0

ARD1939 j1939;
static bool j1939Ok = false;
void initJ1939() {
  // setup pin directions.
  pinMode(10, OUTPUT); 
  // drive outputs high before initialising.
  digitalWrite(10, HIGH);

  Serial.println(F("j1939 Init Started"));

  // Initialize the J1939 protocol including CAN settings
  // calls Begin
  if(j1939.Init(SYSTEM_TIME) != 0) {
    j1939Ok = false;
    Serial.print(F("CAN Controller Init Failed.\n\r"));
    return;
  }
  j1939Ok = true;
  Serial.print(F("CAN Controller Init OK.\n\r\n\r"));
    
  // Set the preferred address and address range
  j1939.SetPreferredAddress(EMS_ADDRESS);
  j1939.SetAddressRange(EMS_ADDRESS, ADDRESSRANGETOP);
  
  // Set the message filter
  //j1939.SetMessageFilter(59999);
  
  //j1939.SetMessageFilter(65242);
  //j1939.SetMessageFilter(65259);
  //j1939.SetMessageFilter(65267);
  
  // Set the NAME
  j1939.SetNAME(EMS_IDENTITY,
                EMS_MANUFACTURER_CODE,
                EMS_FUNCTION_INSTANCE,
                EMS_ECU_INSTANCE,
                EMS_FUNCTION,
                EMS_VEHICLE_SYSTEM,
                EMS_VEHICLE_SYSTEM_INSTANCE,
                EMS_INDUSTRY_GROUP,
                EMS_ARBITRARY_ADDRESS_CAPABLE);
}

char j1939Status() {
  if ( !j1939Ok ) {
    return 'F';
  } else if ( canCheckError() ) { 
    return 'E';
  } 
  return 'G';
}

struct PGN61444 {
  byte engineTorqueMode;
  byte requestedTorque;
  byte deliveredTorque;
  uint16_t engineSpeed;
  byte controlSourceAddress;
  byte engineStarterMode;
  byte engineDemantTorque;
};

union PGN61444Msg {
  byte pgn61444Data[sizeof(PGN61444)];
  PGN61444 pgn61444;
};

#define STARTER_NOT_REQUESTED 0b0000 // start not requested
#define STARTER_ACTIVE_NO_GEAR 0b0001 //starter active, gear not engaged
#define STARTER_ACTIVE_GEAR 0b0010 //starter active, gear engaged
#define STARTER_FINISHED 0b0011 //start finished; starter not active after having been actively engaged (after 50ms mode goes to 0000) 0100 starter inhibited due to engine already running
#define STARTER_NOT_READY_PREHEAT 0b0101 //starter inhibited due to engine not ready for start (preheating)
#define STARTER_INHIBIT_GEAR 0b0110 //starter inhibited due to driveline engaged or other transmission inhibit
#define STARTER_INHIBIT_IMOBIL 0b0111 //starter inhibited due to active immobilizer
#define STARTER_INHIBIT_OVER_TEMP 0b1000 //starter inhibited due to starter over-temp
#define STARTER_INHIBIT_UNOWN 0b1100 //starter inhibited - reason unknown
#define STARTER_ERROR 0b1110 //error
#define STARTER_NA 0b1111 //not available


#define J1939_UPDATE_PERIOD 500
void sendJ1939() {

  static unsigned long j1939Update = 0;
  static int nCounter = 0;

  unsigned long now = millis();
  if ( now < j1939Update+J1939_UPDATE_PERIOD ) {
    return;
  }
  j1939Update = now;


 // J1939 Variables
  byte nMsgId;
  byte nDestAddr;
  byte nSrcAddr;
  byte nPriority;
  byte nJ1939Status;
  
  int nMsgLen;
  
  long lPGN;
  
  byte pMsg[J1939_MSGLEN];
  
  // Variables for proof of concept tests
  //byte msgFakeNAME[] = {0, 0, 0, 0, 0, 0, 0, 0};
  byte msgLong[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45};
  
  // Establish the timer base in units of milliseconds
  delay(SYSTEM_TIME);
  
  // Call the J1939 protocol stack
  nJ1939Status = j1939.Operate(&nMsgId, &lPGN, &pMsg[0], &nMsgLen, &nDestAddr, &nSrcAddr, &nPriority);
  switch(nJ1939Status) {
    case ADDRESSCLAIM_INPROGRESS:
    Serial.print(F("Address Claim Inprogress"));
    break;
    case ADDRESSCLAIM_FAILED:
    Serial.print(F("Address Claim Failed"));
    break;
    case NORMALDATATRAFFIC:
    Serial.print(F("Normal"));
    break;
  }
  switch(nMsgId) {
    case J1939_MSG_PROTOCOL:
    Serial.print(F(" MSG Protocol "));
    break;
    case J1939_MSG_APP:
    Serial.print(F(" MSG App "));
    break;
    default:
    Serial.print(F(" MSG "));
    Serial.print(nMsgId);
    break;   
  }
  Serial.print(lPGN);
  Serial.print(F("  "));
  Serial.print(nSrcAddr);
  Serial.println(".");

  nSrcAddr = j1939.GetSourceAddress();
  
  // SPN 899 1.1 4 bits engine torque mode 0x00, no request
  // SPN 512 2   1 byte requested torque 125 no torque
  // SPN 513 3   1 byte delivered torque 125 no torque
  // SPN 190 4   2 bytes engine speed  0.125 rpm per bit
  // SPN 1483 6  1 byte source address controlling engine
  // SPN 1675 7.1 4 bits Engine starter mode
  // SPN 2432 8 1 byte engine torque demand



  PGN61444Msg engineUpdate;
  engineUpdate.pgn61444.engineTorqueMode = 0x00;
  engineUpdate.pgn61444.requestedTorque = 128;
  engineUpdate.pgn61444.deliveredTorque = 128;
  engineUpdate.pgn61444.engineSpeed = 2000*8;
  engineUpdate.pgn61444.controlSourceAddress = 0;
  engineUpdate.pgn61444.engineStarterMode = STARTER_NOT_REQUESTED;
  engineUpdate.pgn61444.engineDemantTorque = 128;
  j1939.Transmit(3, 61444, 0, GLOBALADDRESS, engineUpdate.pgn61444Data, sizeof(PGN61444));

  /*
  // Block certain claimed addresses
  if(nMsgId == J1939_MSG_PROTOCOL)
  {
    if(lPGN == 0x00EE00)
    {
      if(nSrcAddr >= 129 && nSrcAddr <= 134)
        j1939.Transmit(6, 0x00EE00, nSrcAddr, 255, msgFakeNAME, 8);
    
    }// end if
    
  }// end if
  */
  
  /*
  // Send out a periodic message with a length of more than 8 bytes
  // BAM Session
  if(nJ1939Status == NORMALDATATRAFFIC)
  {
    nCounter++;
    
    if(nCounter == (int)(5000/SYSTEM_TIME))
    {
      nSrcAddr = j1939.GetSourceAddress();
      j1939.Transmit(6, 59999, nSrcAddr, 255, msgLong, 15);
      nCounter = 0;
      
    }// end if
  
  }// end if
  */
  
  
  // Send out a periodic message with a length of more than 8 bytes
  // RTS/CTS SessionNORMALDATATRAFFIC
  if(nJ1939Status == NORMALDATATRAFFIC)
  {
    nCounter++;
    
    if(nCounter == (int)(5000/SYSTEM_TIME))
    {
      nSrcAddr = j1939.GetSourceAddress();
      j1939.Transmit(6, 59999, nSrcAddr, 0x33, msgLong, 15);
      nCounter = 0;
      
    }// end if
  
  }// end if
  
  
  // Check for reception of PGNs for our ECU/CA
  if(nMsgId == J1939_MSG_APP)
  {
    // Check J1939 protocol status
    switch(nJ1939Status)
    {
      case ADDRESSCLAIM_INPROGRESS:
        Serial.println("Claim in progress");
      
        break;
        
      case NORMALDATATRAFFIC:
        Serial.println("Normal data traffic");
      
        // Determine the negotiated source address
        //byte nAppAddress;
        //nAppAddress = j1939.GetSourceAddress();
        
       break;
        
      case ADDRESSCLAIM_FAILED:
        Serial.println("Address claim failed");
        break;
      
    }// end switch(nJ1939Status)
    
  }// end if  // put your main code here, to run repeatedly:
}


  


// just came out of sleep.
void initPeripherals() {
  Serial.println(F("Starting all peripherals"));
  initRPM();
  initJ1939();
}



// provide a sign of life on the board.
#define LED_PERIODS 10
void blink(int onAt, int period) {
  static unsigned long lastBlink = 0;
  static uint8_t ticks = LED_PERIODS;
  unsigned long now = millis();
  if ( now > lastBlink+period ) {
    lastBlink = now;
    ticks--;
    if ( ticks == onAt) {
      digitalWrite(LED_BUILTIN, HIGH);
    } else if ( ticks == 0) {
      digitalWrite(LED_BUILTIN, LOW);
      ticks = LED_PERIODS;
    }
  }
}





#endif