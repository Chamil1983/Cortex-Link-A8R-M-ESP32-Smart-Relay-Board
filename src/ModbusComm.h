#ifndef MODBUS_COMM_H
#define MODBUS_COMM_H

#include <Arduino.h>
#include <ModbusRTU.h>
#include "Config.h"

// Callback function type for Modbus register operations
typedef std::function<uint16_t(TRegister* reg, uint16_t val)> cbModbus;

class ModbusComm {
public:
    ModbusComm();
    bool begin(unsigned long baudRate = 9600);
    
    // Modbus functions
    bool readRegisters(uint8_t slaveAddr, uint16_t regAddr, uint16_t numRegs, uint16_t* data);
    bool writeRegister(uint8_t slaveAddr, uint16_t regAddr, uint16_t value);
    bool writeMultipleRegisters(uint8_t slaveAddr, uint16_t regAddr, uint16_t* data, uint16_t numRegs);
    
    // Process Modbus messages
    void task();
    
    // Serial port access for direct communication
    HardwareSerial* getSerial() { return serialPort; }
    
    // Register callback handlers for Modbus server functionality
    bool addHoldingRegisterHandler(uint16_t regAddr, uint16_t numRegs, cbModbus cb);
    bool addInputRegisterHandler(uint16_t regAddr, uint16_t numRegs, cbModbus cb);
    bool addCoilHandler(uint16_t regAddr, uint16_t numCoils, cbModbus cb);
    bool addDiscreteInputHandler(uint16_t regAddr, uint16_t numInputs, cbModbus cb);

private:
    ModbusRTU mb;
    HardwareSerial* serialPort;
    unsigned long baudRate;
    bool mbServerEnabled;
};

#endif // MODBUS_COMM_H