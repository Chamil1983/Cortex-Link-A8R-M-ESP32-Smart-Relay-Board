#ifndef DAC_CONTROL_H
#define DAC_CONTROL_H

#include <Arduino.h>
#include <DFRobot_GP8XXX.h>
#include "Config.h"

class DACControl {
public:
    DACControl();
    bool begin();
    
    // Set output voltage for a channel (0-5V)
    bool setVoltage(uint8_t channel, float voltage);
    
    // Set output current for a channel (4-20mA)
    bool setCurrent(uint8_t channel, float currentmA);
    
    // Get currently set values
    float getVoltage(uint8_t channel);
    float getCurrent(uint8_t channel);

private:
    DFRobot_GP8413 dac;
    float currentVoltages[2];
    float currentCurrents[2];
};

#endif // DAC_CONTROL_H