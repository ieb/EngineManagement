#ifndef ENGINESENSORS_H
#define ENGINESENSORS_H

#include <Arduino.h>


// read frequencies
#define DEFAULT_FLYWHEEL_READ_PERIOD 2000
#define DEFAULT_FUEL_READ_PERIOD 10000
#define DEFAULT_COOLANT_READ_PERIOD 1000
#define DEFAULT_VOLTAGE_READ_PERIOD 1000
#define DEFAULT_NTC_READ_PERIOD 2000


// Engine hours resolution
#define ENGINE_HOURS_PERIOD_MS 15000


// NTC numbering
#define EXHAUST_TEMP 0
#define ALTERNATOR_TEMP 1
#define ENGINEROOM_TEMP 2
#define START_NTC 2
#define N_NTC 3


// ADC Numbering and calibration.
#define START_ADC 5
#define N_ADCS 3
// stored as voltage
// to scale the voltage V =  V*5*(100+47)/(47*1024);
// to keep the calculation integer split
// 11.94 = 11.87  11.94/11.87 = 1.0058972199 11.87*1.0061532186 = 11.943038705
// 12.00/11.94  1.0050251256 11.94*1.0061532186 = 12.01346943
// 13.00/12.94  1.0046367852 12.94*1.0061532186 = 13.019622649
// 14.0/13.93  1.0050251256 13.94*1.0061532186 = 14.025775867
// 7.91 = 7.85  7.91/7.85 = 1.0076433121 7.85*1.0061532186 = 7.898302766
// 12.00/11.91  1.0075566751  11.91*1.0061532186 = 11.983284834
// 13.00/12.91  1.00697134   12.91*1.0061532186 = 12.989438052
// 14.00/13.91  1.0064701653  13.91*1.0061532186 = 13.99
// (1.0058972199+1.0050251256+1.0046367852+1.0050251256+1.0076433121+1.0075566751+1.00697134+1.0064701653)/8
// 1.0061532186
// 0.01537150931 * 1.0061532186 = 0.01546609357
// 11.99/11.93
#define VOLTAGE_SCALE  0.01546609357 // 5*(100+47)/(47*1024), calibrated for resistors.

#define ENGINE_BATTERY_VOLTAGE 2
#define SERVICE_BATTERY_VOLTAGE 1
#define ALTERNATOR_VOLTAGE 0

// Coolant Sensor calibration.
#define ADC_COOLANT_SENSOR 0
#define ADC_COOLANT_SUPPLY 7
#define COOLANT_SUPPLY_ADC_12V 780 // calibrated   12V supply Through a 47/100K divider 
                                   // 12*47/147=3.8367, calibrated at 3.8085 = ADC reading of 1024*3.8085/5=780
#define MIN_SUPPLY_VOLTAGE 261


// Fuel sensor calibration.
#define FUEL_RESISTOR 1000
#define ADC_READING_EMPTY 263 // measured for 190R
#define ADC_FUEL_SENSOR 1
#define FUEL_SENSOR_MAX 190
#define FUEL_CAPACITY 60


// pulses  Perms -> RPM conversion.
// 415Hz at idle below needs adjusting.//
// 415Hz == 850 rpm
// 
// pulses*850*1000/(duration)*415
//  2.054
// 1879/1930=0.9735751295    1.9997233161  
#define RPM_FACTOR 1.9997233161 // 2048.1927711 // 850000/415

// Flywheel pulse pin
#define PINS_FLYWHEEL 2



union EngineHoursStorage {
  uint8_t engineHoursBytes[4];
  uint32_t engineHoursPeriods;
};
union CRCStorage {
  uint8_t crcBytes[2];
  uint16_t crc;
};

class EngineSensors {
    public:

       EngineSensors(unsigned long flywheelReadPeriod=DEFAULT_FLYWHEEL_READ_PERIOD, 
                    unsigned long fuelReadPeriod=DEFAULT_FUEL_READ_PERIOD, 
                    unsigned long coolantReadPeriod=DEFAULT_COOLANT_READ_PERIOD, 
                    unsigned long voltageReadPeriod=DEFAULT_VOLTAGE_READ_PERIOD, 
                    unsigned long NTCReadPeriod=DEFAULT_NTC_READ_PERIOD):
                     flywheelReadPeriod{flywheelReadPeriod},
                     fuelReadPeriod{fuelReadPeriod},
                     coolantReadPeriod{coolantReadPeriod},
                     voltageReadPeriod{voltageReadPeriod},
                     NTCReadPeriod{NTCReadPeriod} {};
       bool begin();
       void read();
       void saveEngineHours();
       double getEngineRPM() { return engineRPM; };
       double getEngineSeconds() {  return 15L*engineHours.engineHoursPeriods; };
       double getCoolantTemperatureK() { return coolantTemperature+273.15; };
       double getEngineRoomTemperatureK() { return 0.1*temperature[ENGINEROOM_TEMP]+273.15; };
       double getAlternatorTemperatureK() { return 0.1*temperature[ALTERNATOR_TEMP]+273.15; };  
       double getExhaustTemperatureK() { return 0.1*temperature[EXHAUST_TEMP]+273.15; };  
       

       double getAlternatorVoltage() { return voltages[ALTERNATOR_VOLTAGE]; };
       double getServiceBatteryVoltage() { return voltages[SERVICE_BATTERY_VOLTAGE]; };
       double getEngineBatteryVoltage() { return voltages[ENGINE_BATTERY_VOLTAGE]; };
       double getFuelLevel() { return fuelLevel; }
       double getFuelCapacity() { return FUEL_CAPACITY; }

    private:
        void loadEngineHours();
        void readNTC();
        void readVoltages();
        void readCoolant();
        void readFuelLevel();
        void readEngineRPM();

        int16_t interpolate(
            int16_t reading, 
            int16_t minCurveValue, 
            int16_t maxCurveValue,
            int step, 
            const int16_t *curve, 
            int curveLength
            );
      
        EngineHoursStorage engineHours;
        uint32_t baseEngineHoursPeriods = 0;
        double engineRPM = 0;
        int fuelLevel = 0; // %
        int coolantTemperature = 0; // C in degrees, not interested in 0.1 of a degree.
        double voltages[3]; // in V
        int16_t temperature[3]; // degrees Cx10


        unsigned long lastFlywheelReadTime = 0;
        unsigned long lastFuelReadTime = 0;
        unsigned long lastCoolantReadTime = 0;
        unsigned long lastVotageRead = 0;
        unsigned long lastNTCRead = 0;
        unsigned long flywheelReadPeriod = DEFAULT_FLYWHEEL_READ_PERIOD;
        unsigned long fuelReadPeriod = DEFAULT_FUEL_READ_PERIOD;
        unsigned long coolantReadPeriod = DEFAULT_COOLANT_READ_PERIOD;
        unsigned long voltageReadPeriod = DEFAULT_VOLTAGE_READ_PERIOD;
        unsigned long NTCReadPeriod = DEFAULT_NTC_READ_PERIOD;
};

#endif