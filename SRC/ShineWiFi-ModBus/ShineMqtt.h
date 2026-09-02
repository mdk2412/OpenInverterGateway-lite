#pragma once

#include "Config.h"

#if MQTT_SUPPORTED == 1
#include <Arduino.h>
#include "ShineWifi.h"
#include "Growatt.h"
#include <PicoMQTT.h>
#include <stdbool.h>

typedef struct {
  String server;
  String port;
  String topic;
  String user;
  String pwd;
} MqttConfig;

class ShineMqtt {
 public:
  ShineMqtt(WiFiClient& wc, Growatt& inverter);
  void mqttSetup(const MqttConfig& config);
  bool mqttReconnect();
  boolean mqttPublish(JsonDocument& doc, const String& topic = "", uint8_t qos = 0, bool retain = false);
  boolean mqttEnabled();
  boolean mqttConnected();
#if MQTT_COMMANDS == 1
  void onMqttMessage(char* topic, byte* payload, unsigned int length);
#endif
  void loop();
  // const char* getId() const { return clientId; }

 private:
  void subscribeTopics(); // Hilfsmethode für (Re-)Subscriptions

  WiFiClient& wifiClient;
  unsigned long previousConnectTryMillis = 0;
  MqttConfig mqttconfig;
  PicoMQTT::Client* mqttclient;
  Growatt& inverter;
  // Optimierung 4: loop()-Taktung
  uint32_t lastMqttLoop = 0;
  char clientId[32];
};
#endif