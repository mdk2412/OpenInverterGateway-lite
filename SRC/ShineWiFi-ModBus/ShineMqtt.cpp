#include "ShineMqtt.h"
#include "Growatt.h"

#if MQTT_SUPPORTED == 1
#include <TLog.h>
#include <PubSubClient.h>

ShineMqtt::ShineMqtt(WiFiClient& wc, Growatt& inverter)
    : wifiClient(wc), mqttclient(wifiClient), inverter(inverter) {
  mqttclient.setBufferSize(BUFFER_SIZE);
  // Schnelleres Timeout
  mqttclient.setSocketTimeout(2);
  snprintf(clientId, sizeof(clientId), "growatt-min_tl-xh-%08x",
           (uint32_t)ESP.getChipId());
}

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

  // Intervall prüfen (5 Sekunden)
  uint32_t now = millis();
  if (now - previousConnectTryMillis < 5000) return false;
  previousConnectTryMillis = now;

  Log.print(F("MQTT Connection... "));

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
boolean ShineMqtt::mqttPublish(JsonDocument& doc, const String& topic) {
  if (!mqttclient.connected()) return false;
  const String& t = !topic.isEmpty() ? topic : mqttconfig.topic;

  String output;
  serializeJson(doc, output);

  bool ok = mqttclient.beginPublish(t.c_str(), output.length(), true);
  if (ok) {
    mqttclient.print(output);
    ok = mqttclient.endPublish();
  }
  return ok;
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
  Log.printf("MQTT command received: %s %.*s\n", command.c_str(), (int)length,
             (char*)payload);

  StaticJsonDocument<1024> req;
  StaticJsonDocument<1024> res;

  // Übergabe des empfangenen Puffers an den Handler
  inverter.HandleCommand(command, payload, length, req, res);

  mqttPublish(res, mqttconfig.topic + "/result");
}
#endif

// -------------------------------------------------------
void ShineMqtt::loop() {
  mqttReconnect();
  mqttclient.loop();
}

#endif