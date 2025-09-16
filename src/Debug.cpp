/**
 * Debug.cpp - Implementation of Debug utilities
 */

#include "Debug.h"
#include <stdarg.h>

 // Initialize static variables
bool Debug::initialized = false;
const char* Debug::levelNames[] = { "NONE", "ERROR", "WARNING", "INFO", "DEBUG", "TRACE" };

void Debug::begin(unsigned long baudRate) {
    if (!initialized) {
        Serial.begin(baudRate);
        delay(50);
        Serial.println("Debug started");
        initialized = true;
    }
}

void Debug::log(uint8_t level, const char* format, ...) {
    if (!initialized) {
        begin();
    }

    if (level <= DEBUG_LEVEL) {
        // Use a small buffer to prevent stack overflow
        char buffer[64];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        if (level == DEBUG_LEVEL_ERROR) {
            Serial.print("ERROR: ");
        }

        Serial.println(buffer);
    }
}

void Debug::logMemoryUsage() {
#ifdef ESP32
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
#endif
}

void Debug::scanI2CDevices(TwoWire& wire) {
    Serial.println("I2C scan:");

    uint8_t deviceCount = 0;
    for (uint8_t address = 1; address < 127; address++) {
        wire.beginTransmission(address);
        uint8_t error = wire.endTransmission();

        if (error == 0) {
            Serial.print("  Device at 0x");
            Serial.println(address, HEX);
            deviceCount++;
        }
    }

    Serial.print("Found ");
    Serial.print(deviceCount);
    Serial.println(" devices");
}

void Debug::debugAssert(bool condition, const char* message) {
    if (!condition && message) {
        Serial.print("ASSERT: ");
        Serial.println(message);
    }
}