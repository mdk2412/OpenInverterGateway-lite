#include "ShineMqtt.h"

#if MQTT_SUPPORTED == 1
#include <TLog.h>

// -------------------------------------------------------
// Konstruktor & Destruktor
// -------------------------------------------------------
ShineMqtt::ShineMqtt(WiFiClient& wc, Growatt& inverter)
    : wifiClient(wc),
      previousConnectTryMillis(0),
      mqttclient(nullptr),
      inverter(inverter),
      lastMqttLoop(0),
      lastConnectedState(false) {
  snprintf(clientId, sizeof(clientId), "growatt-min_tl-xh-%08x",
           (uint32_t)ESP.getChipId());
}

ShineMqtt::~ShineMqtt() {
  if (mqttclient != nullptr) {
    delete mqttclient;
    mqttclient = nullptr;
  }
}

// -------------------------------------------------------
// Status-Abfragen & Validierung
// -------------------------------------------------------
boolean ShineMqtt::mqttEnabled() { return !mqttconfig.server.isEmpty(); }

boolean ShineMqtt::mqttConnected() {
  return mqttclient && mqttclient->connected();
}

bool ShineMqtt::mqttReconnect() {
  if (!mqttEnabled() || WiFi.status() != WL_CONNECTED) return false;
  return mqttclient != nullptr;
}

// -------------------------------------------------------
// Helper: Subscriptions registrieren
// -------------------------------------------------------
void ShineMqtt::subscribeTopics() {
#if MQTT_COMMANDS == 1
  if (!mqttclient) return;

  // 1. Topic-Muster für Subskription
  String commandTopicPattern = mqttconfig.topic + "/command/#";

  Log.printf("MQTT Subscribing to Topic: %s\n", commandTopicPattern.c_str());

  mqttclient->subscribe(
      commandTopicPattern.c_str(),
      [this](const char* topic, const char* payload) {
        // 2. Präfix-Länge dynamisch berechnen (<baseTopic>/command/)
        const size_t prefixLen =
            mqttconfig.topic.length() + 9;  // 9 = strlen("/command/")

        // 3. Befehl isolieren (Zero-Copy)
        const char* command =
            (strlen(topic) >= prefixLen) ? (topic + prefixLen) : topic;
        const char* safePayload = payload ? payload : "";

        // 4. Wunschausgabe
        Log.printf("Received Command: %s %s\n", command, safePayload);

        // 5. ArduinoJson v7: Automatische RAM-Verwaltung auf dem Stack (RAII)
        JsonDocument req;
        JsonDocument res;

        // 6. Übergabe an bestehenden Handler (unveränderte Schnittstelle)
        const unsigned int payloadLen = strlen(safePayload);
        inverter.HandleCommand(command, (const byte*)safePayload, payloadLen,
                               req, res);

        // 7. Effizientes Senden ohne dynamisches Re-Allokieren
        if (!res.isNull() && res.size() > 0) {
          String resultTopic = mqttconfig.topic + "/result";

          String responsePayload;
          // Reserviert im Voraus Speicher, um Re-Allokationen beim
          // Serialisieren zu vermeiden
          responsePayload.reserve(measureJson(res) + 1);
          serializeJson(res, responsePayload);

          mqttclient->publish(resultTopic.c_str(), responsePayload.c_str());
        }
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

  // Vorherige Instanz löschen, falls re-initialisiert wird
  if (mqttclient != nullptr) {
    delete mqttclient;
    mqttclient = nullptr;
  }

  mqttclient = new PicoMQTT::Client(mqttconfig.server.c_str());
  mqttclient->port = port;
  mqttclient->client_id = clientId;

  if (!mqttconfig.user.isEmpty()) {
    mqttclient->username = mqttconfig.user.c_str();
    mqttclient->password = mqttconfig.pwd.c_str();
  }

  // Subscriptions direkt beim Setup einmalig anlegen
#if MQTT_COMMANDS == 1
  subscribeTopics();
#endif

  mqttclient->begin();
}

// -------------------------------------------------------
// Loop
// -------------------------------------------------------
void ShineMqtt::loop() {
  if (!mqttReconnect()) {
    lastConnectedState = false;
    return;
  }

  bool currentlyConnected = mqttclient->connected();

  // Statuswechsel protokollieren
  if (currentlyConnected && !lastConnectedState) {
    Log.printf("MQTT connected\n");
  } else if (!currentlyConnected && lastConnectedState) {
    Log.printf("MQTT disconnected\n");
  }

  lastConnectedState = currentlyConnected;

  // PicoMQTT kümmert sich intern um die Netzwerkausführung
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

  // 2. Stream-Publish starten
  auto publish_stream = mqttclient->begin_publish(t.c_str(), len, qos, retain);

  // 3. Direkt in den TCP-Stream serialisieren
  size_t bytesWritten = serializeJson(doc, publish_stream);

  if (bytesWritten != len) {
    Log.printf("MQTT Error: Serialization incomplete (%u/%u bytes)\n",
               (unsigned int)bytesWritten, (unsigned int)len);
    return false;
  }

  // 4. Stream leeren & TCP-Pakete senden
  publish_stream.flush();

  return true;
}

// -------------------------------------------------------
// MQTT Command Handler (ArduinoJson v7)
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

  // 2. Zeiger-Arithmetik
  char* command = topic + baseLen + suffixLen;
  if (*command == '\0') return;

  // 3. Lokale Dokumente (RAM-Freigabe sofort nach Funktionsende)
  JsonDocument req;
  JsonDocument res;

  // 4. Übergabe der Daten an den Inverter
  inverter.HandleCommand(command, payload, length, req, res);

  // 5. Antwort-Topic mit Pufferprüfung zusammenbauen
  char resultTopic[128];
  int ret = snprintf(resultTopic, sizeof(resultTopic), "%s/result", baseTopic);

  if (ret > 0 && ret < (int)sizeof(resultTopic)) {
    mqttPublish(res, resultTopic);
  } else {
    Log.printf("MQTT Error: Result topic truncated/overflowed\n");
  }
}
#endif

#endif  // MQTT_SUPPORTED == 1