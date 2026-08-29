#include "ShineMqtt.h"
#include "Growatt.h"

#if MQTT_SUPPORTED == 1
#include <TLog.h>

#include <PubSubClient.h>

ShineMqtt::ShineMqtt(WiFiClient& wc, Growatt& inverter)
    : wifiClient(wc), mqttclient(wifiClient), inverter(inverter) {
  mqttclient.setBufferSize(BUFFER_SIZE);
  // Optimierung 2: schnelleres Timeout
  mqttclient.setSocketTimeout(2);
}

// -------------------------------------------------------
// Sichere, gültige MQTT-Client-ID
// -------------------------------------------------------
void ShineMqtt::getId(char* buffer, size_t buflen) {
  snprintf(buffer, buflen, "growatt-min_tl-xh-%08X",
           (uint32_t)ESP.getChipId());
}

// -------------------------------------------------------
boolean ShineMqtt::mqttEnabled() { return !mqttconfig.server.isEmpty(); }
boolean ShineMqtt::mqttConnected() { return mqttclient.connected(); }

// -------------------------------------------------------
// Setup
// -------------------------------------------------------
void ShineMqtt::mqttSetup(const MqttConfig& config) {
  mqttconfig = config;

  uint16_t port = mqttconfig.port.toInt();
  if (port == 0) port = 1883;

  Log.print(F("MQTT Server: "));
  Log.println(mqttconfig.server);

  Log.print(F("MQTT User: "));
  Log.println(mqttconfig.user);

  Log.print(F("MQTT Port: "));
  Log.println(port);

  Log.print(F("MQTT Topic: "));
  Log.println(mqttconfig.topic);

  mqttclient.setServer(mqttconfig.server.c_str(), port);

#if MQTT_COMMANDS == 1
  mqttclient.setCallback(
      [this](char* topic, byte* payload, unsigned int length) {
        this->onMqttMessage(topic, payload, length);
      });
#endif
}

// -------------------------------------------------------
// Stabile Reconnect-Logik
// -------------------------------------------------------
bool ShineMqtt::mqttReconnect() {
  if (!mqttEnabled() || WiFi.status() != WL_CONNECTED) return false;
  if (mqttclient.connected()) return true;

  uint32_t now = millis();
  if (now - previousConnectTryMillis < 5000) return false;
  previousConnectTryMillis = now;

  Log.print(F("MQTT Connection... "));

  char clientId[32];
  getId(clientId, sizeof(clientId));

  bool ok = mqttclient.connect(clientId, mqttconfig.user.c_str(),
                               mqttconfig.pwd.c_str(), mqttconfig.topic.c_str(),
                               1, true, "{\"InverterStatus\": -1}");

  if (!ok) {
    Log.print(F("failed, rc="));
    Log.println(mqttclient.state());
    return false;
  }

  Log.println(F("succeeded"));

#if MQTT_COMMANDS == 1
  char commandTopic[128];
  snprintf(commandTopic, sizeof(commandTopic), "%s/command/#",
           mqttconfig.topic.c_str());
  if (mqttclient.subscribe(commandTopic, 1)) {
    Log.print(F("Subscribed: "));
    Log.println(commandTopic);
  } else {
    Log.print(F("Subscribe failed: "));
    Log.println(commandTopic);
  }
#endif

  return true;
}

// -------------------------------------------------------
// Publish JSON-Dokument
// -------------------------------------------------------
boolean ShineMqtt::mqttPublish(JsonDocument& doc, String topic) {
  if (!mqttclient.connected()) return false;

  const char* topicName = topic.length() ? topic.c_str() : mqttconfig.topic.c_str();

  char jsonBuffer[JSON_DOCUMENT_SIZE];
  size_t jsonLength = serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
  if (jsonLength == 0) return false;

  bool ok = mqttclient.publish(topicName, (const uint8_t*)jsonBuffer, jsonLength,
                              true);
  //Log.println(ok ? "succeed" : "failed");
  return ok;
}

// -------------------------------------------------------
// MQTT Commands
// -------------------------------------------------------
#if MQTT_COMMANDS == 1
void ShineMqtt::onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String strTopic(topic);

  Log.print(F("MQTT Message: ["));
  Log.print(strTopic);
  Log.print(F("] "));

  // Copy payload into a fixed buffer to avoid repeated heap churn on small
  // receive buffers.
  char payloadBuffer[256];
  size_t payloadLen = length < sizeof(payloadBuffer) - 1u
                          ? length
                          : sizeof(payloadBuffer) - 1u;
  if (payloadLen > 0) {
    memcpy(payloadBuffer, payload, payloadLen);
  }
  payloadBuffer[payloadLen] = '\0';
  Log.println(payloadBuffer);

  char prefix[128];
  snprintf(prefix, sizeof(prefix), "%s/command/", mqttconfig.topic.c_str());
  if (strncmp(topic, prefix, strlen(prefix)) != 0) return;

  String command = String(topic + strlen(prefix));
  if (command.isEmpty()) return;

  StaticJsonDocument<1024> req;
  StaticJsonDocument<1024> res;

  inverter.HandleCommand(command, (const byte*)payloadBuffer, payloadLen, req,
                        res);

  mqttPublish(res, mqttconfig.topic + "/result");
}
#endif

// -------------------------------------------------------
void ShineMqtt::loop() {
  mqttReconnect();

  uint32_t now = millis();
  if (now - lastMqttLoop >= 50) {
    mqttclient.loop();
    lastMqttLoop = now;
  }
}

#endif
