#include "ShineMqtt.h"

#if MQTT_SUPPORTED == 1
#include <TLog.h>

// =======================================================
// 1. LEBENSZYKLUS (Konstruktor & Destruktor)
// =======================================================
ShineMqtt::ShineMqtt(Growatt& inverter)
    : previousConnectTryMillis(0),
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

// =======================================================
// 2. INITIALISIERUNG & SETUP
// =======================================================
void ShineMqtt::mqttSetup(const MqttConfig& config) {
  mqttconfig = config;

  uint16_t port = mqttconfig.port.toInt();
  if (port == 0) port = 1883;

  Log.printf(
      "MQTT Configuration:\n    MQTT Server: %s\n    MQTT User:   %s\n    MQTT "
      "Port: "
      "  %u\n    MQTT Topic:  %s\n",
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

// =======================================================
// 3. LAUFZEIT-SCHLEIFE (Main Loop)
// =======================================================
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

// =======================================================
// 4. ÖFFENTLICHE API (Senden / Empfangen / Interaktion)
// =======================================================
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

// =======================================================
// 5. STATUS-ABFRAGEN & PRÜFUNGEN
// =======================================================
boolean ShineMqtt::mqttEnabled() { return !mqttconfig.server.isEmpty(); }

boolean ShineMqtt::mqttConnected() {
  return mqttclient && mqttclient->connected();
}

bool ShineMqtt::mqttReconnect() {
  if (!mqttEnabled() || WiFi.status() != WL_CONNECTED) return false;
  return mqttclient != nullptr;
}

// =======================================================
// 6. INTERNE HELPER (Private Subscriptions / Handler)
// =======================================================
void ShineMqtt::subscribeTopics() {
#if MQTT_COMMANDS == 1
  if (!mqttclient) return;

  // 1. Topic-Muster für Subskription
  String commandTopicPattern = mqttconfig.topic + "/command/#";

  Log.printf("MQTT Subscribing to Topic: %s\n", commandTopicPattern.c_str());

  mqttclient->subscribe(
      commandTopicPattern.c_str(),
      [this](const char* topic, const char* payload) {
        // 2. Präfix-Länge berechnen (<baseTopic>/command/)
        const size_t prefixLen =
            mqttconfig.topic.length() + 9;  // 9 = strlen("/command/")

        // 3. Sicherheitsprüfung: Abbrechen, falls das Topic zu kurz ist
        if (strlen(topic) < prefixLen) return;

        // 4. Befehl isolieren
        const char* command = topic + prefixLen;
        const char* safePayload = payload ? payload : "";

        // 5. Wunschausgabe
        Log.printf("Received Command: %s %s\n", command, safePayload);

        // 6. ArduinoJson v7: Dynamische Dokumente auf dem Stack
        JsonDocument req;
        JsonDocument res;

        // Falls eine Payload gesendet wurde, direkt in req deserialisieren
        if (safePayload[0] != '\0') {
          DeserializationError err = deserializeJson(req, safePayload);
          if (err) {
            Log.printf("MQTT Payload JSON parse error: %s\n", err.c_str());
          }
        }

        // 7. Übergabe an bestehenden Handler (v7 Signatur mit 3 Parametern)
        inverter.HandleCommand(command, req, res);

        // 8. Ergebnis zurücksenden
        if (!res.isNull() && res.size() > 0) {
          String resultTopic = mqttconfig.topic + "/result";

          String responsePayload;
          responsePayload.reserve(measureJson(res) + 1);
          serializeJson(res, responsePayload);

          mqttclient->publish(resultTopic.c_str(), responsePayload.c_str());
        }
      });
#endif
}

#endif  // MQTT_SUPPORTED == 1