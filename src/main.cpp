#include <Arduino.h>
#include <stdlib.h>
#include <inttypes.h>
#include <SPI.h>
#include <Adafruit_MCP23X08.h>
#include <LiquidCrystal_I2C.h>
#include "ARD1939.h"


// forward declarations
extern void lcdPrint(int page, int r, int c, int v, const char *fmt);
extern void stopPeripherals();
extern void endIOExtender();
extern int initIOExtender();



#define PINS_FLYWHEEL 2
#define PINS_CAN 3
volatile unsigned long flywheelPulses = 0;

void flywheelPuseHandler() {
  flywheelPulses++;
}

void initRPM() {
  pinMode(PINS_FLYWHEEL, INPUT);
  attachInterrupt(digitalPinToInterrupt(PINS_FLYWHEEL), flywheelPuseHandler, RISING);
}
void endRPM() {
  // no action required.
}

int engineRPM = 0;
#define FLYWHEEL_READ_PERIOD 500
// pulses  Perms -> RPM conversion.
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

void endLCD() {
  Wire.end();
  pinMode(SCL, OUTPUT); 
  digitalWrite(SCL, LOW);
  pinMode(SDA, OUTPUT); 
  digitalWrite(SDA, LOW);
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
        lcd.print(F("J1939 Ok, RS485 Ok  ")); 
        lcd.setCursor(0,2); 
        lcd.print(F("Sensors Ok          ")); 
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

void holdDisplay() {
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

ARD1939 j1939;
void initJ1939() {
  // Initialize the J1939 protocol including CAN settings
  if(j1939.Init(SYSTEM_TIME) == 0)
    Serial.print(F("CAN Controller Init OK.\n\r\n\r"));
  else
    Serial.print(F("CAN Controller Init Failed.\n\r"));
    
  // Set the preferred address and address range
  j1939.SetPreferredAddress(SA_PREFERRED);
  j1939.SetAddressRange(ADDRESSRANGEBOTTOM, ADDRESSRANGETOP);
  
  // Set the message filter
  //j1939.SetMessageFilter(59999);
  
  //j1939.SetMessageFilter(65242);
  //j1939.SetMessageFilter(65259);
  //j1939.SetMessageFilter(65267);
  
  // Set the NAME
  j1939.SetNAME(NAME_IDENTITY_NUMBER,
                NAME_MANUFACTURER_CODE,
                NAME_FUNCTION_INSTANCE,
                NAME_ECU_INSTANCE,
                NAME_FUNCTION,
                NAME_VEHICLE_SYSTEM,
                NAME_VEHICLE_SYSTEM_INSTANCE,
                NAME_INDUSTRY_GROUP,
                NAME_ARBITRARY_ADDRESS_CAPABLE);
}

void  endJ1939() {
  SPI.end();
  pinMode(2, OUTPUT); 
  digitalWrite(2, LOW);
  pinMode(10, OUTPUT); 
  digitalWrite(10, LOW);
  pinMode(11, OUTPUT); 
  digitalWrite(11, LOW);
  pinMode(12, OUTPUT); 
  digitalWrite(12, LOW);
  pinMode(13, OUTPUT); 
  digitalWrite(13, LOW);
}

void sendJ1939() {


  static int nCounter = 0;

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
  // RTS/CTS Session
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
      
        break;
        
      case NORMALDATATRAFFIC:
      
        // Determine the negotiated source address
        //byte nAppAddress;
        //nAppAddress = j1939.GetSourceAddress();
        
       break;
        
      case ADDRESSCLAIM_FAILED:
      
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


void setup() {
  pinMode(2, INPUT); // CAN INT
  pinMode(3, INPUT); // RPM
  pinMode(4, OUTPUT); // POWERUP
  digitalWrite(4, HIGH);
  pinMode(5, INPUT); // OIL PRESSURE SWITCH
  Serial.begin(115200);
  delay(1000);
  Serial.println("Settup IO");
  // setup buttons.
  // use input pullup for testing only
  pinMode(6, INPUT);
  pinMode(7, INPUT);
  pinMode(8, INPUT);
  pinMode(9, INPUT);
  pinMode(10, OUTPUT); // CAN CS
  Serial.println("Done Setup IO");
  for (int i = 6; i < 10; i++) {
    Serial.print("Pin");
    Serial.print(i);
    Serial.print(",");
    Serial.println(digitalRead(i));    
  }
  stopPeripherals();
}


void stopPeripherals() {
  // scl sda low
  //D10, D11, D12, D13 low
  // D2 low
  endLCD();
  endRPM();
  endJ1939();
  endIOExtender();
  Serial.println("All Pins should be low");
}


// just came out of sleep.
void initPeripherals() {
  Serial.println("Starting all peripherals");
  initIOExtender();
  initLCD();
  lcd.setCursor(1,7);  
  lcd.print(F("."));
  initRPM();
  lcd.setCursor(1,8);  
  lcd.print(F("."));
  initJ1939();
  lcd.setCursor(1,9);  
  lcd.print(F("."));
  lcd.setCursor(0,2);  
  lcd.print(F("Ready.....         "));
  holdDisplay();
}

Adafruit_MCP23X08 mcp;
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
  return 1;
}

void endIOExtender() {
  Wire.end();
}



bool enginePowerOn = false;
#define BUTTON_READ_PERIOD 200
#define MAX_BUTTON_PRESS 300 // 60000/200 ie 60s
#define START_POWER 2
#define STOP_POWER 3
#define GLOW_POWER 4
#define V12V_POWER 0
#define V12V40A_POWER 1
#define MCU_POWER 4

#define BTN_ONOFF 0
#define BTN_START 1
#define BTN_STOP 2
#define BTN_GLOW 3
#define BUTTON_PRESSED(i) (!stateNow[i] && buttonState[i])
#define BUTTON_RELEASED(i) (stateNow[i] && !buttonState[i])



void readButtons() {
  static unsigned long lastButtonRead = 0;
  static uint16_t ticks[4] = {0,0,0,0};
  // at power up, all the lines are pulled high by an external resistor
  // so the button state stats as all 1's.
  static bool buttonState[4] = { 1,1,1,1};
  unsigned long now = millis();
  if ( now > lastButtonRead+BUTTON_READ_PERIOD ) {
    lastButtonRead = now;
    int stateNow[4];
    int npressed = 0;
    for (int i = 0; i < 4; i++) {
      stateNow[i] = digitalRead(i+6);
      if (!stateNow[i]) npressed++;
    }
    if ( npressed > 1) {
      Serial.print("Buttons pressed is ");
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
        Serial.println("Button Short");
      }
      if ( ticks[i] > 0 ) {
        // state has remained the same for 2x read period, not a bounce.
        switch(i) {
          case BTN_ONOFF:
            // on/off button is press on, release, press off.
            // no action on release.
            // button pressed, but not acted on
            if ( BUTTON_PRESSED(i) ) {            
                Serial.println("On/Off pressed ");
                if ( enginePowerOn  ) {
                  // power down
                    // make certain start and glow are off.
                    mcp.digitalWrite(START_POWER,RELAY_OFF);
                    mcp.digitalWrite(GLOW_POWER,RELAY_OFF);
                    mcp.digitalWrite(STOP_POWER,RELAY_OFF);
                    mcp.digitalWrite(V12V40A_POWER, RELAY_OFF); // 40A 12V power.
                    mcp.digitalWrite(V12V_POWER, RELAY_OFF); // 12v peripherals
                    stopPeripherals();
                    digitalWrite(MCU_POWER,RELAY_OFF); // 8v and 5v
                    enginePowerOn = false;
                    Serial.println("Power Off");
                } else {
                    // power is off, go through power on sequence
                    // this can be done with delays rather than looping.
                    digitalWrite(MCU_POWER,RELAY_ON); // 8v and 5v
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
                    Serial.println("Power On");
                }
            } else if ( BUTTON_RELEASED(i) ) {
              Serial.println("On/Off release ignored");
              // button released, or button still pressed, do nothing
            } else {
              // no change
              Serial.println("No change in button state should not happen");
            }
            buttonState[i] = stateNow[i];
            break;
          case BTN_START:
            // the start button is press on, release off.
            if ( BUTTON_PRESSED(i)  ) {
              Serial.println("Start pressed");
              if ( enginePowerOn ) {
                // only if the engine is powered on.
                if ( coolantTemperature < 50 ) {
                  mcp.digitalWrite(GLOW_POWER, RELAY_ON); // force glow on
                } else {
                  mcp.digitalWrite(GLOW_POWER, RELAY_OFF); // force glow off
                }
                mcp.digitalWrite(STOP_POWER, RELAY_OFF); // stop relay off
                mcp.digitalWrite(START_POWER, RELAY_ON); // start
                Serial.println("Starting");
              } else {
                Serial.println("Start ignored");
              }
              buttonState[i] = stateNow[i];
            } else if ( BUTTON_RELEASED(i) ) {
              Serial.println("Start released");
            // turn start sequence off, regardless
              mcp.digitalWrite(STOP_POWER, RELAY_OFF); // stop relay off
              mcp.digitalWrite(START_POWER, RELAY_OFF);
              mcp.digitalWrite(GLOW_POWER, RELAY_OFF);
              buttonState[i] = stateNow[i];
              Serial.println("Not Starting");
            }

            // could get the engine temp and decide to turn the glow plugs on automatically.
            // below 60C they are supposed to be turned on
            // if we did this then we would need to 
            // start
            break;
          case BTN_STOP:
            // stop button is press on, release off.
            if ( BUTTON_PRESSED(i) ) {
              Serial.println("Stop pressed");
              mcp.digitalWrite(START_POWER, RELAY_OFF);
              mcp.digitalWrite(GLOW_POWER, RELAY_OFF);
              mcp.digitalWrite(STOP_POWER, RELAY_ON); // stop relay on
              buttonState[i] = stateNow[i];
              Serial.println("Stopping");
            } else if (BUTTON_RELEASED(i) ) {
              Serial.println("Stop released");
              mcp.digitalWrite(START_POWER, RELAY_OFF);
              mcp.digitalWrite(GLOW_POWER, RELAY_OFF);
              mcp.digitalWrite(STOP_POWER, RELAY_OFF); // stop relay off
              buttonState[i] = stateNow[i];
              Serial.println("Not Stopping");
            }
            break;
          case BTN_GLOW:
            // glow button is press on, release off
            // glow
            if ( BUTTON_PRESSED(i) ) {
              Serial.println("Glow pressed");
              if ( enginePowerOn ) {
                mcp.digitalWrite(START_POWER, RELAY_OFF);
                mcp.digitalWrite(GLOW_POWER, RELAY_ON); // glow on
                mcp.digitalWrite(STOP_POWER, RELAY_OFF); 
                Serial.println("Glow on");
              } else {
                Serial.println("Glow button ignored");
              }
              buttonState[i] = stateNow[i];
            } else if (BUTTON_RELEASED(i) ) {
              Serial.println("Glow released");
              mcp.digitalWrite(START_POWER, RELAY_OFF);
              mcp.digitalWrite(GLOW_POWER, RELAY_OFF); // glow off
              mcp.digitalWrite(STOP_POWER, RELAY_OFF);
              buttonState[i] = stateNow[i];
              Serial.println("Glow off");
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


void loop() {
  readButtons();
  if ( enginePowerOn ) {
    updateDisplay();
    readEngineRPM();
    readCoolant();
    readFuelLevel();
    readVoltages();
    //sendRS485();
    sendJ1939();
    //blink(5,200); // 2s cycle on for 1s
  } else {
    //blink(1,400); // 4s cycle on for 400ms
  }
}