#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <vector>
#include "Config.h"

// --- Conditionals / Forward Declarations ---
#if MQTT_SUPPORTED == 1
#include "ShineMqtt.h"
#endif

#if OTA_SUPPORTED == 0
#include <ESP8266mDNS.h>
#endif

// --- System & Project Definitions ---
#define CONFIG_PORTAL_MAX_TIME_SECONDS 300

extern WiFiEventHandler disconnectHandler;
extern void onStationModeDisconnected(const WiFiEventStationModeDisconnected& event);

// --- Config Structures ---
struct WifiConfig {
  String hostname;
  String static_ip;
  String static_netmask;
  String static_gateway;
  String static_dns;

#if MQTT_SUPPORTED == 1
  MqttConfig mqtt;
#endif

  String syslog_ip;
  bool force_ap;
};

// Extern Global Wifi Object
extern WifiConfig Wifi;

// --- Function Prototypes ---
void loadConfig();
void saveConfig();
void setupWifiHost();
void saveParamCallback();
void setupWifiManagerConfigMenu(WiFiManager& wm);
void setupMenu(WiFiManager& wm, bool enableCustomParams);
void initWifiManager(WiFiManager& wm);

#endif // WIFIMANAGER_H