#include "AnalogInputs.h"

AnalogInputs::AnalogInputs() {
    // Initialize pin arrays
    voltageChannelPins[0] = PIN_ANALOG_CH1;
    voltageChannelPins[1] = PIN_ANALOG_CH2;
    
    currentChannelPins[0] = PIN_CURRENT_CH1;
    currentChannelPins[1] = PIN_CURRENT_CH2;
}

void AnalogInputs::begin() {
    // Set ADC resolution to 12 bits (0-4095)
    analogReadResolution(12);
}

uint16_t AnalogInputs::readRawVoltageChannel(uint8_t channel) {
    if (channel >= NUM_ANALOG_CHANNELS) {
        return 0;
    }
    return analogRead(voltageChannelPins[channel]);
}

uint16_t AnalogInputs::readRawCurrentChannel(uint8_t channel) {
    if (channel >= NUM_CURRENT_CHANNELS) {
        return 0;
    }
    return analogRead(currentChannelPins[channel]);
}

float AnalogInputs::readVoltage(uint8_t channel) {
    if (channel >= NUM_ANALOG_CHANNELS) {
        return 0.0;
    }
    
    uint16_t rawValue = readRawVoltageChannel(channel);
    return rawToVoltage(rawValue);
}

float AnalogInputs::readCurrent(uint8_t channel) {
    if (channel >= NUM_CURRENT_CHANNELS) {
        return 0.0;
    }
    
    uint16_t rawValue = readRawCurrentChannel(channel);
    return rawToCurrent(rawValue);
}

float AnalogInputs::getAverageVoltage(uint8_t channel, uint8_t samples) {
    if (channel >= NUM_ANALOG_CHANNELS) {
        return 0.0;
    }
    
    float sum = 0.0;
    for (uint8_t i = 0; i < samples; i++) {
        sum += readVoltage(channel);
        delay(2);  // Short delay between samples
    }
    
    return sum / samples;
}

float AnalogInputs::getAverageCurrent(uint8_t channel, uint8_t samples) {
    if (channel >= NUM_CURRENT_CHANNELS) {
        return 0.0;
    }
    
    float sum = 0.0;
    for (uint8_t i = 0; i < samples; i++) {
        sum += readCurrent(channel);
        delay(2);  // Short delay between samples
    }
    
    return sum / samples;
}

float AnalogInputs::rawToVoltage(uint16_t rawValue) {
    // Convert raw ADC value to voltage (0-5V)
    // ESP32 ADC is non-linear, so this is a simplified conversion
    // For more accurate results, consider using a calibration curve
    float voltage = (rawValue / (float)ADC_RESOLUTION) * ADC_VOLTAGE_REF;
    return voltage * (5.0 / ADC_VOLTAGE_REF); // Scale to 0-5V range
}

float AnalogInputs::rawToCurrent(uint16_t rawValue) {
    // Convert raw ADC value to current (4-20mA)
    // First convert to voltage across the resistor
    float voltage = (rawValue / (float)ADC_RESOLUTION) * ADC_VOLTAGE_REF;
    
    // Convert voltage to current (I = V/R) and scale to 4-20mA range
    float current = voltage * 1000.0 / CURRENT_LOOP_RESISTOR;  // Convert to mA
    
    // Apply calibration and scaling to 4-20mA range
    // This is a simplified conversion - you might need to calibrate
    float scaledCurrent = 4.0 + (current / (ADC_VOLTAGE_REF / CURRENT_LOOP_RESISTOR)) * 16.0;
    
    return scaledCurrent;
}