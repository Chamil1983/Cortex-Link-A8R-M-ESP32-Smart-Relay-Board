#ifndef DHT_SENSORS_H
#define DHT_SENSORS_H

#include <Arduino.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Config.h"

class DHTSensors {
public:
    DHTSensors();
    ~DHTSensors();
    bool begin();
    
    // Read temperature and humidity from DHT sensors
    float getTemperature(uint8_t sensorNum);
    float getHumidity(uint8_t sensorNum);
    
    // Check if sensor is connected and working
    bool isSensorConnected(uint8_t sensorNum);
    
    // Update sensor readings
    void update();
    
    // DS18B20 Temperature Sensor Functions
    uint8_t getDS18B20Count();
    float getDS18B20Temperature(uint8_t index);
    DeviceAddress* getDS18B20Address(uint8_t index);
    bool isDS18B20Connected(uint8_t index);
    void printDS18B20Address(uint8_t index);
    void setDS18B20Resolution(uint8_t resolution = 12); // 9-12 bits

private:
    // DHT sensor variables
    DHT* dhtSensors[NUM_DHT_SENSORS];
    uint8_t sensorPins[NUM_DHT_SENSORS];
    float temperatures[NUM_DHT_SENSORS];
    float humidities[NUM_DHT_SENSORS];
    bool sensorConnected[NUM_DHT_SENSORS];
    unsigned long lastReadTime;
    const unsigned long READ_INTERVAL = 2000; // 2 seconds between readings
    
    // DS18B20 variables
    OneWire* oneWire;
    DallasTemperature* ds18b20Sensors;
    DeviceAddress ds18b20Addresses[MAX_DS18B20_SENSORS];
    float ds18b20Temperatures[MAX_DS18B20_SENSORS];
    uint8_t ds18b20Count;
    unsigned long lastDS18B20ReadTime;
    const unsigned long DS18B20_READ_INTERVAL = 1000; // 1 second between readings
};

#endif // DHT_SENSORS_H