#pragma once

#include "Config.h"

#if MQTT_SUPPORTED == 1
#include <Arduino.h>
#include "ShineWifi.h"
#include "Growatt.h"
#include <PubSubClient.h>
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
  boolean mqttPublish(JsonDocument& doc, String topic = "");
  boolean mqttEnabled();
  boolean mqttConnected();
#if MQTT_COMMANDS == 1
  void onMqttMessage(char* topic, byte* payload, unsigned int length);
#endif
  void loop();
  const char* getId() const { return clientId; }

 private:
  WiFiClient& wifiClient;
  unsigned long previousConnectTryMillis = 0;
  MqttConfig mqttconfig;
  PubSubClient mqttclient;
  Growatt& inverter;
  // Optimierung 4: loop()-Taktung
  uint32_t lastMqttLoop = 0;
  char clientId[32];
};
#endif
