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
  snprintf(clientId, sizeof(clientId), "growatt-min_tl-xh-%08x", (uint32_t)ESP.getChipId());
}

// -------------------------------------------------------
// Sichere, gültige MQTT-Client-ID
// -------------------------------------------------------
// String ShineMqtt::getId() {
//   return "growatt-min_tl-xh-" + String(ESP.getChipId(), HEX);
// }

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

  if (millis() - previousConnectTryMillis < 5000) return false;
  previousConnectTryMillis = millis();

  Log.print(F("MQTT Connection... "));

  bool ok = mqttclient.connect(getId(), mqttconfig.user.c_str(),
                               mqttconfig.pwd.c_str(), mqttconfig.topic.c_str(),
                               1, true, "{\"InverterStatus\": -1}");

  if (!ok) {
    Log.print(F("failed, rc="));
    Log.println(mqttclient.state());
    return false;
  }

  Log.println(F("succeeded"));

#if MQTT_COMMANDS == 1
  String commandTopic = mqttconfig.topic + "/command/#";
  if (mqttclient.subscribe(commandTopic.c_str(), 1)) {
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

  const String& t = topic.length() ? topic : mqttconfig.topic;

  size_t len = measureJson(doc);
  bool ok = mqttclient.beginPublish(t.c_str(), len, true);
  if (ok) {
    serializeJson(doc, mqttclient);
    ok = mqttclient.endPublish();
  }
  //Log.println(ok ? "succeed" : "failed");
  return ok;
}

// -------------------------------------------------------
// MQTT Commands
// -------------------------------------------------------
#if MQTT_COMMANDS == 1
void ShineMqtt::onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String strTopic(topic);  // Optional: Topic-Matching lässt sich auch ohne
                           // String machen, ist hier aber meist kurz

  String prefix = mqttconfig.topic + "/command/";
  if (!strTopic.startsWith(prefix)) return;

  String command = strTopic.substring(prefix.length());
  if (command.isEmpty()) return;

  StaticJsonDocument<1024> req;
  StaticJsonDocument<1024> res;

  // Direkte Übergabe des empfangenen Bytes-Puffers ohne Umweg über String
  // messagePayload:
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
