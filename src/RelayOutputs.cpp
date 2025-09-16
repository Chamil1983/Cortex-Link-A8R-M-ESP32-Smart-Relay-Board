#include "RelayOutputs.h"

RelayOutputs::RelayOutputs() : relayStates(0) {
}

bool RelayOutputs::begin() {
    // Initialize MCP23017 for relay outputs
    mcp.begin(MCP23017_OUTPUT_ADDR);
    
    // Configure GPA0-GPA5 as outputs for the 6 relays
    for (uint8_t pin = 0; pin < NUM_RELAY_OUTPUTS; pin++) {
        mcp.pinMode(pin, OUTPUT);
        mcp.digitalWrite(pin, HIGH);  // Initialize all relays to OFF
    }
    
    relayStates = 0;
    return true;
}

bool RelayOutputs::setRelay(uint8_t relayNum, bool state) {
    if (relayNum >= NUM_RELAY_OUTPUTS) {
        return false;
    }
    
    // Set relay state
    mcp.digitalWrite(relayNum, state ? HIGH : LOW);
    
    // Update relay states
    if (state) {
        relayStates |= (1 << relayNum);
    } else {
        relayStates &= ~(1 << relayNum);
    }
    
    return true;
}

bool RelayOutputs::toggleRelay(uint8_t relayNum) {
    if (relayNum >= NUM_RELAY_OUTPUTS) {
        return false;
    }
    
    bool currentState = getRelayState(relayNum);
    return setRelay(relayNum, !currentState);
}

bool RelayOutputs::getRelayState(uint8_t relayNum) {
    if (relayNum >= NUM_RELAY_OUTPUTS) {
        return false;
    }
    
    return (relayStates & (1 << relayNum)) != 0;
}

uint8_t RelayOutputs::getAllRelayStates() {
    return relayStates;
}

void RelayOutputs::setAllRelays(uint8_t states) {
    // Set all relays according to the bit pattern in 'states'
    for (uint8_t i = 0; i < NUM_RELAY_OUTPUTS; i++) {
        bool state = (states & (1 << i)) != 0;
        mcp.digitalWrite(i, state ? HIGH : LOW);
    }
    
    // Update relay states
    relayStates = states & ((1 << NUM_RELAY_OUTPUTS) - 1);  // Mask to valid relays only
}