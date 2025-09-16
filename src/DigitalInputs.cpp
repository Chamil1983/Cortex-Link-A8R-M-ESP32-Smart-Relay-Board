#include "DigitalInputs.h"

DigitalInputs::DigitalInputs() : lastInputState(0), interruptOccurred(false) {
}

bool DigitalInputs::begin() {
    // Initialize MCP23017 for digital inputs
    mcp.begin(MCP23017_INPUT_ADDR);
    
    // Configure all PORTB pins as inputs with pull-ups
    for (uint8_t pin = 0; pin < 8; pin++) {
        mcp.pinMode(pin + 8, INPUT_PULLUP);  // PORTB pins are 8-15
    }
    
    // Setup interrupt handling for MCP23017
    setupInterrupts();
    
    // Read initial state
    lastInputState = mcp.readPort(MCP23017Port::B);
    
    return true;
}

bool DigitalInputs::readInput(uint8_t inputNum) {
    if (inputNum >= NUM_DIGITAL_INPUTS) {
        return false;
    }
    
    // Read specific input from PORTB (GPB0-GPB7)
    // Invert value since inputs are active LOW (pulled up when not activated)
    return !mcp.digitalRead(inputNum + 8);
}

uint8_t DigitalInputs::readAllInputs() {
    // Read all PORTB inputs and invert since inputs are active LOW
    uint8_t portValue = mcp.readPort(MCP23017Port::B);
    lastInputState = portValue;
    return ~portValue & 0xFF;  // Invert all bits and mask to 8 bits
}

void DigitalInputs::setupInterrupts() {
    // Configure MCP23017 interrupts
    mcp.interruptMode(MCP23017InterruptMode::Separated);
    
    // Since the library doesn't have interruptOnChangePort or interruptOnChange methods,
    // we'll manually configure the interrupt control registers
    
    // Set INTCON register for PORT B to compare against previous value (0x00)
    // and DEFVAL register to 0x00 (not used when comparing against previous)
    Wire.beginTransmission(MCP23017_INPUT_ADDR);
    Wire.write(0x08);  // INTCONB register
    Wire.write(0x00);  // 0 = compare against previous value
    Wire.endTransmission();
    
    // Enable interrupts on all pins of PORT B
    Wire.beginTransmission(MCP23017_INPUT_ADDR);
    Wire.write(0x0A);  // GPINTENB register
    Wire.write(0xFF);  // Enable interrupts on all pins
    Wire.endTransmission();
    
    // Configure interrupt pins as inputs
    pinMode(PIN_MCP_INTB, INPUT_PULLUP);
}

void DigitalInputs::attachInterrupt(void (*callback)()) {
    // Attach interrupt to INTB pin (for PORTB)
    ::attachInterrupt(digitalPinToInterrupt(PIN_MCP_INTB), callback, FALLING);
}

bool DigitalInputs::inputChanged() {
    return interruptOccurred;
}

void DigitalInputs::clearInterrupt() {
    interruptOccurred = false;
    // Read port values to clear the interrupt condition
    mcp.readPort(MCP23017Port::B);
}