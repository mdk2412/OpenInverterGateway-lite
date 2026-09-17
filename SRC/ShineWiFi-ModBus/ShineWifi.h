#pragma once

#include <Arduino.h>
#include <vector>
#include "Config.h"
#include "UserConfig.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#if OTA_SUPPORTED == 0
#include <ESP8266mDNS.h>
#endif
#elif defined(ESP32)
#include <WebServer.h>
#include <WiFi.h>
#ifdef MQTTS_BROKER_CA_CERT
#define MQTTS_ENABLED 1
#endif
#endif

#include <WiFiManager.h>

#ifdef MQTTS_ENABLED
#include <WiFiClientSecure.h>
extern WiFiClientSecure espClient;
#else
#include <WiFiClient.h>
extern WiFiClient espClient;
#endif

// Makro für das Portal-Timeout
#define CONFIG_PORTAL_MAX_TIME_SECONDS 300

// Event-Handler extern bereitstellen
extern WiFiEventHandler disconnectHandler;

/**
 * Zentrale Initialisierung für WLAN, Hostnamen, Menü und WiFiManager.
 */
void setupShineWifi(WiFiManager& wm);

/**
 * Überwacht die Verbindung in der Loop und führt ggf. einen Reconnect oder Neustart aus.
 */
void WiFi_Reconnect();