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
boolean ShineMqtt::mqttConnected() { return mqttclient && mqttclient->connected(); }

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
  // Verwende '#' statt '+'
  snprintf(commandTopic, sizeof(commandTopic), "%s/command/#",
           mqttconfig.topic.c_str());

  Log.printf("MQTT Subscribing to topic pattern: %s\n", commandTopic);

  mqttclient->subscribe(commandTopic, [this](const char* topic, const char* payload) {
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

  // 1. Exakte Länge berechnen, BEVOR gesendet wird
  size_t len = measureJson(doc);

  if (len > BUFFER_SIZE) {
    Log.printf("MQTT Error: Payload Size (%u) > BUFFER_SIZE (%u)\n", (unsigned int)len, BUFFER_SIZE);
    return false;
  }

  // 2. JSON in einen statischen Puffer serialisieren (nicht auf dem Stack!)
  static char buffer[BUFFER_SIZE];
  size_t bytesWritten = serializeJson(doc, buffer, sizeof(buffer));

  if (bytesWritten == 0 || bytesWritten != len) {
    Log.printf("MQTT Error: Serialization failed (%u/%u bytes)\n", (unsigned int)bytesWritten, (unsigned int)len);
    return false;
  }

  // 3. Mit PicoMQTT publishen
  bool success = mqttclient->publish(t.c_str(), buffer);

  if (!success) {
    Log.println(F("MQTT Error: Publish failed"));
    return false;
  }

  return true;
}

// -------------------------------------------------------
// MQTT Commands
// -------------------------------------------------------
#if MQTT_COMMANDS == 1
void ShineMqtt::onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String strTopic(topic);
  String prefix = mqttconfig.topic + "/command/";

  if (!strTopic.startsWith(prefix)) return;

  String command = strTopic.substring(prefix.length());
  if (command.isEmpty()) return;

  // %.*s erwartet zuerst die Länge als int und dann den Zeiger char*
  Log.printf("Received Command via MQTT: %s %.*s\n", command.c_str(),
             (int)length, (char*)payload);

  // Use static allocation to avoid stack overflow
  static StaticJsonDocument<1024> req;
  static StaticJsonDocument<1024> res;
  
  // Clear previous contents
  req.clear();
  res.clear();

  // Übergabe des empfangenen Puffers an den Handler
  inverter.HandleCommand(command, payload, length, req, res);

  mqttPublish(res, mqttconfig.topic + "/result");
}
#endif

// -------------------------------------------------------
void ShineMqtt::loop() {
  mqttReconnect();
  if (mqttclient)
    mqttclient->loop();
}

#endif