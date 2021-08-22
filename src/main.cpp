#include <Arduino.h>
#include <pins_arduino.h>
#include <stdlib.h>
#include <inttypes.h>
#include <SPI.h>
#include <Adafruit_MCP23X08.h>
#include <LiquidCrystal_I2C.h>
#include "ARD1939.h"
#include "LowPower.h"



// forward declarations
extern void lcdPrint(int page, int r, int c, int v, const char *fmt);
extern int initIOExtender();
extern char extenderStatus();
extern char j1939Status();
extern char rs485Status();
extern byte canCheckError();
extern void beginButtons();

extern void powerDown();
extern void readButtons();
extern void updateDisplay();
extern void readEngineRPM();
extern void readCoolant();
extern void readFuelLevel();
extern void readVoltages();
extern void sendRS485();
extern void sendJ1939();
extern bool isEnginePowerOn();


#define PIN_RESET 6 // wired to the reset pin. The reset pin has a 10K pullup so at power on


void hardReset() {
  pinMode(PIN_RESET,OUTPUT);
  digitalWrite(PIN_RESET,LOW);
}

void setup() {
  digitalWrite(PIN_RESET,HIGH); // just in case it was low.
  pinMode(PIN_RESET,INPUT); // this ensures it will float high till reset.

  Serial.begin(115200);
  Serial.println(F("Luna monitor start"));
  // ensure SCL and SDA dont power anything during startup.
  pinMode(SCL, INPUT);
  digitalWrite(SCL, LOW);
  pinMode(SDA, INPUT);
  digitalWrite(SDA, LOW);
  pinMode(10, INPUT); // CAN CS
  digitalWrite(10, LOW);
  pinMode(MOSI, INPUT);
  digitalWrite(MOSI, LOW);
  pinMode(MISO, INPUT);
  digitalWrite(MISO, LOW);
  pinMode(SCK, INPUT);
  digitalWrite(SCK, LOW);
  pinMode(3, INPUT); // RPM
  digitalWrite(3, LOW);
  pinMode(5, INPUT); // OIL PRESSURE SWITCH
  digitalWrite(5, LOW);
  beginButtons();  
}


void loop() {
  powerDown();
  readButtons();
  if ( isEnginePowerOn() ) {
    updateDisplay();
    readEngineRPM();
    readCoolant();
    readFuelLevel();
    readVoltages();
    //sendRS485();
    sendJ1939();
    //blink(5,200); // 2s cycle on for 1s
  }
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
    lcdPrint(1,engineRPM,0,4,"%04d");
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
    lcdPrint(1,fuelLevel,0,17,"%03d");
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
          lcdPrint(1,coolantTemperature,0,11,"%03d");
          return;
      }
    }
    // < 0 and off the ADC scale, so cant interpolate. Should never get here.
    coolantTemperature = 0;
    lcdPrint(1, coolantTemperature,0,11,"%03d");
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
      lcdPrint(1,voltages[i]/10,1,(i*4)+5,"%03d");
    }
  }
}




LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
void initLCD() {
  lcd.init();                      // initialize the lcd 
  // Print a message to the LCD.
  lcd.backlight();
            // 012345678901234567890   
  lcd.print(F("Luna Engine Control  "));
  lcd.setCursor(0,1);  
            // 012345678901234567890   
  lcd.print(F("Startup              "));
}

void lcdUpdateState(int c, int r, char status) {
    lcd.setCursor(c,r);
    lcd.print(status);
}


#define DISPLAY_CYCLE_RATE 5000
static int displayPage = -1;
static unsigned long lastDisplayChange = 0;
void updateDisplay() {
  // probably need a long to do this calc
  unsigned long now = millis();
  if ( now > lastDisplayChange+DISPLAY_CYCLE_RATE ) {
    lastDisplayChange = now;
    if ( displayPage < 10 ) {
      displayPage = (displayPage+1)%2;
    }
    switch(displayPage) {
      case 0:
        lcd.setCursor(0,0); 
        lcd.print(F("Luna Engine Control ")); 
        lcd.setCursor(0,1); 
        lcd.print(F("J1939 -, RS485 -    ")); 
        lcdUpdateState(6,1,j1939Status());
        lcdUpdateState(15,1,rs485Status());
        lcd.setCursor(0,2); 
        lcd.print(F("Sensors G, Ports -  ")); 
        lcdUpdateState(17,2,extenderStatus());
        lcd.setCursor(0,3); 
        lcd.print(F("Alarms None         ")); 
        break;
      case 1:    
        lcd.setCursor(0,0);  
                  // 012345678901234567890   
        lcd.print(F("RPM:---- C:--- F:---"));
        lcd.setCursor(0,1);  
                  // 012345678901234567890   
        lcd.print(F("VOLT:---,---,---,---"));
        lcd.setCursor(0,2);  
                  // 012345678901234567890   
        lcd.print(F("TEMP1-3:---,---,--- "));
        lcd.setCursor(0,3);  
        lcd.print(F("TEMP4-6:---,---,--- "));
        lcdPrint(1,engineRPM,0,4,"%04d");
        lcdPrint(1,coolantTemperature,0,11,"%03d");
        lcdPrint(1,fuelLevel,0,17,"%03d");

        for (int i = 0; i < N_ADCS; i++) {
          lcdPrint(1,voltages[i]/10,1,(i*4)+5,"%03d");
        }
      break;
    }
  }
}

void switchDisplay(int page) {
  displayPage = page;
  lastDisplayChange = 0;
  updateDisplay();
}
void resumeDisplay() {
  displayPage = 1;
  lastDisplayChange = 0;
  updateDisplay();
}

void holdDisplay(int page) {
  displayPage = page;
  lastDisplayChange = millis();
}

void lcdPrint(int page, int v, int r, int c, const char *fmt) {
  if ( page == displayPage ) {
    char buffer[5];
    lcd.setCursor(c,r);
    sprintf(&buffer[0],fmt, v);
    lcd.print(&buffer[0]);
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

uint16_t crc_ccitt (const uint8_t * str, unsigned int length) {
  uint16_t crc = 0;
  for (unsigned int i = 0; i < length; i++) {
    uint8_t data = str[i];
    data ^= crc & 0xff;
    data ^= data << 4;
    crc = ((((uint16_t)data << 8) | (crc >> 8)) ^ (uint8_t)(data >> 4) 
                    ^ ((uint16_t)data << 3));
  }
  return crc;  
}

void writeNMEA0183(char *output) {
  unsigned int len = strlen(output);
  uint16_t checksum = crc_ccitt((const uint8_t *)output, len);
  Serial.print(output);
  Serial.print(',');
  Serial.println(checksum,HEX);
}
#define FAST_UPDATE_PERIOD 1000
#define SLOW_UPDATE_PERIOD 10005
#define SLOWEST_UPDATE_PERIOD 20003

void sendRS485() {
  static unsigned long fastUpdate = 0;
  static unsigned long slowUpdate = 0;
  static unsigned long slowestUpdate = 0;
  static char output[16];
  
  unsigned long now = millis();
  if ( now > fastUpdate+FAST_UPDATE_PERIOD ) {
    fastUpdate = now;
    // RPM and Coolant Temp
    sprintf(&output[0],"$LUFUP,%04d,%03d",engineRPM,coolantTemperature);
    writeNMEA0183(&output[0]);    
  }
  if ( now > slowUpdate+SLOW_UPDATE_PERIOD ) {
    slowUpdate = now;
    // voltages
    for(int i = 0; i < N_ADCS; i++) {
      sprintf(&output[0],"$LUVLT,%1d,%04d",i,voltages[i]);
      writeNMEA0183(&output[0]);
    }   
  }
  if ( now > slowestUpdate+SLOWEST_UPDATE_PERIOD ) {
    slowestUpdate = now;    
    sprintf(&output[0],"$LUFUL,%03d",fuelLevel);
    writeNMEA0183(&output[0]);
  }
}

char rs485Status() {
  return 'G';
}



  


// just came out of sleep.
void initPeripherals() {
  Serial.println(F("Starting all peripherals"));
  initIOExtender();
  initLCD();
  lcd.setCursor(0,3);
             //01234567890123456789  
  lcd.print(F("Start flywheel...   "));
  initRPM();
  lcd.setCursor(0,3);  
             //01234567890123456789  
  lcd.print(F("Start j1939...      "));
  initJ1939();
  lcd.setCursor(0,3);  
             //01234567890123456789  
  lcd.print(F("Ready.              "));
  holdDisplay(3);
}

Adafruit_MCP23X08 mcp;
static bool extenderOk = false;
#define RELAY_ON LOW
#define RELAY_OFF HIGH
int initIOExtender() {
  if (!mcp.begin_I2C()) {
    return 0;
  }
  for (int i = 0; i < 8; i++) {
    mcp.pinMode(i, OUTPUT);
    mcp.digitalWrite(i, RELAY_OFF);
  }
  extenderOk = true;
  return 1;
}

char extenderStatus() {
  if ( extenderOk ) {
    return 'G';
  } else {
    return 'F';
  }
}




bool enginePowerOn = false;
#define BUTTON_READ_PERIOD 200
#define MAX_BUTTON_PRESS 300 // 60000/200 ie 60s

// IO Extender outputs
#define V12V_POWER 0
#define V12V40A_POWER 1
#define START_POWER 2
#define STOP_POWER 3
#define GLOW_POWER 4

#define PIN_ONOFF_BUTTON 2
#define PIN_START_BUTTON 7
#define PIN_STOP_BUTTON 8
#define PIN_GLOW_BUTTON 9
#define PIN_MCU_POWER 4


#define BTN_ONOFF 0
#define BTN_START 1
#define BTN_STOP 2
#define BTN_GLOW 3
#define BUTTON_PRESSED(i) (!stateNow[i] && buttonState[i])
#define BUTTON_RELEASED(i) (stateNow[i] && !buttonState[i])

const int button_pins[4] = {PIN_ONOFF_BUTTON,PIN_START_BUTTON,PIN_STOP_BUTTON,PIN_GLOW_BUTTON};
bool buttonState[4] = {1,1,1,1};




void beginButtons() {
  pinMode(PIN_MCU_POWER, OUTPUT); // POWERUP
  digitalWrite(PIN_MCU_POWER, HIGH); // Relay off.

  Serial.println(F("Done Setup IO"));
  for (int i = 0; i < 4; i++) {
    pinMode(button_pins[i], INPUT);
    Serial.print(F("Pin"));
    Serial.print(i);
    Serial.print(F(","));
    buttonState[i] = digitalRead(button_pins[i]);
    Serial.println(buttonState[i]);    
  }
}

// only used for wakeup
void powerWakeHandler() {
}


void powerDown() {
  static unsigned long startSleepMillis = 0;
  unsigned long now = millis();
  if (enginePowerOn) {
    startSleepMillis = 0;
  } else {
    if ( startSleepMillis == 0 ) {
      // stay awake for 30s after power down
      startSleepMillis = now + 30000;
    } else if ( now > startSleepMillis) {
      // go to sleep
      Serial.println(F("Sleeping....."));
      delay(100);
      // bring everything into a low power state, the only way out is to restart
      Wire.end();
      SPI.end();

      pinMode(SCL, INPUT); 
      digitalWrite(SCL, LOW);
      pinMode(SDA, INPUT); 
      digitalWrite(SDA, LOW);
      // switch all non SPI pins to floating inputs to ensure they dont power the board.
      pinMode(10, INPUT); 
      digitalWrite(10, LOW);
      pinMode(MOSI, INPUT);
      digitalWrite(MOSI, LOW);
      pinMode(MISO, INPUT);
      digitalWrite(MISO, LOW);
      pinMode(SCK, INPUT);
      digitalWrite(SCK, LOW);
      // Note the only way to restart the Can shield is to perform a reset on the MCU

      // make sure the flywheel doesnt wake up.
      Serial.end();
      detachInterrupt(digitalPinToInterrupt(PINS_FLYWHEEL));
      attachInterrupt(digitalPinToInterrupt(PIN_ONOFF_BUTTON), powerWakeHandler, LOW);
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
      Serial.begin(115300);
      Serial.println("Wokeup...");
      delay(100);
      // the bootloader on pro mini's has a but that causes a 
      // watchdog reset to go into an infinite loop
      // this jumps to 0, but may still not work to reset all the 
      // IO pins properly.
      hardReset();
      // doesnt get here.
    }
  }
}

bool isEnginePowerOn() {
  return enginePowerOn;
}

/**
 * The onoff button is edge triggerd, the other buttons are read.
 */ 
void readButtons() {
  static unsigned long lastButtonRead = 0;
  static uint16_t ticks[4] = {0,0,0,0};
  // at power up, all the lines are pulled high by an external resistor
  // so the button state stats as all 1's.
  unsigned long now = millis();
  if ( now > lastButtonRead+BUTTON_READ_PERIOD ) {
    lastButtonRead = now;
    int stateNow[4];
    int npressed = 0;
    for (int i = 0; i < 4; i++) {
      stateNow[i] = digitalRead(button_pins[i]);
      if (!stateNow[i]) npressed++;
    }
    if ( npressed > 1) {
      Serial.print(F("Buttons low, power needed, "));
      Serial.println(npressed);

      // multiple buttons pressed, ignore
      return;
    }
    for(int i = 0; i < 4; i++) {
      // 1 == voltage high, button not pressed
      // 0 == voltage low, button pressed
      if ( buttonState[i] == stateNow[i] ) {
        // no change in state, after being acted on.
        // buttonState is updated once the button press has been acted on.
        ticks[i] = 0;
      } else {
        ticks[i]++;
      }
      if ( !stateNow[i] && ticks[i] > MAX_BUTTON_PRESS ) {
        // button has been pressed for 60s, is this a short ?
        // force button off, ie high
        stateNow[i] = 1;
        Serial.println(F("Button Short"));
      }
      if ( ticks[i] > 0 ) {
        // state has remained the same for 2x read period, not a bounce.
        switch(i) {
          case BTN_ONOFF:
            if (BUTTON_PRESSED(i)) {
              Serial.println(F("OnOff pressed"));
              // button press detected
              if ( enginePowerOn  ) {
                // power down
                  // make certain start and glow are off.
                  mcp.digitalWrite(START_POWER,RELAY_OFF);
                  mcp.digitalWrite(GLOW_POWER,RELAY_OFF);
                  mcp.digitalWrite(STOP_POWER,RELAY_OFF);
                  mcp.digitalWrite(V12V40A_POWER, RELAY_OFF); // 40A 12V power.
                  mcp.digitalWrite(V12V_POWER, RELAY_OFF); // 12v peripherals
                  digitalWrite(PIN_MCU_POWER,RELAY_OFF); // 8v and 5v
                  enginePowerOn = false;
                  Serial.println(F("Reboot"));
                  Serial.end();
                  delay(100);
                  hardReset();
                  // never gets here after the reset.

              } else {
                  Serial.println(F("Power On"));
                  // power is off, go through power on sequence
                  // this can be done with delays rather than looping.
                  digitalWrite(PIN_MCU_POWER,RELAY_ON); // 8v and 5v
                  delay(100); // let 8v and 5v units startup.
                  initPeripherals();
                  // make certain start and glow are off, before powering up 12v
                  mcp.digitalWrite(START_POWER,RELAY_OFF);
                  mcp.digitalWrite(GLOW_POWER,RELAY_OFF);
                  mcp.digitalWrite(STOP_POWER,RELAY_OFF);

                  delay(100);
                  mcp.digitalWrite(V12V_POWER, RELAY_ON); // 12v peripherals
                  delay(100);
                  mcp.digitalWrite(V12V40A_POWER, RELAY_ON); // 40A 12V power.
                  enginePowerOn = true;
              }
              buttonState[i] = stateNow[i];
            } else if ( BUTTON_RELEASED(i)) {
              Serial.println(F("OnOff released"));
              buttonState[i] = stateNow[i];
            }
          break;
          case BTN_START:
            // the start button is press on, release off.
            if ( BUTTON_PRESSED(i)  ) {
              Serial.println(F("Start pressed"));
              if ( enginePowerOn ) {
                // only if the engine is powered on.
                if ( coolantTemperature < 50 ) {
                  mcp.digitalWrite(GLOW_POWER, RELAY_ON); // force glow on
                } else {
                  mcp.digitalWrite(GLOW_POWER, RELAY_OFF); // force glow off
                }
                mcp.digitalWrite(STOP_POWER, RELAY_OFF); // stop relay off
                mcp.digitalWrite(START_POWER, RELAY_ON); // start
                Serial.println(F("Starting"));
              } else {
                Serial.println(F("Start ignored"));
              }
              buttonState[i] = stateNow[i];
            } else if ( BUTTON_RELEASED(i) ) {
              Serial.println(F("Start released"));
            // turn start sequence off, regardless
              mcp.digitalWrite(STOP_POWER, RELAY_OFF); // stop relay off
              mcp.digitalWrite(START_POWER, RELAY_OFF);
              mcp.digitalWrite(GLOW_POWER, RELAY_OFF);
              buttonState[i] = stateNow[i];
              Serial.println(F("Not Starting"));
            }

            // could get the engine temp and decide to turn the glow plugs on automatically.
            // below 60C they are supposed to be turned on
            // if we did this then we would need to 
            // start
            break;
          case BTN_STOP:
            // stop button is press on, release off.
            if ( BUTTON_PRESSED(i) ) {
              Serial.println(F("Stop pressed"));
              mcp.digitalWrite(START_POWER, RELAY_OFF);
              mcp.digitalWrite(GLOW_POWER, RELAY_OFF);
              mcp.digitalWrite(STOP_POWER, RELAY_ON); // stop relay on
              buttonState[i] = stateNow[i];
              Serial.println(F("Stopping"));
            } else if (BUTTON_RELEASED(i) ) {
              Serial.println(F("Stop released"));
              mcp.digitalWrite(START_POWER, RELAY_OFF);
              mcp.digitalWrite(GLOW_POWER, RELAY_OFF);
              mcp.digitalWrite(STOP_POWER, RELAY_OFF); // stop relay off
              buttonState[i] = stateNow[i];
              Serial.println(F("Not Stopping"));
            }
            break;
          case BTN_GLOW:
            // glow button is press on, release off
            // glow
            if ( BUTTON_PRESSED(i) ) {
              Serial.println(F("Glow pressed"));
              if ( enginePowerOn ) {
                mcp.digitalWrite(START_POWER, RELAY_OFF);
                mcp.digitalWrite(GLOW_POWER, RELAY_ON); // glow on
                mcp.digitalWrite(STOP_POWER, RELAY_OFF); 
                Serial.println(F("Glow on"));
              } else {
                Serial.println(F("Glow button ignored"));
              }
              buttonState[i] = stateNow[i];
            } else if (BUTTON_RELEASED(i) ) {
              Serial.println(F("Glow released"));
              mcp.digitalWrite(START_POWER, RELAY_OFF);
              mcp.digitalWrite(GLOW_POWER, RELAY_OFF); // glow off
              mcp.digitalWrite(STOP_POWER, RELAY_OFF);
              buttonState[i] = stateNow[i];
              Serial.println(F("Glow off"));
            }
            break;
        }
      }
    }
  }
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




