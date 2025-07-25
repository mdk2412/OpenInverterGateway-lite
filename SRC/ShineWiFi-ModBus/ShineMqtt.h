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
  boolean mqttPublish(const String& JsonString);
  boolean mqttPublish(JsonDocument& doc, String topic = "");
  boolean mqttEnabled();
  boolean mqttConnected();
#if MQTT_COMMANDS == 1
  void onMqttMessage(char* topic, byte* payload, unsigned int length);
#endif
  void loop();

 private:
  WiFiClient& wifiClient;
  unsigned long previousConnectTryMillis = 0;
  MqttConfig mqttconfig;
  PubSubClient mqttclient;
  Growatt& inverter;
  static String getId();
};
#endif
