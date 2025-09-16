/**
 * EthernetControl.cpp - Implementation of Ethernet W5500 control
 */

#include "EthernetControl.h"

EthernetControl::EthernetControl() :
    state(NETWORK_DISCONNECTED),
    dhcpMode(true),
    lastConnectionAttempt(0),
    lastCheckTime(0),
    mcpDevice(nullptr),
    mcpInitialized(false)
{
    // Initialize MAC address to all zeros
    memset(macAddress, 0, sizeof(macAddress));
}

EthernetControl::~EthernetControl() {
    // No dynamic memory to free
}

bool EthernetControl::initMCP(MCP23017& mcp) {
    // Store reference to MCP23017 instance
    mcpDevice = &mcp;

    // Configure GPA5 as output for W5500 reset
    mcpDevice->pinMode(MCP_ETH_RESET_PIN, OUTPUT);

    // Set reset pin high initially (not in reset state)
    mcpDevice->digitalWrite(MCP_ETH_RESET_PIN, HIGH);
    mcpInitialized = true;
    ERROR_LOG("ETH reset pin ready");

    return mcpInitialized;
}

bool EthernetControl::begin(uint8_t mac[6], bool useStaticIP,
    const IPAddress& ip, const IPAddress& gateway,
    const IPAddress& subnet, const IPAddress& dns) {

    // Store MAC address
    memcpy(macAddress, mac, 6);
    dhcpMode = !useStaticIP;

    // Initialize SPI interface for W5500
    SPI.begin(PIN_ETH_SCLK, PIN_ETH_MISO, PIN_ETH_MOSI);

    // Reset Ethernet module
    if (!reset()) {
        ERROR_LOG("Ethernet reset failed");
        state = NETWORK_ERROR;
        return false;
    }

    // Initialize Ethernet with proper CS pin
    Ethernet.init(PIN_ETH_CS);

    // Start the Ethernet connection
    state = NETWORK_CONNECTING;
    int result;

    if (useStaticIP) {
        // Static IP configuration
        ERROR_LOG("ETH static IP");
        Ethernet.begin(mac, ip, dns, gateway, subnet);
        result = 1; // Assume success for static IP
    }
    else {
        // DHCP configuration
        ERROR_LOG("ETH DHCP");
        result = Ethernet.begin(mac);
    }

    if (result == 0) {
        ERROR_LOG("ETH DHCP failed");
        state = NETWORK_ERROR;
        return false;
    }

    // Check if we got a valid IP address
    if (Ethernet.localIP() == IPAddress(0, 0, 0, 0)) {
        ERROR_LOG("ETH no IP");
        state = NETWORK_ERROR;
        return false;
    }

    // Success!
    state = NETWORK_CONNECTED;

    // Log IP information
    char ipBuffer[16];
    sprintf(ipBuffer, "%d.%d.%d.%d",
        Ethernet.localIP()[0], Ethernet.localIP()[1],
        Ethernet.localIP()[2], Ethernet.localIP()[3]);
    ERROR_LOG("ETH IP: %s", ipBuffer);

    return true;
}

bool EthernetControl::reset() {
    // Check if MCP23017 is initialized
    if (!mcpInitialized || mcpDevice == nullptr) {
        ERROR_LOG("ETH reset: No MCP");
        return false;
    }

    // Reset sequence for W5500:
    // 1. Pull reset pin LOW
    mcpDevice->digitalWrite(MCP_ETH_RESET_PIN, LOW);
    delay(ETH_RESET_DURATION);  // Keep reset active

    // 2. Pull reset pin HIGH
    mcpDevice->digitalWrite(MCP_ETH_RESET_PIN, HIGH);
    delay(ETH_RESET_DURATION);  // Wait for stabilization

    ERROR_LOG("ETH reset done");
    return true;
}

bool EthernetControl::isConnected() {
    return (state == NETWORK_CONNECTED);
}

NetworkState EthernetControl::getState() {
    return state;
}

IPAddress EthernetControl::getIP() {
    return Ethernet.localIP();
}

IPAddress EthernetControl::getSubnetMask() {
    return Ethernet.subnetMask();
}

IPAddress EthernetControl::getGateway() {
    return Ethernet.gatewayIP();
}

IPAddress EthernetControl::getDNS() {
    return Ethernet.dnsServerIP();
}

void EthernetControl::getMACAddress(uint8_t mac[6]) {
    memcpy(mac, macAddress, 6);
}

void EthernetControl::task() {
    unsigned long currentMillis = millis();

    // Periodic connection check
    if (currentMillis - lastCheckTime >= CHECK_INTERVAL) {
        lastCheckTime = currentMillis;

        // If we're using DHCP, maintain the DHCP lease
        if (dhcpMode) {
            Ethernet.maintain();
        }

        // Check link status
        EthernetLinkStatus linkStatus = Ethernet.linkStatus();

        if (linkStatus == LinkOFF) {
            if (state != NETWORK_DISCONNECTED) {
                state = NETWORK_DISCONNECTED;
                ERROR_LOG("ETH link down");
            }
        }
        else if (linkStatus == LinkON && state != NETWORK_CONNECTED) {
            state = NETWORK_CONNECTED;
            ERROR_LOG("ETH link up");
        }
    }
}