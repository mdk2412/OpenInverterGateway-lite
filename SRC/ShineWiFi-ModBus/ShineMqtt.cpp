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
  static char commandTopic[128];
  snprintf(commandTopic, sizeof(commandTopic), "%s/command/#",
           mqttconfig.topic.c_str());

  Log.printf("MQTT Subscribing to topic pattern: %s\n", commandTopic);

  // ZWEI Parameter: topic und payload
  mqttclient->subscribe(
      commandTopic, [this](const char* topic, const char* payload) {
        this->onMqttMessage((char*)topic, (byte*)payload, strlen(payload));
      });
#endif

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
boolean ShineMqtt::mqttPublish(JsonDocument& doc, const String& topic) {
  if (!mqttclient || !mqttclient->connected()) return false;
  const String& t = !topic.isEmpty() ? topic : mqttconfig.topic;

  // 1. Exakte Länge des JSON berechnen
  size_t len = measureJson(doc);

  // 2. Stream-Publish bei PicoMQTT starten (übermittelt Topic & Content-Length)
  auto publish_stream = mqttclient->begin_publish(t.c_str(), len);

  // 3. Directly Stream: ArduinoJson schreibt direkt in den PicoMQTT-Socket!
  // Es wird KEIN lokaler/statischer char-Puffer mehr benötigt!
  size_t bytesWritten = serializeJson(doc, publish_stream);

  if (bytesWritten != len) {
    Log.printf("MQTT Error: Serialization incomplete (%u/%u bytes)\n",
               (unsigned int)bytesWritten, (unsigned int)len);
    return false;
  }

  // 4. Stream leeren/abschließen (flush hat void als Rückgabe)
  publish_stream.flush();

  return true;
}

// -------------------------------------------------------
// MQTT Commands
// -------------------------------------------------------
#if MQTT_COMMANDS == 1
void ShineMqtt::onMqttMessage(char* topic, byte* payload, unsigned int length) {
  // 1. Zero-Copy Topic-Prüfung ohne String-Instanziierung
  const char* baseTopic = mqttconfig.topic.c_str();
  size_t baseLen = strlen(baseTopic);
  const char* commandSuffix = "/command/";
  size_t suffixLen = strlen(commandSuffix);

  // Sicherstellen, dass das Topic mit "<mqttconfig.topic>/command/" beginnt
  if (strncmp(topic, baseTopic, baseLen) != 0 ||
      strncmp(topic + baseLen, commandSuffix, suffixLen) != 0) {
    return;
  }

  // Der Command-Name beginnt direkt nach "/command/" (Zero-Copy-Pointer)
  const char* command = topic + baseLen + suffixLen;
  if (*command == '\0') return;

  Log.printf("Received Command via MQTT: %s %.*s\n", command, (int)length,
             (char*)payload);

  // Static JSON Docs wiederverwenden
  static StaticJsonDocument<1024> req;
  static StaticJsonDocument<1024> res;
  req.clear();
  res.clear();

  // Übergabe ohne Umkopieren
  inverter.HandleCommand(command, payload, length, req, res);

  // Ergebnis per Stream publishen
  mqttPublish(res, mqttconfig.topic + "/result");
}
#endif

// -------------------------------------------------------
void ShineMqtt::loop() {
  mqttReconnect();
  if (mqttclient) mqttclient->loop();
}

#endif