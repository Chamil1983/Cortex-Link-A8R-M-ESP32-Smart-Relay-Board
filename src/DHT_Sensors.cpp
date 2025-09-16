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
    for (uint8_t i = 0; i < NUM_DHT_SENSORS; i++) {
        if (dhtSensors[i] != nullptr) {
            delete dhtSensors[i];
        }
        dhtSensors[i] = new DHT(sensorPins[i], DHT22);
        dhtSensors[i]->begin();
    }

    // Initialize DS18B20 sensors - with retry
    for (int attempt = 0; attempt < 3; attempt++) {
        oneWire = new OneWire(PIN_DS18B20);
        if (oneWire) {
            ds18b20Sensors = new DallasTemperature(oneWire);
            if (ds18b20Sensors) {
                ds18b20Sensors->begin();

                // Search for DS18B20 devices
                ds18b20Count = ds18b20Sensors->getDeviceCount();

                if (ds18b20Count > 0) {
                    // Limit to maximum
                    if (ds18b20Count > MAX_DS18B20_SENSORS) {
                        ds18b20Count = MAX_DS18B20_SENSORS;
                    }

                    // Get addresses
                    for (uint8_t i = 0; i < ds18b20Count; i++) {
                        if (ds18b20Sensors->getAddress(ds18b20Addresses[i], i)) {
                            // Set to 10-bit resolution for faster conversion
                            ds18b20Sensors->setResolution(ds18b20Addresses[i], 10);
                        }
                    }

                    ERROR_LOG("Found %d DS18B20", ds18b20Count);
                    break; // Success!
                }
            }
        }

        // Cleanup on failure
        if (ds18b20Sensors) {
            delete ds18b20Sensors;
            ds18b20Sensors = nullptr;
        }
        if (oneWire) {
            delete oneWire;
            oneWire = nullptr;
        }

        delay(100); // Wait before retrying
    }

    // Initial reading
    update();
    return success;
}

float DHTSensors::getTemperature(uint8_t sensorNum) {
    if (sensorNum >= NUM_DHT_SENSORS) {
        return -999.0;
    }
    return temperatures[sensorNum];
}

float DHTSensors::getHumidity(uint8_t sensorNum) {
    if (sensorNum >= NUM_DHT_SENSORS) {
        return -999.0;
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

        for (uint8_t i = 0; i < NUM_DHT_SENSORS; i++) {
            if (dhtSensors[i] == nullptr) continue;

            float t = dhtSensors[i]->readTemperature();
            if (!isnan(t)) {
                temperatures[i] = t;
                sensorConnected[i] = true;
            }
            else {
                sensorConnected[i] = false;
            }

            float h = dhtSensors[i]->readHumidity();
            if (!isnan(h)) {
                humidities[i] = h;
            }
        }
    }

    // Update DS18B20 sensors
    if (ds18b20Count > 0 && ds18b20Sensors != nullptr &&
        (currentTime - lastDS18B20ReadTime >= DS18B20_READ_INTERVAL)) {

        lastDS18B20ReadTime = currentTime;

        // Request temperatures (with timeout protection)
        unsigned long startTime = millis();
        ds18b20Sensors->requestTemperatures();

        // Read temperatures for all sensors
        for (uint8_t i = 0; i < ds18b20Count; i++) {
            ds18b20Temperatures[i] = ds18b20Sensors->getTempC(ds18b20Addresses[i]);

            // Guard against excessive time in this loop
            if (millis() - startTime > 1000) {
                break; // Emergency timeout
            }
        }
    }
}

uint8_t DHTSensors::getDS18B20Count() {
    return ds18b20Count;
}

float DHTSensors::getDS18B20Temperature(uint8_t index) {
    if (index >= ds18b20Count) {
        return -127.0;
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
    return ds18b20Temperatures[index] > -127.0;
}

void DHTSensors::printDS18B20Address(uint8_t index) {
    if (index >= ds18b20Count) {
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
    if (ds18b20Sensors != nullptr && ds18b20Count > 0) {
        resolution = constrain(resolution, 9, 12);
        for (uint8_t i = 0; i < ds18b20Count; i++) {
            ds18b20Sensors->setResolution(ds18b20Addresses[i], resolution);
        }
    }
}