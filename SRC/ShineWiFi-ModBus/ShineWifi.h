#pragma once

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#elif defined(ESP32)
#include <WebServer.h>
#include <WiFi.h>
#ifdef MQTTS_BROKER_CA_CERT
#define MQTTS_ENABLED 1
#endif
#ifdef MQTTS_ENABLED
#include <WiFiClientSecure.h>
#else
#include <WiFiClient.h>
#endif
#endif

#ifdef MQTTS_ENABLED
extern WiFiClientSecure espClient;
#else
extern WiFiClient espClient;
#endif

/**
 * Trennt die WLAN-Verbindung sauber und schaltet das WLAN-Modul aus.
 * @return true, wenn die Trennung erfolgreich war.
 */
bool ShineWifiDisconnect();

/**
 * Event-Callback, der aufgerufen wird, wenn die Station-Verbindung abbricht.
 * @param event Event-Details zur Trennung
 */
void onStationModeDisconnected(const WiFiEventStationModeDisconnected& event);

/**
 * Überwacht anhaltende Disconnects im Loop und führt bei Überschreiten des
 * Timeouts (5 Minuten) einen Neustart des ESP durch.
 */
void WiFi_Reconnect();