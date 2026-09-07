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

  // Abschließende Slashes aus dem Basistopic entfernen
  while (mqttconfig.topic.endsWith("/")) {
    mqttconfig.topic.remove(mqttconfig.topic.length() - 1);
  }

  // 1. Port zwingend VOR der Verwendung parsen
  uint16_t port = mqttconfig.port.toInt();
  if (port == 0) port = 1883;

  Log.printf(
      "MQTT Configuration:\n    MQTT Server: %s\n    MQTT User:   %s\n    MQTT "
      "Port:   %u\n    MQTT Topic:  %s\n",
      mqttconfig.server.c_str(), mqttconfig.user.c_str(), port,
      mqttconfig.topic.c_str());

  if (mqttclient != nullptr) {
    delete mqttclient;
    mqttclient = nullptr;
  }

  // 2. Jetzt ist 'port' bekannt und kann übergeben werden
  mqttclient = new PicoMQTT::Client(mqttconfig.server.c_str(), port, clientId);

  if (!mqttconfig.user.isEmpty()) {
    mqttclient->username = mqttconfig.user.c_str();
    mqttclient->password = mqttconfig.pwd.c_str();
  }

#if MQTT_COMMANDS == 1
  subscribeTopics();
#endif

  // Starte den Hintergrund-Task von picoMQTT
  mqttclient->begin();
}

// =======================================================
// 3. LAUFZEIT-SCHLEIFE (Main Loop)
// =======================================================
void ShineMqtt::loop() {
  if (!mqttEnabled() || !mqttclient) {
    lastConnectedState = false;
    return;
  }

  // Bei fehlendem WLAN braucht loop() nicht aufgerufen zu werden
  if (WiFi.status() != WL_CONNECTED) {
    if (lastConnectedState) {
      Log.printf("MQTT disconnected (WiFi down)\n");
      lastConnectedState = false;
    }
    return;
  }

  // PicoMQTT kümmert sich um Connect, Reconnect & Keep-Alive/Ping
  mqttclient->loop();

  bool currentlyConnected = mqttclient->connected();

  if (currentlyConnected && !lastConnectedState) {
    Log.printf("MQTT connected\n");
  } else if (!currentlyConnected && lastConnectedState) {
    Log.printf("MQTT disconnected\n");
  }

  lastConnectedState = currentlyConnected;
}

// =======================================================
// 4. ÖFFENTLICHE API (Senden / Empfangen / Interaktion)
// =======================================================
boolean ShineMqtt::mqttPublish(JsonDocument& doc, const String& topic,
                               uint8_t qos, bool retain) {
  if (!mqttConnected()) return false;

  const String& t = !topic.isEmpty() ? topic : mqttconfig.topic;

  // 1. Stream starten
  auto publishStream = mqttclient->begin_publish(t.c_str(), measureJson(doc), qos, retain);

  // 2. JSON in den Stream schreiben
  serializeJson(doc, publishStream);

  // 3. Stream leeren/schließen (gibt void zurück)
  publishStream.flush();

  return true;
}

// =======================================================
// 5. STATUS-ABFRAGEN & PRÜFUNGEN
// =======================================================
boolean ShineMqtt::mqttEnabled() { 
  return !mqttconfig.server.isEmpty(); 
}

boolean ShineMqtt::mqttConnected() {
  return mqttclient && mqttclient->connected();
}

bool ShineMqtt::mqttReconnect() {
  // Diese Methode existiert nur zur Abwärtskompatibilität.
  // In picoMQTT prüft man nur, ob die Verbindung betriebsbereit ist.
  return mqttEnabled() && (WiFi.status() == WL_CONNECTED);
}

// =======================================================
// 6. INTERNE HELPER (Private Subscriptions / Handler)
// =======================================================
void ShineMqtt::subscribeTopics() {
#if MQTT_COMMANDS == 1
  if (!mqttclient) return;

  String commandTopicPattern = mqttconfig.topic + "/command/#";

  Log.printf("MQTT Subscribing to Topic: %s\n", commandTopicPattern.c_str());

  mqttclient->subscribe(
      commandTopicPattern.c_str(),
      [this](const char* topic, const char* payload) {
        const size_t prefixLen = mqttconfig.topic.length() + 9; // strlen("/command/") = 9

        if (strlen(topic) < prefixLen) return;

        const char* command = topic + prefixLen;
        const char* safePayload = payload ? payload : "";

        Log.printf("Received Command: %s %s\n", command, safePayload);

        JsonDocument req;
        JsonDocument res;

        if (safePayload[0] != '\0') {
          DeserializationError err = deserializeJson(req, safePayload);
          if (err) {
            Log.printf("MQTT Payload JSON parse error: %s\n", err.c_str());

            res["command"] = command;
            res["success"] = false;
            res["message"] = String("Invalid JSON Payload: ") + err.c_str();

            String resultTopic = mqttconfig.topic + "/result";
            String responsePayload;
            serializeJson(res, responsePayload);

            mqttclient->publish(resultTopic.c_str(), responsePayload.c_str());
            return;
          }
        }

        // Inverter Befehl ausführen
        inverter.HandleCommand(command, req, res);

        // Antwort zurücksenden
        if (!res.isNull()) {
          String resultTopic = mqttconfig.topic + "/result";
          String responsePayload;
          serializeJson(res, responsePayload);

          mqttclient->publish(resultTopic.c_str(), responsePayload.c_str());
        }
      });
#endif
}

#endif  // MQTT_SUPPORTED == 1