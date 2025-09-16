#include "DigitalInputs.h"
#include "Debug.h"

DigitalInputs::DigitalInputs() : lastInputState(0), interruptOccurred(false) {
}

bool DigitalInputs::begin() {
    // Initialize MCP23017 for digital inputs with retry mechanism
    bool success = false;

    // Try multiple times in case of I2C errors
    for (int attempt = 0; attempt < 3; attempt++) {
        Wire.setClock(50000); // Slower speed for reliability
        mcp.begin(MCP23017_INPUT_ADDR); // Return value ignored - can't use as bool

        // Configure PORTB pins as inputs with pull-ups
        bool pinsConfigured = true;
        for (uint8_t pin = 0; pin < 8; pin++) {
            mcp.pinMode(pin + 8, INPUT_PULLUP); // Return value ignored - can't use as bool
            delay(5); // Small delay between operations
        }

        // Setup interrupt handling
        setupInterrupts(); // Return value ignored - must be void

        // Read initial state
        lastInputState = mcp.readPort(MCP23017Port::B);
        success = true;
        break;
    }

    // Return to normal speed if successful
    if (success) {
        Wire.setClock(100000);
    }

    return success;
}

bool DigitalInputs::readInput(uint8_t inputNum) {
    if (inputNum >= NUM_DIGITAL_INPUTS) {
        return false;
    }

    // Read without error checking - library doesn't have lastError()
    bool value = !mcp.digitalRead(inputNum + 8);
    return value;
}

uint8_t DigitalInputs::readAllInputs() {
    // Read without error checking - library doesn't have lastError()
    uint8_t portValue = mcp.readPort(MCP23017Port::B);
    lastInputState = portValue;
    return ~portValue & 0xFF; // Invert all bits and mask to 8 bits
}

void DigitalInputs::setupInterrupts() {
    // Configure MCP23017 interrupts without error checking
    mcp.interruptMode(MCP23017InterruptMode::Separated);

    // Set INTCON register (compare against previous value)
    Wire.beginTransmission(MCP23017_INPUT_ADDR);
    Wire.write(0x08);  // INTCONB register
    Wire.write(0x00);  // 0 = compare against previous value
    Wire.endTransmission();

    delay(5);

    // Enable interrupts on all pins
    Wire.beginTransmission(MCP23017_INPUT_ADDR);
    Wire.write(0x0A);  // GPINTENB register
    Wire.write(0xFF);  // Enable interrupts on all pins
    Wire.endTransmission();

    // Configure interrupt pin
    pinMode(PIN_MCP_INTB, INPUT_PULLUP);
}

void DigitalInputs::attachInterrupt(void (*callback)()) {
    ::attachInterrupt(digitalPinToInterrupt(PIN_MCP_INTB), callback, FALLING);
}

bool DigitalInputs::inputChanged() {
    return interruptOccurred;
}

void DigitalInputs::clearInterrupt() {
    interruptOccurred = false;
    mcp.readPort(MCP23017Port::B); // Clear the interrupt condition
}