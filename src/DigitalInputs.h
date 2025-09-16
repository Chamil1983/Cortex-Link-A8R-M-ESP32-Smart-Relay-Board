#ifndef DIGITAL_INPUTS_H
#define DIGITAL_INPUTS_H

#include <Arduino.h>
#include <MCP23017.h>
#include "Config.h"

class DigitalInputs {
public:
    DigitalInputs();
    bool begin();
    bool readInput(uint8_t inputNum);
    uint8_t readAllInputs();
    void attachInterrupt(void (*callback)());
    bool inputChanged();
    void clearInterrupt();

    // Provide access to MCP23017 for Ethernet reset
    MCP23017& getMCP() { return mcp; }

private:
    MCP23017 mcp;
    uint8_t lastInputState;
    bool interruptOccurred;
    void setupInterrupts();
};

#endif // DIGITAL_INPUTS_H