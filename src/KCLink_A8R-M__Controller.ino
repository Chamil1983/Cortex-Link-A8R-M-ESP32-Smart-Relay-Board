/**
 * Cortex Link A8R-M ESP32 IoT Smart Home Controller
 *
 * This application provides functionality for the Cortex Link A8R-M ESP32 Smart Relay Board.
 * Features include:
 * - Digital inputs reading (8 channels via MCP23017)
 * - Relay control (6 channels via MCP23017)
 * - Analog inputs (2x 0-5V and 2x 4-20mA)
 * - Digital to Analog outputs (via GP8413)
 * - Temperature/humidity sensing (2x DHT22 and DS18B20)
 * - RS485 Modbus communication
 * - RF433 communication
 * - Ethernet communication via W5500 module
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>  // Make sure this is included
#include "src/Config.h"
#include "src/Debug.h"
#include "src/DigitalInputs.h"
#include "src/RelayOutputs.h"
#include "src/AnalogInputs.h"
#include "src/DACControl.h"
#include "src/DHT_Sensors.h"
#include "src/ModbusComm.h"
#include "src/RF433Comm.h"
#include "src/EthernetControl.h"

 // Module instances
DigitalInputs digitalInputs;
RelayOutputs relayOutputs;
AnalogInputs analogInputs;
DACControl dacControl;
DHTSensors dhtSensors;
ModbusComm modbusComm;
RF433Comm rf433Comm;
EthernetControl ethernetControl;

// Ethernet MAC address (must be unique on your network)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Flags
volatile bool digitalInputInterrupt = false;

// Function prototypes
void setupWire();
void digitalInputInterruptHandler();
void processBuzzer(unsigned long currentMillis);
void handleDigitalInputs();
void setupModbusServer();

// Timing variables
unsigned long lastStatusPrint = 0;
const unsigned long STATUS_INTERVAL = 10000;  // Print status every 10 seconds

// Buzzer variables
unsigned long buzzerStartTime = 0;
bool buzzerActive = false;
const unsigned long BUZZER_DURATION = 100;  // 100ms beep

// Modbus server variables (reduced size)
uint16_t mbInputsRegs[NUM_DIGITAL_INPUTS];
uint16_t mbRelayRegs[NUM_RELAY_OUTPUTS];
uint16_t mbAnalogRegs[NUM_ANALOG_CHANNELS * 2];
uint16_t mbTempRegs[NUM_DHT_SENSORS];
uint16_t mbHumRegs[NUM_DHT_SENSORS];
uint16_t mbDS18B20Temps[MAX_DS18B20_SENSORS];
uint16_t mbDacRegs[4];

// Modbus callback function declarations
uint16_t cbDigitalInputs(TRegister* reg, uint16_t val);
uint16_t cbRelays(TRegister* reg, uint16_t val);
uint16_t cbAnalogValues(TRegister* reg, uint16_t val);
uint16_t cbDHTValues(TRegister* reg, uint16_t val);
uint16_t cbDS18B20Values(TRegister* reg, uint16_t val);
uint16_t cbDacValues(TRegister* reg, uint16_t val);

void setup() {
    // Initialize serial first
    Serial.begin(115200);
    delay(1000);  // Wait for serial to stabilize
    Serial.println("\nCortex Link A8R-M ESP32 IoT Controller");

    // Configure I2C with slower clock speed
    Wire.begin(I2C_SDA_PIN, I2C_SCK_PIN, 50000);

    // Initialize buzzer early
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    // =============================================
    // Initialize modules in a safe order
    // =============================================

    // Analog inputs (doesn't need I2C)
    Serial.print("Analog: ");
    analogInputs.begin();
    Serial.println("OK");

    // Wait between initializations to avoid I2C bus contention
    delay(200);

    // Digital inputs (MCP23017)
    Serial.print("Digital: ");
    if (digitalInputs.begin()) {
        digitalInputs.attachInterrupt(digitalInputInterruptHandler);
        Serial.println("OK");
    }
    else {
        Serial.println("FAILED");
    }

    delay(200);

    // Relay outputs (MCP23017)
    Serial.print("Relays: ");
    if (relayOutputs.begin()) {
        Serial.println("OK");
    }
    else {
        Serial.println("FAILED");
    }

    delay(200);

    // Ethernet initialization (uses MCP23017 for reset control)
    // We need to initialize the MCP23017 for the Ethernet reset first
    Serial.print("Ethernet: ");
    if (ethernetControl.initMCP(digitalInputs.getMCP())) {
        // Now initialize the Ethernet module with DHCP
        if (ethernetControl.begin(mac)) {
            Serial.println("OK");
        }
        else {
            Serial.println("Failed to connect");
        }
    }
    else {
        Serial.println("Reset init failed");
    }

    delay(200);

    // DAC Control
    Serial.print("DAC: ");
    if (dacControl.begin()) {
        Serial.println("OK");
    }
    else {
        Serial.println("FAILED");
    }

    delay(200);

    // DHT & DS18B20 sensors (OneWire)
    Serial.print("Sensors: ");
    bool sensorsOk = dhtSensors.begin();
    Serial.println(sensorsOk ? "OK" : "FAILED");

    delay(200);

    // RF433 Communication (doesn't need I2C)
    Serial.print("RF433: ");
    bool rf433Ok = rf433Comm.begin();
    Serial.println(rf433Ok ? "OK" : "FAILED");

    delay(200);

    // Modbus Communication (last)
    Serial.print("Modbus: ");
    if (modbusComm.begin(9600)) {
        setupModbusServer();
        Serial.println("OK");
    }
    else {
        Serial.println("FAILED");
    }

    Serial.println("Init complete");

    // Short beep to indicate startup completed
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_BUZZER, LOW);
}

void loop() {
    unsigned long currentMillis = millis();

    // DHT and DS18B20 sensor readings - update less frequently
    static uint8_t sensorUpdateCounter = 0;
    if (++sensorUpdateCounter >= 10) {  // Update every ~100ms
        dhtSensors.update();
        sensorUpdateCounter = 0;
    }

    // Process Modbus communications
    modbusComm.task();

    // Process Ethernet tasks
    ethernetControl.task();

    // Check for digital input changes
    if (digitalInputInterrupt) {
        handleDigitalInputs();
        digitalInputInterrupt = false;
    }

    // Check for RF433 received data
    if (rf433Comm.available()) {
        unsigned long rfCode = rf433Comm.getReceivedValue();
        Serial.print("RF: ");
        Serial.println(rfCode);
        rf433Comm.resetReceiver();
    }

    // Process buzzer state
    processBuzzer(currentMillis);

    // Print status every 10 seconds
    if (currentMillis - lastStatusPrint >= STATUS_INTERVAL) {
        LOG_MEMORY();

        // Print Ethernet status
        if (ethernetControl.isConnected()) {
            IPAddress ip = ethernetControl.getIP();
            Serial.print("ETH IP: ");
            Serial.print(ip[0]);
            Serial.print(".");
            Serial.print(ip[1]);
            Serial.print(".");
            Serial.print(ip[2]);
            Serial.print(".");
            Serial.println(ip[3]);
        }
        else {
            Serial.println("ETH: Not connected");
        }

        lastStatusPrint = currentMillis;
    }

    // Allow for system tasks to run
    delay(10);
}

void digitalInputInterruptHandler() {
    // Keep this function minimal - just set a flag
    digitalInputInterrupt = true;
}

void setupModbusServer() {
    // Only register essential Modbus handlers
    modbusComm.addInputRegisterHandler(MB_REG_INPUTS_START, NUM_DIGITAL_INPUTS, cbDigitalInputs);
    modbusComm.addHoldingRegisterHandler(MB_REG_RELAYS_START, NUM_RELAY_OUTPUTS, cbRelays);
    modbusComm.addInputRegisterHandler(MB_REG_ANALOG_START, NUM_ANALOG_CHANNELS * 2, cbAnalogValues);

    // Only add DHT handlers if needed
    modbusComm.addInputRegisterHandler(MB_REG_TEMP_START, NUM_DHT_SENSORS, cbDHTValues);
    modbusComm.addInputRegisterHandler(MB_REG_HUM_START, NUM_DHT_SENSORS, cbDHTValues);

    // Only add DS18B20 handlers if there are sensors
    uint8_t ds18b20Count = dhtSensors.getDS18B20Count();
    if (ds18b20Count > 0) {
        modbusComm.addInputRegisterHandler(MB_REG_DS18B20_START, ds18b20Count, cbDS18B20Values);
    }

    modbusComm.addHoldingRegisterHandler(MB_REG_DAC_START, 4, cbDacValues);
}

void processBuzzer(unsigned long currentMillis) {
    if (buzzerActive && (currentMillis - buzzerStartTime >= BUZZER_DURATION)) {
        digitalWrite(PIN_BUZZER, LOW);
        buzzerActive = false;
    }
}

void handleDigitalInputs() {
    uint8_t newInputs = digitalInputs.readAllInputs();
    Serial.print("DI: 0x");
    Serial.println(newInputs, HEX);

    // Clear the interrupt
    digitalInputs.clearInterrupt();

    // Beep when inputs change
    digitalWrite(PIN_BUZZER, HIGH);
    buzzerStartTime = millis();
    buzzerActive = true;
}

// Modbus callback functions - simplified implementations
uint16_t cbDigitalInputs(TRegister* reg, uint16_t val) {
    for (uint8_t i = 0; i < NUM_DIGITAL_INPUTS; i++) {
        mbInputsRegs[i] = digitalInputs.readInput(i) ? 1 : 0;
    }

    uint8_t index = reg->address.address - MB_REG_INPUTS_START;
    return (index < NUM_DIGITAL_INPUTS) ? mbInputsRegs[index] : 0;
}

uint16_t cbRelays(TRegister* reg, uint16_t val) {
    uint8_t relayNum = reg->address.address - MB_REG_RELAYS_START;

    if (val != reg->value && relayNum < NUM_RELAY_OUTPUTS) {
        relayOutputs.setRelay(relayNum, val > 0);
        mbRelayRegs[relayNum] = val > 0 ? 1 : 0;
        return val;
    }

    for (uint8_t i = 0; i < NUM_RELAY_OUTPUTS; i++) {
        mbRelayRegs[i] = relayOutputs.getRelayState(i) ? 1 : 0;
    }

    return (relayNum < NUM_RELAY_OUTPUTS) ? mbRelayRegs[relayNum] : 0;
}

uint16_t cbAnalogValues(TRegister* reg, uint16_t val) {
    for (uint8_t i = 0; i < NUM_ANALOG_CHANNELS; i++) {
        mbAnalogRegs[i] = (uint16_t)(analogInputs.readVoltage(i) * 1000);
        mbAnalogRegs[i + NUM_ANALOG_CHANNELS] = (uint16_t)(analogInputs.readCurrent(i) * 1000);
    }

    uint8_t index = reg->address.address - MB_REG_ANALOG_START;
    return (index < NUM_ANALOG_CHANNELS * 2) ? mbAnalogRegs[index] : 0;
}

uint16_t cbDHTValues(TRegister* reg, uint16_t val) {
    uint16_t regAddr = reg->address.address;

    for (uint8_t i = 0; i < NUM_DHT_SENSORS; i++) {
        if (dhtSensors.isSensorConnected(i)) {
            mbTempRegs[i] = (uint16_t)(dhtSensors.getTemperature(i) * 10);
            mbHumRegs[i] = (uint16_t)(dhtSensors.getHumidity(i) * 10);
        }
        else {
            mbTempRegs[i] = 0xFFFF;
            mbHumRegs[i] = 0xFFFF;
        }
    }

    if (regAddr >= MB_REG_TEMP_START && regAddr < MB_REG_TEMP_START + NUM_DHT_SENSORS) {
        return mbTempRegs[regAddr - MB_REG_TEMP_START];
    }
    else if (regAddr >= MB_REG_HUM_START && regAddr < MB_REG_HUM_START + NUM_DHT_SENSORS) {
        return mbHumRegs[regAddr - MB_REG_HUM_START];
    }

    return 0;
}

uint16_t cbDS18B20Values(TRegister* reg, uint16_t val) {
    uint8_t ds18b20Count = dhtSensors.getDS18B20Count();

    for (uint8_t i = 0; i < ds18b20Count && i < MAX_DS18B20_SENSORS; i++) {
        mbDS18B20Temps[i] = (uint16_t)(dhtSensors.getDS18B20Temperature(i) * 10);
    }

    uint8_t index = reg->address.address - MB_REG_DS18B20_START;
    return (index < ds18b20Count) ? mbDS18B20Temps[index] : 0;
}

uint16_t cbDacValues(TRegister* reg, uint16_t val) {
    uint8_t regOffset = reg->address.address - MB_REG_DAC_START;
    uint8_t channel = regOffset / 2;
    bool isCurrent = (regOffset % 2) == 1;

    if (channel < 2) {
        if (val != reg->value) {
            if (isCurrent) {
                float current = val / 1000.0;
                dacControl.setCurrent(channel, current);
                mbDacRegs[channel * 2 + 1] = val;
            }
            else {
                float voltage = val / 1000.0;
                dacControl.setVoltage(channel, voltage);
                mbDacRegs[channel * 2] = val;
            }
            return val;
        }

        mbDacRegs[channel * 2] = (uint16_t)(dacControl.getVoltage(channel) * 1000);
        mbDacRegs[channel * 2 + 1] = (uint16_t)(dacControl.getCurrent(channel) * 1000);
        return mbDacRegs[regOffset];
    }

    return 0;
}
