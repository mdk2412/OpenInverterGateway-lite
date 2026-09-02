#include "ShineMqtt.h"
#include "Growatt.h"

#if MQTT_SUPPORTED == 1
#include <TLog.h>
#include <PicoMQTT.h>

// -------------------------------------------------------
// Konstruktor
// -------------------------------------------------------
ShineMqtt::ShineMqtt(WiFiClient& wc, Growatt& inverter)
    : wifiClient(wc), mqttclient(nullptr), inverter(inverter) {
  snprintf(clientId, sizeof(clientId), "growatt-min_tl-xh-%08x",
           (uint32_t)ESP.getChipId());
}

// -------------------------------------------------------
// Status-Abfragen
// -------------------------------------------------------
boolean ShineMqtt::mqttEnabled() { 
  return !mqttconfig.server.isEmpty(); 
}

boolean ShineMqtt::mqttConnected() {
  return mqttclient && mqttclient->connected();
}

// -------------------------------------------------------
// Helper: Subscriptions registrieren
// -------------------------------------------------------
void ShineMqtt::subscribeTopics() {
#if MQTT_COMMANDS == 1
  if (!mqttclient) return;

  String commandTopic = mqttconfig.topic + "/command/#";
  Log.printf("MQTT Subscribing to Topic Pattern: %s\n", commandTopic.c_str());

  mqttclient->subscribe(
      commandTopic, [this](const char* topic, const char* payload) {
        Log.printf("MQTT Command received on topic: %s\n", topic);
        this->onMqttMessage((char*)topic, (byte*)payload,
                            (unsigned int)strlen(payload));
      });
#endif
}

// -------------------------------------------------------
// Setup / Initialisierung
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

  if (mqttclient != nullptr) {
    delete mqttclient;
  }
  mqttclient = new PicoMQTT::Client(mqttconfig.server.c_str());
  mqttclient->port = port;
  mqttclient->client_id = clientId;

  if (!mqttconfig.user.isEmpty()) {
    mqttclient->username = mqttconfig.user.c_str();
    mqttclient->password = mqttconfig.pwd.c_str();
  }

  // Erst-Subscriptions vor dem Start registrieren
  subscribeTopics();

  // Verbindungsaufbau starten
  mqttclient->begin();
}

// -------------------------------------------------------
// Reconnect-Prüfung
// -------------------------------------------------------
bool ShineMqtt::mqttReconnect() {
  if (!mqttEnabled() || WiFi.status() != WL_CONNECTED) return false;
  return mqttclient != nullptr;
}

// -------------------------------------------------------
// Loop mit automatischer Re-Subscription bei Reconnect
// -------------------------------------------------------
void ShineMqtt::loop() {
  if (!mqttReconnect()) return;

  static bool lastConnectedState = false;
  bool currentlyConnected = mqttclient->connected();

  // Statuswechsel erkennen: Von disconnected -> connected
  if (currentlyConnected && !lastConnectedState) {
    Log.printf("MQTT (Re)connected! Subscribing to Topics...\n");
    subscribeTopics();
  }

  lastConnectedState = currentlyConnected;

  // PicoMQTT verarbeitet Netzwerk und automatische TCP-Reconnects
  mqttclient->loop();
}

// -------------------------------------------------------
// Publish JSON-Dokument (Zero-Buffer Streaming)
// -------------------------------------------------------
boolean ShineMqtt::mqttPublish(JsonDocument& doc, const String& topic,
                               uint8_t qos, bool retain) {
  if (!mqttclient || !mqttclient->connected()) return false;

  const String& t = !topic.isEmpty() ? topic : mqttconfig.topic;

  // 1. Exakte JSON-Länge berechnen
  size_t len = measureJson(doc);

  // 2. Stream-Publish mit dynamischem QoS und Retain-Flag
  auto publish_stream = mqttclient->begin_publish(t.c_str(), len, qos, retain);

  // 3. Direkt in den TCP-Stream serialisieren
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
// MQTT Command Handler
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

  // 4. Zero-Copy Antwort-Topic zusammenbauen
  char resultTopic[128];
  snprintf(resultTopic, sizeof(resultTopic), "%s/result",
           mqttconfig.topic.c_str());

  // Publish nutzt Zero-Buffer Streaming
  mqttPublish(res, resultTopic);
}
#endif

#endif // MQTT_SUPPORTED == 1