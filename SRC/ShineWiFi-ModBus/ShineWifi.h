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

bool ShineWifiDisconnect();