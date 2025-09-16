#include "DACControl.h"

DACControl::DACControl() {
    currentVoltages[0] = 0.0;
    currentVoltages[1] = 0.0;
    currentCurrents[0] = 4.0;  // 4mA is minimum for current loop
    currentCurrents[1] = 4.0;
}

bool DACControl::begin() {
    // Initialize the I2C connection
    Wire.begin();
    
    // Initialize the DAC
    // The GP8413 library expects to be initialized without an address parameter
    // We'll need to initialize without specifying the address
    dac.begin();
    
    // Set the I2C address for subsequent communications
    // Since there's no setI2CAddress method, we'll write directly to the device
    // via the Wire library to set the configuration
    Wire.beginTransmission(GP8413_DAC_ADDR);
    Wire.write(0x02);  // Configuration register
    Wire.write(0x01);  // Enable DAC
    Wire.endTransmission();
    
    // Reset outputs to zero
    dac.setDACOutVoltage(0, 0.0);
    dac.setDACOutVoltage(1, 0.0);
    
    return true;
}

bool DACControl::setVoltage(uint8_t channel, float voltage) {
    if (channel > 1) {  // GP8413 has 2 channels (0 and 1)
        return false;
    }
    
    // Constrain voltage to 0-5V range
    voltage = constrain(voltage, 0.0, 5.0);
    
    // Set DAC output
    dac.setDACOutVoltage(channel, voltage);
    
    // Store current voltage
    currentVoltages[channel] = voltage;
    
    // Update current value if channel is used for current loop
    if (voltage <= 3.3) {  // Max voltage for 20mA (assuming 165 ohm resistor)
        float current = 4.0 + (voltage / 3.3) * 16.0;  // Scale to 4-20mA
        currentCurrents[channel] = current;
    }
    
    return true;
}

bool DACControl::setCurrent(uint8_t channel, float currentmA) {
    if (channel > 1) {  // GP8413 has 2 channels (0 and 1)
        return false;
    }
    
    // Constrain current to 4-20mA range
    currentmA = constrain(currentmA, 4.0, 20.0);
    
    // Calculate voltage needed for this current (V = I * R)
    // For 4-20mA with 165 ohm resistor, voltage range is ~0.66V to ~3.3V
    float voltage = (currentmA - 4.0) * (3.3 / 16.0);
    
    // Set DAC output
    dac.setDACOutVoltage(channel, voltage);
    
    // Store current values
    currentVoltages[channel] = voltage;
    currentCurrents[channel] = currentmA;
    
    return true;
}

float DACControl::getVoltage(uint8_t channel) {
    if (channel > 1) {
        return 0.0;
    }
    return currentVoltages[channel];
}

float DACControl::getCurrent(uint8_t channel) {
    if (channel > 1) {
        return 4.0;  // Minimum current loop value
    }
    return currentCurrents[channel];
}