#include "ShineMqtt.h"
#include "Growatt.h"

#if MQTT_SUPPORTED == 1
#include <TLog.h>
#include <PicoMQTT.h>

ShineMqtt::ShineMqtt(WiFiClient& wc, Growatt& inverter)
    : wifiClient(wc), mqttclient(nullptr), inverter(inverter) {
  snprintf(clientId, sizeof(clientId), "growatt-min_tl-xh-%08x",
           (uint32_t)ESP.getChipId());
}

boolean ShineMqtt::mqttEnabled() { return !mqttconfig.server.isEmpty(); }
boolean ShineMqtt::mqttConnected() {
  return mqttclient && mqttclient->connected();
}

// -------------------------------------------------------
// Setup
// -------------------------------------------------------
void ShineMqtt::mqttSetup(const MqttConfig& config) {
  mqttconfig = config;

  uint16_t port = mqttconfig.port.toInt();
  if (port == 0) port = 1883;

  Log.printf(
      "MQTT Configuration:\n  MQTT Server: %s\n  MQTT User:   %s\n  MQTT Port: "
      "  %u\n  MQTT Topic:  %s\n",
      mqttconfig.server.c_str(), mqttconfig.user.c_str(), port,
      mqttconfig.topic.c_str());

  // Initialize PicoMQTT client with broker hostname
  if (mqttclient != nullptr) {
    delete mqttclient;
  }
  mqttclient = new PicoMQTT::Client(mqttconfig.server.c_str());
  mqttclient->port = port;
  mqttclient->client_id = clientId;

  // Set credentials if provided
  if (!mqttconfig.user.isEmpty()) {
    mqttclient->username = mqttconfig.user.c_str();
    mqttclient->password = mqttconfig.pwd.c_str();
  }

#if MQTT_COMMANDS == 1
  // Explicit als String übergeben
  String commandTopic = mqttconfig.topic + "/command/#";

  Log.printf("MQTT Subscribing to topic pattern: %s\n", commandTopic.c_str());

  // Subscribe registrieren
  mqttclient->subscribe(
      commandTopic, [this](const char* topic, const char* payload) {
        Log.printf("MQTT Command received on topic: %s\n", topic);
        this->onMqttMessage((char*)topic, (byte*)payload,
                            (unsigned int)strlen(payload));
      });
#endif

  // erst DANACH connecten oder loop() starten!

  // Erst NACH allen subscribe()-Aufrufen begin() starten!
  mqttclient->begin();
}

// -------------------------------------------------------
// Stabile Reconnect-Logik
// -------------------------------------------------------
bool ShineMqtt::mqttReconnect() {
  if (!mqttEnabled() || WiFi.status() != WL_CONNECTED) return false;
  if (mqttclient && mqttclient->connected()) return true;

  // PicoMQTT handles reconnection automatically in loop()
  // This function just ensures we have a client instance
  return mqttclient != nullptr;
}

// -------------------------------------------------------
// Publish JSON-Dokument
// -------------------------------------------------------
// ShineMqtt.cpp
// ShineMqtt.cpp
boolean ShineMqtt::mqttPublish(JsonDocument& doc, const String& topic,
                               uint8_t qos, bool retain) {
  if (!mqttclient || !mqttclient->connected()) return false;

  const String& t = !topic.isEmpty() ? topic : mqttconfig.topic;

  // 1. Exakte JSON-Länge berechnen
  size_t len = measureJson(doc);

  // 2. Stream-Publish mit dynamischem QoS (0, 1, 2) und Retain-Flag
  auto publish_stream = mqttclient->begin_publish(t.c_str(), len, qos, retain);

  // 3. Directly serialize into the TCP stream (Zero-Buffer)
  size_t bytesWritten = serializeJson(doc, publish_stream);

  if (bytesWritten != len) {
    Log.printf("MQTT Error: Serialization incomplete (%u/%u bytes)\n",
               (unsigned int)bytesWritten, (unsigned int)len);
    return false;
  }

  // 4. Stream leeren & TCP-Pakete absenden
  publish_stream.flush();

  return true;
}

// -------------------------------------------------------
// MQTT Commands
// -------------------------------------------------------
#if MQTT_COMMANDS == 1
void ShineMqtt::onMqttMessage(char* topic, byte* payload, unsigned int length) {
  const char* baseTopic = mqttconfig.topic.c_str();
  size_t baseLen = strlen(baseTopic);
  const char* commandSuffix = "/command/";
  size_t suffixLen = strlen(commandSuffix);

  // 1. Zero-Copy Topic-Match
  if (strncmp(topic, baseTopic, baseLen) != 0 ||
      strncmp(topic + baseLen, commandSuffix, suffixLen) != 0) {
    return;
  }

  // 2. Zeiger-Arithmetik statt String.substring()
  char* command = topic + baseLen + suffixLen;
  if (*command == '\0') return;

  static StaticJsonDocument<1024> req;
  static StaticJsonDocument<1024> res;
  req.clear();
  res.clear();

  // 3. Übergabe der rohen Zeiger an den Inverter
  inverter.HandleCommand(command, payload, length, req, res);

  // 4. Zero-Copy Antwort-Topic zusammenbauen (OHNE String-Verknüpfung)
  char resultTopic[128];
  snprintf(resultTopic, sizeof(resultTopic), "%s/result",
           mqttconfig.topic.c_str());

  // Publish nutzt unser Zero-Buffer Streaming!
  mqttPublish(res, resultTopic);
}
#endif

// -------------------------------------------------------
void ShineMqtt::loop() {
  mqttReconnect();
  if (mqttclient) mqttclient->loop();
}

#endif