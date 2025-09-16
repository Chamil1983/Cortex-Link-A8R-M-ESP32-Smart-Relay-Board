#ifndef RELAY_OUTPUTS_H
#define RELAY_OUTPUTS_H

#include <Arduino.h>
#include <MCP23017.h>
#include "Config.h"

class RelayOutputs {
public:
    RelayOutputs();
    bool begin();
    bool setRelay(uint8_t relayNum, bool state);
    bool toggleRelay(uint8_t relayNum);
    bool getRelayState(uint8_t relayNum);
    uint8_t getAllRelayStates();
    void setAllRelays(uint8_t states);

private:
    MCP23017 mcp;
    uint8_t relayStates;
};

#endif // RELAY_OUTPUTS_H