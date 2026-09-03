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
  // DIESE ZEILE MUSS EXAKT SO IM HEADER STEHEN:
  ShineMqtt(Growatt& inverter); 
  ~ShineMqtt();

  void mqttSetup(const MqttConfig& config);
  void loop();
  boolean mqttEnabled();
  boolean mqttConnected();
  bool mqttReconnect();

  boolean mqttPublish(JsonDocument& doc, const String& topic = "",
                      uint8_t qos = 0, bool retain = false);

 private:
  void subscribeTopics();

  uint32_t previousConnectTryMillis;
  PicoMQTT::Client* mqttclient;
  Growatt& inverter;
  
  MqttConfig mqttconfig;
  char clientId[32];
  uint32_t lastMqttLoop;
  bool lastConnectedState;
};
#endif