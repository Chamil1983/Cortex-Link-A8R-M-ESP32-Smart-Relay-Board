/**
 * EthernetControl.h - Ethernet W5500 control for Cortex Link A8R-M ESP32 IoT Smart Home Controller
 *
 * This class provides Ethernet communication capabilities using the W5500 module
 * with reset control through the MCP23017 U8 GPA5 pin.
 */

#ifndef ETHERNET_CONTROL_H
#define ETHERNET_CONTROL_H

#include <Arduino.h>
#include <Ethernet.h>
#include <SPI.h>
#include <MCP23017.h>
#include "Config.h"
#include "Debug.h"

 // Network states
enum NetworkState {
    NETWORK_DISCONNECTED,
    NETWORK_CONNECTING,
    NETWORK_CONNECTED,
    NETWORK_ERROR
};

class EthernetControl {
public:
    EthernetControl();
    ~EthernetControl();

    /**
     * Initialize the MCP23017 for Ethernet reset control
     * @param mcp Reference to MCP23017 instance
     * @return true if initialization was successful
     */
    bool initMCP(MCP23017& mcp);

    /**
     * Initialize the Ethernet module
     * @param mac The MAC address to use
     * @param useStaticIP Set true to use static IP instead of DHCP
     * @param ip Static IP address (if useStaticIP is true)
     * @param gateway Gateway address (if useStaticIP is true)
     * @param subnet Subnet mask (if useStaticIP is true)
     * @param dns DNS server address (if useStaticIP is true)
     * @return true if initialization was successful
     */
    bool begin(uint8_t mac[6],
        bool useStaticIP = false,
        const IPAddress& ip = IPAddress(0, 0, 0, 0),
        const IPAddress& gateway = IPAddress(0, 0, 0, 0),
        const IPAddress& subnet = IPAddress(255, 255, 255, 0),
        const IPAddress& dns = IPAddress(0, 0, 0, 0));

    /**
     * Check if Ethernet connection is established
     * @return true if connected
     */
    bool isConnected();

    /**
     * Get current Ethernet connection state
     * @return Connection state enum value
     */
    NetworkState getState();

    /**
     * Get current IP address
     * @return IP address as IPAddress object
     */
    IPAddress getIP();

    /**
     * Get current subnet mask
     * @return Subnet mask as IPAddress object
     */
    IPAddress getSubnetMask();

    /**
     * Get current gateway address
     * @return Gateway address as IPAddress object
     */
    IPAddress getGateway();

    /**
     * Get current DNS server address
     * @return DNS server address as IPAddress object
     */
    IPAddress getDNS();

    /**
     * Get MAC address
     * @param mac Array to store MAC address (must be at least 6 bytes)
     */
    void getMACAddress(uint8_t mac[6]);

    /**
     * Perform a hard reset of the Ethernet module using the MCP23017 GPA5 pin
     * @return true if reset was successful
     */
    bool reset();

    /**
     * Process Ethernet tasks (call this in the loop)
     */
    void task();

private:
    // Ethernet state
    NetworkState state;
    bool dhcpMode;
    uint8_t macAddress[6];
    unsigned long lastConnectionAttempt;
    unsigned long lastCheckTime;
    const unsigned long CHECK_INTERVAL = 5000; // Check connection every 5 seconds

    // Reference to MCP23017 for reset control
    MCP23017* mcpDevice;
    bool mcpInitialized;
};

#endif // ETHERNET_CONTROL_H