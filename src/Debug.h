/**
 * Debug.h - Debug utilities for Cortex Link A8R-M ESP32 IoT Smart Home Controller
 * 
 * This class provides debugging utilities including:
 * - Console output with different levels (ERROR, WARNING, INFO, DEBUG, TRACE)
 * - Performance timing measurements
 * - Memory usage reporting
 * - I2C device scanning
 * - Variable monitoring
 * - Conditional compilation for debug vs. release builds
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>
#include <Wire.h>

// Debug levels
#define DEBUG_LEVEL_NONE    0
#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_WARNING 2
#define DEBUG_LEVEL_INFO    3
#define DEBUG_LEVEL_DEBUG   4
#define DEBUG_LEVEL_TRACE   5

// Set the default debug level here
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_LEVEL_INFO
#endif

// Enable or disable timing measurements
#ifndef DEBUG_TIMING_ENABLED
#define DEBUG_TIMING_ENABLED 1
#endif

// Enable or disable memory usage reports
#ifndef DEBUG_MEMORY_ENABLED
#define DEBUG_MEMORY_ENABLED 1
#endif

// Timer IDs
#define MAX_TIMERS 10

// Enable the DEBUG_LOG macro only if debugging is enabled
#if DEBUG_LEVEL > DEBUG_LEVEL_NONE
#define DEBUG_LOG(level, format, ...) Debug::log(level, format, ##__VA_ARGS__)
#else
#define DEBUG_LOG(level, format, ...)
#endif

// Debug macros for different levels
#if DEBUG_LEVEL >= DEBUG_LEVEL_ERROR
#define ERROR_LOG(format, ...) DEBUG_LOG(DEBUG_LEVEL_ERROR, format, ##__VA_ARGS__)
#else
#define ERROR_LOG(format, ...)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_WARNING
#define WARNING_LOG(format, ...) DEBUG_LOG(DEBUG_LEVEL_WARNING, format, ##__VA_ARGS__)
#else
#define WARNING_LOG(format, ...)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_INFO
#define INFO_LOG(format, ...) DEBUG_LOG(DEBUG_LEVEL_INFO, format, ##__VA_ARGS__)
#else
#define INFO_LOG(format, ...)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_DEBUG
#define DEBUG_LOG_MSG(format, ...) DEBUG_LOG(DEBUG_LEVEL_DEBUG, format, ##__VA_ARGS__)
#else
#define DEBUG_LOG_MSG(format, ...)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_TRACE
#define TRACE_LOG(format, ...) DEBUG_LOG(DEBUG_LEVEL_TRACE, format, ##__VA_ARGS__)
#else
#define TRACE_LOG(format, ...)
#endif

// Timer macros
#if DEBUG_TIMING_ENABLED
#define START_TIMER(id) Debug::startTimer(id)
#define STOP_TIMER(id, label) Debug::stopTimer(id, label)
#else
#define START_TIMER(id)
#define STOP_TIMER(id, label)
#endif

// Memory macros
#if DEBUG_MEMORY_ENABLED
#define LOG_MEMORY(label) Debug::logMemoryUsage(label)
#else
#define LOG_MEMORY(label)
#endif

class Debug {
public:
    // Initialize debugging
    static void begin(unsigned long baudRate = 115200);
    
    // Log message with specified level
    static void log(uint8_t level, const char* format, ...);
    
    // Performance timing functions
    static void startTimer(uint8_t timerID);
    static void stopTimer(uint8_t timerID, const char* label = nullptr);
    
    // Memory usage functions
    static void logMemoryUsage(const char* label = nullptr);
    
    // I2C device scanning
    static void scanI2CDevices(TwoWire &wire = Wire);
    
    // Modbus debugging
    static void setupModbusLogging();
    static void logModbusRegister(uint16_t address, uint16_t value, bool isHolding);
    
    // GPIO pin state monitoring
    static void logPinState(uint8_t pin);
    static void logAllPins(uint8_t startPin, uint8_t endPin);
    
    // Serial data monitoring
    static void hexDump(const uint8_t* data, size_t length);
    static void asciiDump(const uint8_t* data, size_t length);
    
    // Assertion function
    static void assert(bool condition, const char* message = nullptr);
    
private:
    static bool initialized;
    static unsigned long timerStartTimes[MAX_TIMERS];
    static const char* levelNames[];
    
    // Convert level to human-readable string
    static const char* getLevelName(uint8_t level);
};

#endif // DEBUG_H