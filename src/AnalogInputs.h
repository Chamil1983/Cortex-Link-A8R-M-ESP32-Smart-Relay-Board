#ifndef ANALOG_INPUTS_H
#define ANALOG_INPUTS_H

#include <Arduino.h>
#include "Config.h"

class AnalogInputs {
public:
    AnalogInputs();
    void begin();
    
    // Read raw ADC values
    uint16_t readRawVoltageChannel(uint8_t channel);
    uint16_t readRawCurrentChannel(uint8_t channel);
    
    // Read scaled values
    float readVoltage(uint8_t channel);     // Returns value in volts (0-5V)
    float readCurrent(uint8_t channel);     // Returns value in mA (4-20mA)
    
    // Advanced functions
    float getAverageVoltage(uint8_t channel, uint8_t samples = 10);
    float getAverageCurrent(uint8_t channel, uint8_t samples = 10);
    
private:
    uint8_t voltageChannelPins[NUM_ANALOG_CHANNELS];
    uint8_t currentChannelPins[NUM_CURRENT_CHANNELS];
    
    // Scaling functions
    float rawToVoltage(uint16_t rawValue);
    float rawToCurrent(uint16_t rawValue);
};

#endif // ANALOG_INPUTS_H