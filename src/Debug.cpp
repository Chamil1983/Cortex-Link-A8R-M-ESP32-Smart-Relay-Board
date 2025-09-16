/**
 * Debug.cpp - Implementation of Debug utilities
 */

#include "Debug.h"
#include <stdarg.h>

// Initialize static variables
bool Debug::initialized = false;
unsigned long Debug::timerStartTimes[MAX_TIMERS] = {0};
const char* Debug::levelNames[] = {"NONE", "ERROR", "WARNING", "INFO", "DEBUG", "TRACE"};

void Debug::begin(unsigned long baudRate) {
    if (!initialized) {
        Serial.begin(baudRate);
        delay(100);
        Serial.println("\n\n--- Debug Utility Initialized ---");
        
        // Log initial system status
        logMemoryUsage("Initial");
        
        initialized = true;
    }
}

void Debug::log(uint8_t level, const char* format, ...) {
    if (!initialized) {
        begin();
    }
    
    if (level <= DEBUG_LEVEL) {
        // Print timestamp
        unsigned long ms = millis();
        unsigned long seconds = ms / 1000;
        unsigned long minutes = seconds / 60;
        unsigned long hours = minutes / 60;
        
        Serial.printf("[%02lu:%02lu:%02lu.%03lu] [%s] ", 
                     hours % 24, minutes % 60, seconds % 60, ms % 1000,
                     getLevelName(level));
        
        // Print formatted message
        va_list args;
        va_start(args, format);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        Serial.println(buffer);
    }
}

void Debug::startTimer(uint8_t timerID) {
    if (timerID < MAX_TIMERS) {
        timerStartTimes[timerID] = micros();
    }
}

void Debug::stopTimer(uint8_t timerID, const char* label) {
    if (timerID < MAX_TIMERS) {
        unsigned long duration = micros() - timerStartTimes[timerID];
        
        if (label) {
            log(DEBUG_LEVEL_DEBUG, "Timer %d (%s): %lu μs", timerID, label, duration);
        } else {
            log(DEBUG_LEVEL_DEBUG, "Timer %d: %lu μs", timerID, duration);
        }
    }
}

void Debug::logMemoryUsage(const char* label) {
#ifdef ESP32
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    float percentUsed = 100.0 * (float)(totalHeap - freeHeap) / (float)totalHeap;
    
    if (label) {
        log(DEBUG_LEVEL_INFO, "Memory (%s): %u bytes free, %u bytes total (%.1f%% used)", 
            label, freeHeap, totalHeap, percentUsed);
    } else {
        log(DEBUG_LEVEL_INFO, "Memory: %u bytes free, %u bytes total (%.1f%% used)", 
            freeHeap, totalHeap, percentUsed);
    }
#else
    // For non-ESP32 platforms
    if (label) {
        log(DEBUG_LEVEL_INFO, "Memory usage (%s): Feature not supported on this platform", label);
    } else {
        log(DEBUG_LEVEL_INFO, "Memory usage: Feature not supported on this platform");
    }
#endif
}

void Debug::scanI2CDevices(TwoWire &wire) {
    log(DEBUG_LEVEL_INFO, "Scanning I2C bus for devices...");
    
    uint8_t deviceCount = 0;
    for (uint8_t address = 1; address < 127; address++) {
        wire.beginTransmission(address);
        uint8_t error = wire.endTransmission();
        
        if (error == 0) {
            log(DEBUG_LEVEL_INFO, "  I2C device found at address 0x%02X", address);
            deviceCount++;
        } else if (error == 4) {
            log(DEBUG_LEVEL_WARNING, "  Error accessing I2C device at address 0x%02X", address);
        }
    }
    
    if (deviceCount == 0) {
        log(DEBUG_LEVEL_WARNING, "No I2C devices found");
    } else {
        log(DEBUG_LEVEL_INFO, "Found %d I2C device(s)", deviceCount);
    }
}

void Debug::setupModbusLogging() {
    log(DEBUG_LEVEL_INFO, "Modbus logging setup");
}

void Debug::logModbusRegister(uint16_t address, uint16_t value, bool isHolding) {
    const char* regType = isHolding ? "Holding" : "Input";
    log(DEBUG_LEVEL_DEBUG, "Modbus %s Register: Address=%u, Value=%u (0x%04X)", 
        regType, address, value, value);
}

void Debug::logPinState(uint8_t pin) {
    int state = digitalRead(pin);
    log(DEBUG_LEVEL_DEBUG, "Pin %u state: %d", pin, state);
}

void Debug::logAllPins(uint8_t startPin, uint8_t endPin) {
    log(DEBUG_LEVEL_DEBUG, "Pins %u-%u states:", startPin, endPin);
    
    for (uint8_t pin = startPin; pin <= endPin; pin++) {
        pinMode(pin, INPUT); // Set as input to read state
        int state = digitalRead(pin);
        log(DEBUG_LEVEL_DEBUG, "  Pin %u: %d", pin, state);
    }
}

void Debug::hexDump(const uint8_t* data, size_t length) {
    if (data == nullptr || length == 0) {
        log(DEBUG_LEVEL_WARNING, "Hex dump: Invalid data");
        return;
    }
    
    log(DEBUG_LEVEL_DEBUG, "Hex dump of %u bytes:", length);
    
    char line[80];
    for (size_t i = 0; i < length; i += 16) {
        int pos = snprintf(line, sizeof(line), "  %04X: ", i);
        
        for (size_t j = 0; j < 16; j++) {
            if (i + j < length) {
                pos += snprintf(line + pos, sizeof(line) - pos, "%02X ", data[i + j]);
            } else {
                pos += snprintf(line + pos, sizeof(line) - pos, "   ");
            }
        }
        
        pos += snprintf(line + pos, sizeof(line) - pos, " | ");
        
        for (size_t j = 0; j < 16; j++) {
            if (i + j < length) {
                char c = data[i + j];
                if (c < 32 || c > 126) c = '.'; // Non-printable
                pos += snprintf(line + pos, sizeof(line) - pos, "%c", c);
            }
        }
        
        Serial.println(line);
    }
}

void Debug::asciiDump(const uint8_t* data, size_t length) {
    if (data == nullptr || length == 0) {
        log(DEBUG_LEVEL_WARNING, "ASCII dump: Invalid data");
        return;
    }
    
    log(DEBUG_LEVEL_DEBUG, "ASCII dump of %u bytes:", length);
    
    char line[80];
    char* pos = line;
    for (size_t i = 0; i < length; i++) {
        char c = data[i];
        if (c < 32 || c > 126) { // Non-printable
            pos += snprintf(pos, sizeof(line) - (pos - line), "\\%03o", c);
        } else {
            *pos++ = c;
            *pos = '\0';
        }
        
        if (pos - line > 70 || i == length - 1) {
            Serial.println(line);
            pos = line;
            *pos = '\0';
        }
    }
}

void Debug::assert(bool condition, const char* message) {
    if (!condition) {
        if (message) {
            log(DEBUG_LEVEL_ERROR, "ASSERTION FAILED: %s", message);
        } else {
            log(DEBUG_LEVEL_ERROR, "ASSERTION FAILED");
        }
        
        // Log additional diagnostic info
        logMemoryUsage("On assertion failure");
        
        // Infinite loop to halt execution on critical error
        while (1) {
            delay(1000);
        }
    }
}

const char* Debug::getLevelName(uint8_t level) {
    if (level <= DEBUG_LEVEL_TRACE) {
        return levelNames[level];
    }
    return "UNKNOWN";
}