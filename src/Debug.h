/**
 * Debug.h - Debug utilities for Cortex Link A8R-M ESP32 IoT Smart Home Controller
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

// Set the default debug level here - reduced to minimize stack usage
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_LEVEL_ERROR
#endif

// Timer IDs
#define MAX_TIMERS 5

// Enable the DEBUG_LOG macro only if debugging is enabled
#if DEBUG_LEVEL > DEBUG_LEVEL_NONE
#define DEBUG_LOG(level, format, ...) Debug::log(level, format, ##__VA_ARGS__)
#else
#define DEBUG_LOG(level, format, ...)
#endif

// Debug macros for different levels
#if DEBUG_LEVEL >= DEBUG_LEVEL_ERROR
#define ERROR_LOG(format, ...) Debug::log(DEBUG_LEVEL_ERROR, format, ##__VA_ARGS__)
#else
#define ERROR_LOG(format, ...)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_WARNING
#define WARNING_LOG(format, ...) Debug::log(DEBUG_LEVEL_WARNING, format, ##__VA_ARGS__)
#else
#define WARNING_LOG(format, ...)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_INFO
#define INFO_LOG(format, ...) Debug::log(DEBUG_LEVEL_INFO, format, ##__VA_ARGS__)
#else
#define INFO_LOG(format, ...)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_DEBUG
#define DEBUG_LOG_MSG(format, ...) Debug::log(DEBUG_LEVEL_DEBUG, format, ##__VA_ARGS__)
#else
#define DEBUG_LOG_MSG(format, ...)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_TRACE
#define TRACE_LOG(format, ...) Debug::log(DEBUG_LEVEL_TRACE, format, ##__VA_ARGS__)
#else
#define TRACE_LOG(format, ...)
#endif

// Memory macros
#define LOG_MEMORY() Debug::logMemoryUsage()

class Debug {
public:
    // Initialize debugging
    static void begin(unsigned long baudRate = 115200);

    // Log message with specified level - simplified for stack safety
    static void log(uint8_t level, const char* format, ...);

    // Memory usage functions - simplified
    static void logMemoryUsage();

    // I2C device scanning - simplified
    static void scanI2CDevices(TwoWire& wire = Wire);

    // Debug helper functions
    static void debugAssert(bool condition, const char* message = nullptr);

private:
    static bool initialized;
    static const char* levelNames[];
};

#endif // DEBUG_H