#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <vector>
#include "Config.h"

#if OTA_SUPPORTED == 0
#include <ESP8266mDNS.h>
#endif

// --- System & Project Definitions ---
#define CONFIG_PORTAL_MAX_TIME_SECONDS 300

extern WiFiEventHandler disconnectHandler;
extern void onStationModeDisconnected(const WiFiEventStationModeDisconnected& event);

// --- Function Prototypes ---
void setupWifiHost();
void setupWifiManagerConfigMenu(WiFiManager& wm);
void setupMenu(WiFiManager& wm);
void initWifiManager(WiFiManager& wm);

#endif // WIFIMANAGER_H