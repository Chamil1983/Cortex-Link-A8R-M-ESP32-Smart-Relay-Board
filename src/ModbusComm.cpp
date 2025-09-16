#include "ModbusComm.h"
#include <Arduino.h>

ModbusComm::ModbusComm() : baudRate(9600), mbServerEnabled(false) {
    serialPort = &Serial2;
}

bool ModbusComm::begin(unsigned long baud) {
    baudRate = baud;
    
    // Configure MAX485 control pin
    pinMode(PIN_MAX485_TXRX, OUTPUT);
    digitalWrite(PIN_MAX485_TXRX, LOW);  // Set to receive mode by default
    
    // Start serial port for RS485
    serialPort->begin(baudRate, SERIAL_8N1, PIN_MAX485_RO, PIN_MAX485_DI);
    
    // Initialize Modbus RTU with Serial2
    mb.begin(serialPort, PIN_MAX485_TXRX);
    mb.master();  // Set as master by default
    
    return true;
}

bool ModbusComm::readRegisters(uint8_t slaveAddr, uint16_t regAddr, uint16_t numRegs, uint16_t* data) {
    // Use modbus-esp8266 to read holding registers
    mb.readHreg(slaveAddr, regAddr, data, numRegs);
    
    // Process any pending communications (timeout set to 1000ms)
    uint32_t startTime = millis();
    while (millis() - startTime < 1000) {
        mb.task();
        delay(1);  // Small delay to prevent CPU hogging
    }
    
    return true;
}

bool ModbusComm::writeRegister(uint8_t slaveAddr, uint16_t regAddr, uint16_t value) {
    // Write single register
    mb.writeHreg(slaveAddr, regAddr, value);
    
    // Process any pending communications (timeout set to 1000ms)
    uint32_t startTime = millis();
    while (millis() - startTime < 1000) {
        mb.task();
        delay(1);  // Small delay to prevent CPU hogging
    }
    
    return true;
}

bool ModbusComm::writeMultipleRegisters(uint8_t slaveAddr, uint16_t regAddr, uint16_t* data, uint16_t numRegs) {
    // Write multiple registers
    mb.writeHreg(slaveAddr, regAddr, data, numRegs);
    
    // Process any pending communications (timeout set to 1000ms)
    uint32_t startTime = millis();
    while (millis() - startTime < 1000) {
        mb.task();
        delay(1);  // Small delay to prevent CPU hogging
    }
    
    return true;
}

void ModbusComm::task() {
    // Process Modbus messages
    mb.task();
}

bool ModbusComm::addHoldingRegisterHandler(uint16_t regAddr, uint16_t numRegs, cbModbus cb) {
    // If not already in server mode, switch to server mode
    if (!mbServerEnabled) {
        mb.server(1); // Default slave ID = 1
        mbServerEnabled = true;
    }
    
    // Add holding register handler
    bool result = false;
    for (uint16_t i = 0; i < numRegs; i++) {
        result = mb.addHreg(regAddr + i, 0);
        if (!result) break;
    }
    
    if (result) {
        mb.onGetHreg(regAddr, cb, numRegs);
        mb.onSetHreg(regAddr, cb, numRegs);
    }
    
    return result;
}

bool ModbusComm::addInputRegisterHandler(uint16_t regAddr, uint16_t numRegs, cbModbus cb) {
    // If not already in server mode, switch to server mode
    if (!mbServerEnabled) {
        mb.server(1); // Default slave ID = 1
        mbServerEnabled = true;
    }
    
    // Add input register handler
    bool result = false;
    for (uint16_t i = 0; i < numRegs; i++) {
        result = mb.addIreg(regAddr + i, 0);
        if (!result) break;
    }
    
    if (result) {
        mb.onGetIreg(regAddr, cb, numRegs);
    }
    
    return result;
}

bool ModbusComm::addCoilHandler(uint16_t regAddr, uint16_t numCoils, cbModbus cb) {
    // If not already in server mode, switch to server mode
    if (!mbServerEnabled) {
        mb.server(1); // Default slave ID = 1
        mbServerEnabled = true;
    }
    
    // Add coil handler
    bool result = false;
    for (uint16_t i = 0; i < numCoils; i++) {
        result = mb.addCoil(regAddr + i, false);
        if (!result) break;
    }
    
    if (result) {
        mb.onGetCoil(regAddr, cb, numCoils);
        mb.onSetCoil(regAddr, cb, numCoils);
    }
    
    return result;
}

bool ModbusComm::addDiscreteInputHandler(uint16_t regAddr, uint16_t numInputs, cbModbus cb) {
    // If not already in server mode, switch to server mode
    if (!mbServerEnabled) {
        mb.server(1); // Default slave ID = 1
        mbServerEnabled = true;
    }
    
    // Add discrete input handler
    bool result = false;
    for (uint16_t i = 0; i < numInputs; i++) {
        result = mb.addIsts(regAddr + i, false);
        if (!result) break;
    }
    
    if (result) {
        mb.onGetIsts(regAddr, cb, numInputs);
    }
    
    return result;
}