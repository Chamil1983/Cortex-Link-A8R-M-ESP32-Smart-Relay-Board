#include "DHT_Sensors.h"
#include "Debug.h"

DHTSensors::DHTSensors() : lastReadTime(0), lastDS18B20ReadTime(0), ds18b20Count(0) {
    // Initialize DHT sensor pins
    sensorPins[0] = PIN_DHT_SENSOR1;
    sensorPins[1] = PIN_DHT_SENSOR2;
    
    // Initialize DHT arrays
    for (uint8_t i = 0; i < NUM_DHT_SENSORS; i++) {
        temperatures[i] = 0.0;
        humidities[i] = 0.0;
        sensorConnected[i] = false;
        dhtSensors[i] = nullptr;
    }
    
    // Initialize DS18B20 arrays
    oneWire = nullptr;
    ds18b20Sensors = nullptr;
    for (uint8_t i = 0; i < MAX_DS18B20_SENSORS; i++) {
        ds18b20Temperatures[i] = -127.0; // Default value when sensor not present
        for (uint8_t j = 0; j < 8; j++) {
            ds18b20Addresses[i][j] = 0;
        }
    }
}

DHTSensors::~DHTSensors() {
    // Clean up DHT sensors
    for (uint8_t i = 0; i < NUM_DHT_SENSORS; i++) {
        if (dhtSensors[i] != nullptr) {
            delete dhtSensors[i];
            dhtSensors[i] = nullptr;
        }
    }
    
    // Clean up DS18B20 sensors
    if (ds18b20Sensors != nullptr) {
        delete ds18b20Sensors;
        ds18b20Sensors = nullptr;
    }
    
    if (oneWire != nullptr) {
        delete oneWire;
        oneWire = nullptr;
    }
}

bool DHTSensors::begin() {
    bool success = true;
    
    // Initialize DHT sensors
    INFO_LOG("Initializing DHT sensors");
    for (uint8_t i = 0; i < NUM_DHT_SENSORS; i++) {
        if (dhtSensors[i] != nullptr) {
            delete dhtSensors[i];
        }
        dhtSensors[i] = new DHT(sensorPins[i], DHT22);
        dhtSensors[i]->begin();
        INFO_LOG("  DHT sensor %d initialized on pin %d", i+1, sensorPins[i]);
    }
    
    // Initialize DS18B20 sensors
    INFO_LOG("Initializing DS18B20 sensors");
    oneWire = new OneWire(PIN_DS18B20);
    ds18b20Sensors = new DallasTemperature(oneWire);
    ds18b20Sensors->begin();
    
    // Search for DS18B20 devices
    ds18b20Count = ds18b20Sensors->getDeviceCount();
    INFO_LOG("  Found %d DS18B20 sensors on pin %d", ds18b20Count, PIN_DS18B20);
    
    if (ds18b20Count > MAX_DS18B20_SENSORS) {
        WARNING_LOG("  More DS18B20 sensors found (%d) than maximum allowed (%d)", 
                    ds18b20Count, MAX_DS18B20_SENSORS);
        ds18b20Count = MAX_DS18B20_SENSORS;
    }
    
    // Get addresses for all DS18B20 sensors
    for (uint8_t i = 0; i < ds18b20Count; i++) {
        if (ds18b20Sensors->getAddress(ds18b20Addresses[i], i)) {
            INFO_LOG("  DS18B20 sensor %d address: ", i+1);
            printDS18B20Address(i);
            
            // Set resolution for all sensors (9-12 bits)
            ds18b20Sensors->setResolution(ds18b20Addresses[i], 12);
        } else {
            ERROR_LOG("  Failed to get address for DS18B20 sensor %d", i+1);
            success = false;
        }
    }
    
    // Initial reading
    update();
    
    return success;
}

float DHTSensors::getTemperature(uint8_t sensorNum) {
    if (sensorNum >= NUM_DHT_SENSORS) {
        return -999.0;  // Invalid sensor number
    }
    
    return temperatures[sensorNum];
}

float DHTSensors::getHumidity(uint8_t sensorNum) {
    if (sensorNum >= NUM_DHT_SENSORS) {
        return -999.0;  // Invalid sensor number
    }
    
    return humidities[sensorNum];
}

bool DHTSensors::isSensorConnected(uint8_t sensorNum) {
    if (sensorNum >= NUM_DHT_SENSORS) {
        return false;
    }
    
    return sensorConnected[sensorNum];
}

void DHTSensors::update() {
    unsigned long currentTime = millis();
    
    // Update DHT sensors
    if (currentTime - lastReadTime >= READ_INTERVAL) {
        lastReadTime = currentTime;
        
        // Read all DHT sensors
        for (uint8_t i = 0; i < NUM_DHT_SENSORS; i++) {
            if (dhtSensors[i] == nullptr) continue;
            
            // Read temperature and check if reading is valid
            float t = dhtSensors[i]->readTemperature();
            if (!isnan(t)) {
                temperatures[i] = t;
                sensorConnected[i] = true;
            } else {
                sensorConnected[i] = false;
            }
            
            // Read humidity and check if reading is valid
            float h = dhtSensors[i]->readHumidity();
            if (!isnan(h)) {
                humidities[i] = h;
            }
        }
    }
    
    // Update DS18B20 sensors
    if (currentTime - lastDS18B20ReadTime >= DS18B20_READ_INTERVAL) {
        lastDS18B20ReadTime = currentTime;
        
        if (ds18b20Sensors != nullptr && ds18b20Count > 0) {
            // Request temperature readings from all sensors
            ds18b20Sensors->requestTemperatures();
            
            // Read temperatures for all sensors
            for (uint8_t i = 0; i < ds18b20Count; i++) {
                ds18b20Temperatures[i] = ds18b20Sensors->getTempC(ds18b20Addresses[i]);
            }
        }
    }
}

uint8_t DHTSensors::getDS18B20Count() {
    return ds18b20Count;
}

float DHTSensors::getDS18B20Temperature(uint8_t index) {
    if (index >= ds18b20Count) {
        return -127.0; // Invalid sensor index
    }
    
    return ds18b20Temperatures[index];
}

DeviceAddress* DHTSensors::getDS18B20Address(uint8_t index) {
    if (index >= ds18b20Count) {
        return nullptr;
    }
    
    return &ds18b20Addresses[index];
}

bool DHTSensors::isDS18B20Connected(uint8_t index) {
    if (index >= ds18b20Count) {
        return false;
    }
    
    // DS18B20 returns -127.0 when sensor is disconnected
    return ds18b20Temperatures[index] > -127.0;
}

void DHTSensors::printDS18B20Address(uint8_t index) {
    if (index >= ds18b20Count) {
        Serial.println("Invalid DS18B20 index");
        return;
    }
    
    Serial.print("0x");
    for (uint8_t i = 0; i < 8; i++) {
        if (ds18b20Addresses[index][i] < 16) {
            Serial.print("0");
        }
        Serial.print(ds18b20Addresses[index][i], HEX);
    }
    Serial.println();
}

void DHTSensors::setDS18B20Resolution(uint8_t resolution) {
    if (ds18b20Sensors != nullptr) {
        // Constrain resolution to valid values (9-12 bits)
        resolution = constrain(resolution, 9, 12);
        
        // Set resolution for all sensors
        for (uint8_t i = 0; i < ds18b20Count; i++) {
            ds18b20Sensors->setResolution(ds18b20Addresses[i], resolution);
        }
        
        INFO_LOG("DS18B20 resolution set to %d bits", resolution);
    }
}