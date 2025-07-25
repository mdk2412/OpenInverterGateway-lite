#pragma once
#define _SHINE_CONFIG_H_

// ---------------------------------------------------------------
// Build configuration area start
// ---------------------------------------------------------------

// Define used modbus version used by your inverter here. Currently
// only 120 (v1.20), 124 (v1.24) and 305 (v3.05) versions are supported.
// Growatt TL-X(H) inverters use v1.24, but use different registers (3000 and
// above). To enable TL-X(H) support set the modbus version to 3000. Also SPF
// (version 5000) has some preliminary support. New protocols can be easily
// defined by adding new Growatt<version>.cpp/h files and then specifying the
// protocol there. See existing procol files for reference.
#define GROWATT_MODBUS_VERSION 3000

// On some SPH inverters (Protocol 124) the battery temperature multiplier
// differs from the documented value (of 0.1). Set this to 1.0 on these
// inverters.
// #define TEMPERATURE_WORKAROUND_MULTIPLIER 1.0

// Setting this define to 0 will disable the MQTT functionality
#define MQTT_SUPPORTED 1
// Setting this define to 0 will only disable the commands via MQTT
#define MQTT_COMMANDS 1

// Enable OTA update via espota. This is mainly for development purposes and
// enables updates of the device without physical access. Use at our own risk!
#define OTA_SUPPORTED 0
#define OTA_PASSWORD "enter_ota_secret_here"

// Enable direct modbus read/write support via the WebGUI. Enabling this is a
// potential security issue.
#define ENABLE_MODBUS_COMMUNICATION 1

// Enable standby for battery to save power
// if SoC <= BDCDischargeStopSOC & (PVTotalPower & BDCDischargePower) < sleep battery threshold
// (configure in WiFiManager) battery is put to sleep if PTOGRID_TOTAL > wake
// battery threshold (configure in WiFiManager) battery is woken up
#define BATTERY_STANDBY 0
#define BATTERY_STANDBY_TIMER 60000  // 60s default

// AC charge control
// If there are other inverters in the system only load the surplus via AC
// charging
#define ACCHARGE_CONTROL 1
#define ACCHARGE_CONTROL_TIMER 1000     // 1s default
#define ACCHARGE_CONTROL_MAXPOWER 2500  // maximum output power of inverter in W
#define ACCHARGE_CONTROL_OFFSET \
  2  // offset in % to be subtracted from target power rate to ensure 0 power
     // draw from grid

// Define a NTP Server and TZ Info to automatically adjust the inverter
// date/time. TZ Info can be found at:
// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define DEFAULT_NTP_SERVER "de.pool.ntp.org"
#define DEFAULT_TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_TIMER 3600000  // 1 hour default

// Setting this define to 1 will ping the default gateway periodically
// if the ping is not successful, the wifi connection will be reestablished
#define PINGER_SUPPORTED 0

// Setting this define to 1 will enable the debug output via the serial port.
// The serial port is the same used for the communication to the inverter.
// Enabling this feature can cause problems with the inverter communication!
// For ShineWiFi-S everything seems to work perfect even though this flag is set
// #define ENABLE_SERIAL_DEBUG

// define this will enable a web page (<ip>/debug) where debug messages can be
// displayed
#define ENABLE_WEB_DEBUG
// define this will enable Remote Debug
// #define ENABLE_TELNET_DEBUG

#if defined(ENABLE_SERIAL_DEBUG) || defined(ENABLE_WEB_DEBUG) || \
    defined(ENABLE_TELNET_DEBUG)
// to debug Modbus output turn this on (this is very verbose)
// #define DEBUG_MODBUS_OUTPUT
#endif

// Enable TLS connection from stick to broker. Make sure to update port and
// hostname to match the cert of the mqtt broker as well. Change this CERT to
// your custom mqtt cert.
// #define MQTTS_BROKER_CA_CERT "-----BEGIN CERTIFICATE-----\n" \
//     "MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\n" \
//     "....\n" \
//     "Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\n" \
//     "-----END CERTIFICATE-----\n"

// Setting this flag to 1 will simulate the inverter
// This could be helpful if it is night and the inverter is not working or
// during development where the stick is not connected to the inverter
// if Simulating the Inverter we've to set a Device Type this could be
// ShineWiFi_X or ShineWiFi_S
#define SIMULATE_INVERTER 0
#define SIMULATE_DEVICE ShineWiFi_X

// Data of the Wifi access point
// Default IP 192.168.4.1
// Hold down the Button for a few seconds to enter Access Point mode
#define DEFAULT_HOSTNAME "Growatt"
#define APPassword "growsolar"

// Username and password for firmware update
#define UPDATE_USER "admin"
#define UPDATE_PASSWORD "admin"

// For boards without any button you can detect double resets with the Double
// reset detector by setting this to 1
#define ENABLE_DOUBLE_RESET 0

// Timer definitions:
//    REFRESH_TIMER: Modbus polling rate [ms]
//    RETRY_TIMER: Determines the time between reconnection [ms]
//    LED_TIMER: Led blinking rate [ms]
#define REFRESH_TIMER 1000       // 5s default
#define WIFI_RETRY_TIMER 120000  // 120s default
#define LED_TIMER 500            //  0.5s default
#define WDT_TIMEOUT 300          // 5 min default

#if PINGER_SUPPORTED == 1
#define GATEWAY_IP IPAddress(192, 168, 178, 1)
#endif

#define LED_GN 0   // GPIO0
#define LED_RT 2   // GPIO2
#define LED_BL 16  // GPIO16

// Add support for the AP button on the normal Shine Stick. You can
// redefine AP_BUTTON_PRESSED to whatever condition you like for your stick.
//    BUTTON_TIMER: enter config mode after 5*BUTTON_TIMER [ms]
#if ENABLE_AP_BUTTON == 1
#define AP_BUTTON_PRESSED analogRead(A0) < 50
#define BUTTON_TIMER 500  //  0.5s default
#endif

// Enabling this will make the wifiManager available on the previously
// configured wifi and ip. This makes it possible to reconfigure the stick
// without a direct WIFI connection. This is a security risk as anyone could now
// remotely update your firmware.
// #define KEEP_AP_CONFIG_CONNECTION 1
