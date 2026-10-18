#include "ShineMqtt.h"

#if defined(ESP32)
#include <string>
#endif

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
#ifdef ESP8266
  snprintf(clientId, sizeof(clientId), "growatt-min_tl-xh-%08x",
           static_cast<uint32_t>(ESP.getChipId()));
#elif defined(ESP32)
  snprintf(clientId, sizeof(clientId), "growatt-min_tl-xh-%012llx",
           static_cast<unsigned long long>(ESP.getEfuseMac() & 0xFFFFFFFFFFFFULL));
#endif
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

  // 1. Port sicher parsen und validieren (1 bis 65535)
  long parsedPort = mqttconfig.port.toInt();
  uint16_t port =
      (parsedPort > 0 && parsedPort <= 65535) ? (uint16_t)parsedPort : 1883;

  Log.printf(PSTR("MQTT Configuration:\n    MQTT Server: %s\n    MQTT User:   "
                  "%s\n    MQTT "
                  "Port:   %u\n    MQTT Topic:  %s\n"),
             mqttconfig.server.c_str(), mqttconfig.user.c_str(), port,
             mqttconfig.topic.c_str());

  if (mqttclient != nullptr) {
    delete mqttclient;
    mqttclient = nullptr;
  }

  // 2. Jetzt ist 'port' bekannt und validiert
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
      Log.printf(PSTR("MQTT disconnected (WiFi down)\n"));
      lastConnectedState = false;
    }
    return;
  }

  // PicoMQTT kümmert sich um Connect, Reconnect & Keep-Alive/Ping
  mqttclient->loop();

  bool currentlyConnected = mqttclient->connected();

  if (currentlyConnected && !lastConnectedState) {
    Log.printf(PSTR("MQTT connected\n"));
  } else if (!currentlyConnected && lastConnectedState) {
    Log.printf(PSTR("MQTT disconnected\n"));
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

#if defined(ESP32)
  static std::string payload;
  payload.clear();

  const size_t payloadSize = serializeJson(doc, payload);
  if (payloadSize != payload.size()) return false;

  return mqttclient->publish(t.c_str(),
                             static_cast<const void*>(payload.data()),
                             payloadSize, qos, retain);
#else
  // 1. Stream starten
  auto publishStream =
      mqttclient->begin_publish(t.c_str(), measureJson(doc), qos, retain);

  // 2. JSON in den Stream schreiben
  serializeJson(doc, publishStream);

  // 3. Stream leeren/schließen (gibt void zurück)
  publishStream.flush();

  return true;
#endif
}

// =======================================================
// 5. STATUS-ABFRAGEN & PRÜFUNGEN
// =======================================================
boolean ShineMqtt::mqttEnabled() const { return !mqttconfig.server.isEmpty(); }

boolean ShineMqtt::mqttConnected() const {
  return mqttclient && mqttclient->connected();
}

// =======================================================
// 6. INTERNE HELPER (Private Subscriptions / Handler)
// =======================================================
void ShineMqtt::subscribeTopics() {
#if MQTT_COMMANDS == 1
  if (!mqttclient) return;

  // Vereinfachte String-Verknüpfung
  String commandTopicPattern = mqttconfig.topic + F("/command/#");

  Log.printf(PSTR("MQTT Subscribing to Topic: %s\n"),
             commandTopicPattern.c_str());

  mqttclient->subscribe(commandTopicPattern.c_str(), [this](
                                                         const char* topic,
                                                         const char* payload) {
    const size_t prefixLen =
        mqttconfig.topic.length() + 9;  // strlen("/command/") = 9

    if (strlen(topic) < prefixLen) return;

    const char* command = topic + prefixLen;
    const char* safePayload = payload ? payload : "";

    Log.printf(PSTR("Received Command: %s %s\n"), command, safePayload);

    JsonDocument req;
    JsonDocument res;

    auto publishResult = [this](JsonDocument& response) {
      String resultTopic = mqttconfig.topic + F("/result");
      auto publishStream = mqttclient->begin_publish(
          resultTopic.c_str(), measureJson(response), 0, false);
      serializeJson(response, publishStream);
      publishStream.flush();
    };

    if (safePayload[0] != '\0') {
      DeserializationError err = deserializeJson(req, safePayload);
      if (err) {
        Log.printf(PSTR("MQTT Payload JSON parse error: %s\n"), err.c_str());

        res[F("command")] = command;
        res[F("success")] = false;

        char message[64];
        snprintf_P(message, sizeof(message), PSTR("Invalid JSON Payload: %s"),
                   err.c_str());
        res[F("message")] = message;

        publishResult(res);
        return;
      }
    }

    // Inverter Befehl ausführen
    inverter.HandleCommand(command, req, res);

    // Antwort zurücksenden
    if (!res.isNull()) {
      publishResult(res);
    }
  });
#endif
}

#endif  // MQTT_SUPPORTED == 1