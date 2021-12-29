
#include <Arduino.h>

#define USE_MCP_CAN_CLOCK_SET 8
#define SNMEA_SPI_CS_PIN 10
#include "enginesensors.h"
#include "SmallNMEA2000.h"

#define RAPID_ENGINE_UPDATE_PERIOD 500
#define ENGINE_UPDATE_PERIOD 1000
#define VOLTAGE_UPDATE_PERIOD 9996
#define FUEL_UPDATE_PERIOD 10000
#define TEMPERATURE_UPDATE_PERIOD 4850

const SNMEA2000ProductInfo productInfomation PROGMEM={
                                       1300,                        // N2kVersion
                                       44,                         // Manufacturer's product code
                                       "EMS",    // Manufacturer's Model ID
                                       "1.2.3.4 (2017-06-11)",     // Manufacturer's Software version code
                                       "5.6.7.8 (2017-06-11)",      // Manufacturer's Model version
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

EngineSensors sensors(DEFAULT_FLYWHEEL_READ_PERIOD,
  DEFAULT_FUEL_READ_PERIOD,
  DEFAULT_COOLANT_READ_PERIOD,
  DEFAULT_VOLTAGE_READ_PERIOD,
  DEFAULT_NTC_READ_PERIOD);

void sendRapidEngineData() {
  static unsigned long lastRapidEngineUpdate=0;
  static byte sid = 0;
  unsigned long now = millis();
  if ( now > lastRapidEngineUpdate+RAPID_ENGINE_UPDATE_PERIOD ) {
    lastRapidEngineUpdate = now;
    engineMonitor.sendRapidEngineDataMessage(sid++, sensors.getEngineRPM());
  }
}


void sendEngineData() {
  static unsigned long lastEngineUpdate=0;
  static byte sid = 0;
  unsigned long now = millis();
  if ( now > lastEngineUpdate+ENGINE_UPDATE_PERIOD ) {
    lastEngineUpdate = now;
    engineMonitor.sendEngineDynamicParamMessage(sid++,
        sensors.getEngineSeconds(),
        sensors.getCoolantTemperatureK(),
        sensors.getAlternatorVoltage());
  }
}

void sendVoltages() {
  static unsigned long lastVoltageUpdate=0;
  static byte sid = 0;
  unsigned long now = millis();
  if ( now > lastVoltageUpdate+VOLTAGE_UPDATE_PERIOD ) {
    lastVoltageUpdate = now;
    // because the engine monitor is not on all the time, the sid and instance ids of these messages has been shifted
    // to make space for sensors that are on all the time, and would be used by default
    engineMonitor.sendDCBatterStatusMessage(sid, 2, sensors.getServiceBatteryVoltage(), sensors.getEngineRoomTemperatureK());
    engineMonitor.sendDCBatterStatusMessage(sid, 3, sensors.getEngineBatteryVoltage(), sensors.getEngineRoomTemperatureK());
    engineMonitor.sendDCBatterStatusMessage(sid, 4, sensors.getAlternatorVoltage(), sensors.getAlternatorTemperatureK());
    sid++;
  }
}

void sendFuel() {
  static unsigned long lastFuelUpdate=0;
  static byte sid = 0;
  unsigned long now = millis();
  if ( now > lastFuelUpdate+FUEL_UPDATE_PERIOD ) {
    lastFuelUpdate = now;
    engineMonitor.sendFluidLevelMessage(sid, 0, sensors.getFuelLevel(), sensors.getFuelCapacity());
    sid++;
  }
}

void sendTemperatures() {
  static unsigned long lastTempUpdate=0;
  static byte sid = 0;
  unsigned long now = millis();
  if ( now > lastTempUpdate+TEMPERATURE_UPDATE_PERIOD ) {
    lastTempUpdate = now;
    engineMonitor.sendTemperatureMessage(sid, 0, 14, sensors.getExhaustTemperatureK());
    engineMonitor.sendTemperatureMessage(sid, 1, 3, sensors.getEngineRoomTemperatureK());
    engineMonitor.sendTemperatureMessage(sid, 2, 15, sensors.getAlternatorTemperatureK());
    sid++;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Luna monitor start"));
  sensors.init();
  Serial.println(F("Opening CAN"));
  engineMonitor.open();
  Serial.println(F("Opened"));
  Serial.println(F("Running..."));
}

void loop() {
  sensors.read();
  sendRapidEngineData();
  sendEngineData();
  sendVoltages();
  sendTemperatures();
  sendFuel();
  engineMonitor.processMessages();
}


