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
 * - RS485 Modbus communication using modbus-esp8266 library
 * - RF433 communication
 * 
 * The application is structured in a modular way with separate classes for each functionality.
 */

#include <Arduino.h>
#include <Wire.h>
#include "Config.h"
#include "Debug.h"
#include "DigitalInputs.h"
#include "RelayOutputs.h"
#include "AnalogInputs.h"
#include "DACControl.h"
#include "DHT_Sensors.h"
#include "ModbusComm.h"
#include "RF433Comm.h"

// Module instances
DigitalInputs digitalInputs;
RelayOutputs relayOutputs;
AnalogInputs analogInputs;
DACControl dacControl;
DHTSensors dhtSensors;
ModbusComm modbusComm;
RF433Comm rf433Comm;

// Flags for interrupt handling
volatile bool digitalInputInterrupt = false;
volatile bool rf433Received = false;

// Function prototypes
void setupWire();
void digitalInputInterruptHandler();
void printAllSensorValues();
void processBuzzer(unsigned long currentMillis);
void handleDigitalInputs();
void setupModbusServer();

// Timing variables
unsigned long lastStatusPrint = 0;
const unsigned long STATUS_INTERVAL = 5000;  // Print status every 5 seconds

// Buzzer variables
unsigned long buzzerStartTime = 0;
bool buzzerActive = false;
const unsigned long BUZZER_DURATION = 100;  // 100ms beep

// Modbus server variables
uint16_t mbInputsRegs[NUM_DIGITAL_INPUTS];
uint16_t mbRelayRegs[NUM_RELAY_OUTPUTS];
uint16_t mbAnalogRegs[NUM_ANALOG_CHANNELS * 2]; // Include both voltage and current channels
uint16_t mbTempRegs[NUM_DHT_SENSORS];
uint16_t mbHumRegs[NUM_DHT_SENSORS];
uint16_t mbDS18B20Temps[MAX_DS18B20_SENSORS];
uint16_t mbDacRegs[4]; // 2 DAC channels, each with voltage and current

// Modbus callback functions
uint16_t cbDigitalInputs(TRegister* reg, uint16_t val) {
  // Update digital inputs in Modbus registers - this is read-only
  for (uint8_t i = 0; i < NUM_DIGITAL_INPUTS; i++) {
    mbInputsRegs[i] = digitalInputs.readInput(i) ? 1 : 0;
  }
  
  uint8_t index = reg->address.address - MB_REG_INPUTS_START;
  if (index < NUM_DIGITAL_INPUTS) {
    return mbInputsRegs[index];
  }
  return 0;
}

uint16_t cbRelays(TRegister* reg, uint16_t val) {
  // Handle write to relay registers
  uint8_t relayNum = reg->address.address - MB_REG_RELAYS_START;
  
  // Check if it's a write operation
  if (val != reg->value && relayNum < NUM_RELAY_OUTPUTS) {
    DEBUG_LOG_MSG("Relay %d state change: %d -> %d", relayNum, reg->value, val);
    relayOutputs.setRelay(relayNum, val > 0);
    mbRelayRegs[relayNum] = val > 0 ? 1 : 0;
    return val;
  }

  // Read operation - update from actual relay state
  for (uint8_t i = 0; i < NUM_RELAY_OUTPUTS; i++) {
    mbRelayRegs[i] = relayOutputs.getRelayState(i) ? 1 : 0;
  }
  
  if (relayNum < NUM_RELAY_OUTPUTS) {
    return mbRelayRegs[relayNum];
  }
  return 0;
}

uint16_t cbAnalogValues(TRegister* reg, uint16_t val) {
  // Update analog values in Modbus registers - this is read-only
  for (uint8_t i = 0; i < NUM_ANALOG_CHANNELS; i++) {
    // Scale voltage (0-5V) to 0-5000 for integer representation
    mbAnalogRegs[i] = (uint16_t)(analogInputs.readVoltage(i) * 1000);
    
    // Scale current (4-20mA) to 4000-20000 for integer representation
    mbAnalogRegs[i + NUM_ANALOG_CHANNELS] = (uint16_t)(analogInputs.readCurrent(i) * 1000);
  }
  
  uint8_t index = reg->address.address - MB_REG_ANALOG_START;
  if (index < NUM_ANALOG_CHANNELS * 2) {
    return mbAnalogRegs[index];
  }
  return 0;
}

uint16_t cbDHTValues(TRegister* reg, uint16_t val) {
  uint16_t regAddr = reg->address.address;
  
  // Update temperature and humidity values in Modbus registers - this is read-only
  for (uint8_t i = 0; i < NUM_DHT_SENSORS; i++) {
    if (dhtSensors.isSensorConnected(i)) {
      // Scale temperature to integer value (x10 for one decimal precision)
      mbTempRegs[i] = (uint16_t)(dhtSensors.getTemperature(i) * 10);
      
      // Scale humidity to integer value (x10 for one decimal precision)
      mbHumRegs[i] = (uint16_t)(dhtSensors.getHumidity(i) * 10);
    } else {
      mbTempRegs[i] = 0xFFFF; // Indicate sensor not connected
      mbHumRegs[i] = 0xFFFF;  // Indicate sensor not connected
    }
  }
  
  // Determine if we're reading temperature or humidity register
  if (regAddr >= MB_REG_TEMP_START && regAddr < MB_REG_TEMP_START + NUM_DHT_SENSORS) {
    uint8_t index = regAddr - MB_REG_TEMP_START;
    return mbTempRegs[index];
  } else if (regAddr >= MB_REG_HUM_START && regAddr < MB_REG_HUM_START + NUM_DHT_SENSORS) {
    uint8_t index = regAddr - MB_REG_HUM_START;
    return mbHumRegs[index];
  }
  
  return 0;
}

uint16_t cbDS18B20Values(TRegister* reg, uint16_t val) {
  uint16_t regAddr = reg->address.address;
  uint8_t ds18b20Count = dhtSensors.getDS18B20Count();
  
  // Update DS18B20 temperature values in Modbus registers
  for (uint8_t i = 0; i < ds18b20Count && i < MAX_DS18B20_SENSORS; i++) {
    if (dhtSensors.isDS18B20Connected(i)) {
      // Scale temperature to integer value (x10 for one decimal precision)
      mbDS18B20Temps[i] = (uint16_t)(dhtSensors.getDS18B20Temperature(i) * 10);
    } else {
      mbDS18B20Temps[i] = 0xFFFF;  // Indicate sensor not connected
    }
  }
  
  uint8_t index = regAddr - MB_REG_DS18B20_START;
  if (index < ds18b20Count) {
    return mbDS18B20Temps[index];
  }
  
  return 0;
}

uint16_t cbDacValues(TRegister* reg, uint16_t val) {
  uint8_t regOffset = reg->address.address - MB_REG_DAC_START;
  uint8_t channel = regOffset / 2;
  bool isCurrent = (regOffset % 2) == 1;
  
  if (channel < 2) { // We have 2 DAC channels
    // Check if it's a write operation
    if (val != reg->value) {
      // Handle write operation
      if (isCurrent) {
        // Convert from integer (4000-20000) to float (4.0-20.0)
        float current = val / 1000.0;
        DEBUG_LOG_MSG("DAC channel %d current set: %.1fmA", channel, current);
        dacControl.setCurrent(channel, current);
        mbDacRegs[channel*2 + 1] = val;
      } else {
        // Convert from integer (0-5000) to float (0.0-5.0)
        float voltage = val / 1000.0;
        DEBUG_LOG_MSG("DAC channel %d voltage set: %.2fV", channel, voltage);
        dacControl.setVoltage(channel, voltage);
        mbDacRegs[channel*2] = val;
      }
      return val;
    }
    
    // Update DAC values for read operations
    for (uint8_t i = 0; i < 2; i++) {
      mbDacRegs[i*2] = (uint16_t)(dacControl.getVoltage(i) * 1000);
      mbDacRegs[i*2 + 1] = (uint16_t)(dacControl.getCurrent(i) * 1000);
    }
    return mbDacRegs[regOffset];
  }
  return 0;
}

void setup() {
  // Initialize debugging
  Debug::begin(115200);
  
  INFO_LOG("Cortex Link A8R-M ESP32 IoT Smart Home Controller");
  INFO_LOG("Firmware Version: 1.0.0");
  INFO_LOG("Build Date: " __DATE__ " " __TIME__);
  INFO_LOG("Initializing...");
  
  // Log ESP32 info
  INFO_LOG("ESP32 Chip Model: %s", ESP.getChipModel());
  INFO_LOG("ESP32 Chip Revision: %d", ESP.getChipRevision());
  INFO_LOG("ESP32 SDK Version: %s", ESP.getSdkVersion());
  LOG_MEMORY("Startup");
  
  // Setup I2C
  setupWire();
  
  // Scan I2C devices
  Debug::scanI2CDevices();
  
  // Initialize buzzer
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  
  // Initialize all modules with timing measurements
  START_TIMER(0);
  INFO_LOG("Initializing Digital Inputs...");
  if (digitalInputs.begin()) {
    INFO_LOG("Digital Inputs initialization OK");
    digitalInputs.attachInterrupt(digitalInputInterruptHandler);
  } else {
    ERROR_LOG("Digital Inputs initialization FAILED");
  }
  STOP_TIMER(0, "Digital Inputs init");
  
  START_TIMER(1);
  INFO_LOG("Initializing Relay Outputs...");
  if (relayOutputs.begin()) {
    INFO_LOG("Relay Outputs initialization OK");
  } else {
    ERROR_LOG("Relay Outputs initialization FAILED");
  }
  STOP_TIMER(1, "Relay Outputs init");
  
  START_TIMER(2);
  INFO_LOG("Initializing Analog Inputs...");
  analogInputs.begin();
  INFO_LOG("Analog Inputs initialization OK");
  STOP_TIMER(2, "Analog Inputs init");
  
  START_TIMER(3);
  INFO_LOG("Initializing DAC Control...");
  if (dacControl.begin()) {
    INFO_LOG("DAC Control initialization OK");
  } else {
    ERROR_LOG("DAC Control initialization FAILED");
  }
  STOP_TIMER(3, "DAC Control init");
  
  START_TIMER(4);
  INFO_LOG("Initializing Temperature/Humidity Sensors...");
  if (dhtSensors.begin()) {
    INFO_LOG("Temperature/Humidity Sensors initialization OK");
    INFO_LOG("Found %d DS18B20 temperature sensors", dhtSensors.getDS18B20Count());
  } else {
    ERROR_LOG("Temperature/Humidity Sensors initialization FAILED");
  }
  STOP_TIMER(4, "Temp/Humidity Sensors init");
  
  START_TIMER(5);
  INFO_LOG("Initializing Modbus...");
  if (modbusComm.begin(9600)) {
    INFO_LOG("Modbus initialization OK");
    setupModbusServer(); // Configure Modbus server if needed
  } else {
    ERROR_LOG("Modbus initialization FAILED");
  }
  STOP_TIMER(5, "Modbus init");
  
  START_TIMER(6);
  INFO_LOG("Initializing RF433...");
  if (rf433Comm.begin()) {
    INFO_LOG("RF433 initialization OK");
  } else {
    ERROR_LOG("RF433 initialization FAILED");
  }
  STOP_TIMER(6, "RF433 init");
  
  INFO_LOG("Initialization complete!");
  LOG_MEMORY("After initialization");
  
  // Short beep to indicate startup completed
  digitalWrite(PIN_BUZZER, HIGH);
  delay(200);
  digitalWrite(PIN_BUZZER, LOW);
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Update DHT and DS18B20 sensor readings
  dhtSensors.update();
  
  // Process Modbus communications
  modbusComm.task();
  
  // Check for digital input changes
  if (digitalInputInterrupt) {
    handleDigitalInputs();
    digitalInputInterrupt = false;
  }
  
  // Check for RF433 received data
  if (rf433Comm.available()) {
    unsigned long rfCode = rf433Comm.getReceivedValue();
    unsigned int bitLength = rf433Comm.getReceivedBitlength();
    unsigned int protocol = rf433Comm.getReceivedProtocol();
    
    INFO_LOG("Received RF code: %lu, Bitlength: %u, Protocol: %u", 
             rfCode, bitLength, protocol);
             
    rf433Comm.resetReceiver();
  }
  
  // Process buzzer state
  processBuzzer(currentMillis);
  
  // Print status periodically
  if (currentMillis - lastStatusPrint >= STATUS_INTERVAL) {
    printAllSensorValues();
    LOG_MEMORY("Periodic");
    lastStatusPrint = currentMillis;
  }
  
  // Small delay to prevent CPU hogging
  delay(10);
}

void setupWire() {
  INFO_LOG("Initializing I2C on pins SDA=%d, SCL=%d", I2C_SDA_PIN, I2C_SCK_PIN);
  Wire.begin(I2C_SDA_PIN, I2C_SCK_PIN);
  Wire.setClock(100000);  // 100kHz I2C clock speed
}

void digitalInputInterruptHandler() {
  digitalInputInterrupt = true;
}

void setupModbusServer() {
  INFO_LOG("Setting up Modbus server registers");
  
  // Register handlers for different register types
  modbusComm.addInputRegisterHandler(MB_REG_INPUTS_START, NUM_DIGITAL_INPUTS, cbDigitalInputs);
  modbusComm.addHoldingRegisterHandler(MB_REG_RELAYS_START, NUM_RELAY_OUTPUTS, cbRelays);
  modbusComm.addInputRegisterHandler(MB_REG_ANALOG_START, NUM_ANALOG_CHANNELS * 2, cbAnalogValues);
  modbusComm.addInputRegisterHandler(MB_REG_TEMP_START, NUM_DHT_SENSORS, cbDHTValues);
  modbusComm.addInputRegisterHandler(MB_REG_HUM_START, NUM_DHT_SENSORS, cbDHTValues);
  modbusComm.addInputRegisterHandler(MB_REG_DS18B20_START, dhtSensors.getDS18B20Count(), cbDS18B20Values);
  modbusComm.addHoldingRegisterHandler(MB_REG_DAC_START, 4, cbDacValues);
  
  DEBUG_LOG_MSG("Modbus server setup complete with %d DS18B20 sensors", dhtSensors.getDS18B20Count());
}

void printAllSensorValues() {
  INFO_LOG("--- System Status ---");
  
  // Digital inputs
  uint8_t inputs = digitalInputs.readAllInputs();
  INFO_LOG("Digital Inputs: 0b%d%d%d%d%d%d%d%d",
           (inputs >> 7) & 0x01, (inputs >> 6) & 0x01,
           (inputs >> 5) & 0x01, (inputs >> 4) & 0x01,
           (inputs >> 3) & 0x01, (inputs >> 2) & 0x01,
           (inputs >> 1) & 0x01, inputs & 0x01);
  
  // Relay states
  uint8_t relays = relayOutputs.getAllRelayStates();
  INFO_LOG("Relay States:   0b%d%d%d%d%d%d",
           (relays >> 5) & 0x01, (relays >> 4) & 0x01,
           (relays >> 3) & 0x01, (relays >> 2) & 0x01,
           (relays >> 1) & 0x01, relays & 0x01);
  
  // Analog inputs
  INFO_LOG("Analog Inputs:");
  for (int i = 0; i < NUM_ANALOG_CHANNELS; i++) {
    float voltage = analogInputs.readVoltage(i);
    INFO_LOG("  Channel %d (0-5V):    %.2fV", i+1, voltage);
  }
  
  for (int i = 0; i < NUM_CURRENT_CHANNELS; i++) {
    float current = analogInputs.readCurrent(i);
    INFO_LOG("  Channel %d (4-20mA):  %.2fmA", i+1, current);
  }
  
  // DAC outputs
  INFO_LOG("DAC Outputs:");
  for (int i = 0; i < 2; i++) {
    INFO_LOG("  Channel %d: %.2fV / %.2fmA", 
             i+1, dacControl.getVoltage(i), dacControl.getCurrent(i));
  }
  
  // DHT sensors
  INFO_LOG("DHT Temperature/Humidity Sensors:");
  for (int i = 0; i < NUM_DHT_SENSORS; i++) {
    if (dhtSensors.isSensorConnected(i)) {
      INFO_LOG("  DHT %d: %.1f°C / %.1f%%", 
               i+1, dhtSensors.getTemperature(i), dhtSensors.getHumidity(i));
    } else {
      INFO_LOG("  DHT %d: Not connected", i+1);
    }
  }
  
  // DS18B20 sensors
  uint8_t ds18b20Count = dhtSensors.getDS18B20Count();
  if (ds18b20Count > 0) {
    INFO_LOG("DS18B20 Temperature Sensors:");
    for (int i = 0; i < ds18b20Count; i++) {
      if (dhtSensors.isDS18B20Connected(i)) {
        INFO_LOG("  DS18B20 %d: %.1f°C", i+1, dhtSensors.getDS18B20Temperature(i));
      } else {
        INFO_LOG("  DS18B20 %d: Not connected", i+1);
      }
    }
  }
  
  // Print Modbus server information
  INFO_LOG("Modbus Server (Slave ID: 1):");
  INFO_LOG("  Registers (Read-only):");
  INFO_LOG("    Input Registers (%d-%d): Digital Inputs",
           MB_REG_INPUTS_START, MB_REG_INPUTS_START + NUM_DIGITAL_INPUTS - 1);
  
  INFO_LOG("    Input Registers (%d-%d): Analog Values",
           MB_REG_ANALOG_START, MB_REG_ANALOG_START + NUM_ANALOG_CHANNELS * 2 - 1);
  
  INFO_LOG("    Input Registers (%d-%d): Temperature",
           MB_REG_TEMP_START, MB_REG_TEMP_START + NUM_DHT_SENSORS - 1);
  
  INFO_LOG("    Input Registers (%d-%d): Humidity",
           MB_REG_HUM_START, MB_REG_HUM_START + NUM_DHT_SENSORS - 1);
  
  INFO_LOG("    Input Registers (%d-%d): DS18B20 Temperature",
           MB_REG_DS18B20_START, MB_REG_DS18B20_START + ds18b20Count - 1);
  
  INFO_LOG("  Registers (Read-Write):");
  INFO_LOG("    Holding Registers (%d-%d): Relay Control",
           MB_REG_RELAYS_START, MB_REG_RELAYS_START + NUM_RELAY_OUTPUTS - 1);
  
  INFO_LOG("    Holding Registers (%d-%d): DAC Control",
           MB_REG_DAC_START, MB_REG_DAC_START + 3);
}

void processBuzzer(unsigned long currentMillis) {
  // Turn off buzzer after timeout if active
  if (buzzerActive && (currentMillis - buzzerStartTime >= BUZZER_DURATION)) {
    digitalWrite(PIN_BUZZER, LOW);
    buzzerActive = false;
  }
}

void handleDigitalInputs() {
  // Read all digital inputs
  uint8_t newInputs = digitalInputs.readAllInputs();
  
  INFO_LOG("Digital inputs changed: 0b%d%d%d%d%d%d%d%d",
           (newInputs >> 7) & 0x01, (newInputs >> 6) & 0x01,
           (newInputs >> 5) & 0x01, (newInputs >> 4) & 0x01,
           (newInputs >> 3) & 0x01, (newInputs >> 2) & 0x01,
           (newInputs >> 1) & 0x01, newInputs & 0x01);
  
  // Clear the interrupt
  digitalInputs.clearInterrupt();
  
  // Optional: beep when inputs change
  digitalWrite(PIN_BUZZER, HIGH);
  buzzerStartTime = millis();
  buzzerActive = true;
}
