#include "ShineMqtt.h"
#include "Growatt.h"

#if MQTT_SUPPORTED == 1
#include <TLog.h>
#include <StreamUtils.h>
#include <PubSubClient.h>

ShineMqtt::ShineMqtt(WiFiClient& wc, Growatt& inverter)
    : wifiClient(wc), mqttclient(wifiClient), inverter(inverter) {
  mqttclient.setBufferSize(BUFFER_SIZE);
  // Optimierung 2: schnelleres Timeout
  mqttclient.setSocketTimeout(2);
}

// -------------------------------------------------------
// Sichere, gültige MQTT-Client-ID
// -------------------------------------------------------
String ShineMqtt::getId() {
#if defined(ESP8266)
  uint32_t id = ESP.getChipId();
#elif defined(ESP32)
  uint32_t id = (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF);
#endif
  return "Growatt-" + String(id, HEX);
}

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
  if (!mqttEnabled()) return false;
  if (WiFi.status() != WL_CONNECTED) return false;

  // Bereits verbunden?
  if (mqttclient.connected()) return true;

  // Reconnect nur alle 5 Sekunden
  if (millis() - previousConnectTryMillis < 5000) return false;
  previousConnectTryMillis = millis();

  Log.print(F("MQTT Connection... "));

  bool ok = mqttclient.connect(getId().c_str(), mqttconfig.user.c_str(),
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
// Publish String
// -------------------------------------------------------
boolean ShineMqtt::mqttPublish(const String& jsonString) {
  if (!mqttclient.connected()) return false;

  if (jsonString.length() >= BUFFER_SIZE) {
    Log.println(F("MQTT Message too long!"));
    return false;
  }

  return mqttclient.publish(mqttconfig.topic.c_str(), jsonString.c_str(), true);
}

// -------------------------------------------------------
// Publish JSON-Dokument
// -------------------------------------------------------
boolean ShineMqtt::mqttPublish(JsonDocument& doc, String topic) {
  if (!mqttclient.connected()) return false;
  if (topic.isEmpty()) topic = mqttconfig.topic;

  size_t len = measureJson(doc);

  if (!mqttclient.beginPublish(topic.c_str(), len, true)) {
    Log.println(F("beginPublish failed"));
    return false;
  }

  BufferingPrint buffered(mqttclient, BUFFER_SIZE);
  serializeJson(doc, buffered);
  buffered.flush();

  return mqttclient.endPublish();
}

// -------------------------------------------------------
// MQTT Commands
// -------------------------------------------------------
#if MQTT_COMMANDS == 1
void ShineMqtt::onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String strTopic(topic);

  Log.print(F("MQTT Message: ["));
  Log.print(strTopic);
  Log.print(F("] "));

  // Sichere Payload-Kopie
  String messagePayload;
  messagePayload.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) {
    messagePayload += (char)payload[i];
  }
  Log.println(messagePayload);

  String prefix = mqttconfig.topic + "/command/";
  if (!strTopic.startsWith(prefix)) return;

  String command = strTopic.substring(prefix.length());
  if (command.isEmpty()) return;

  StaticJsonDocument<1024> req;
  StaticJsonDocument<1024> res;

  inverter.HandleCommand(command, (byte*)messagePayload.c_str(),
                         messagePayload.length(), req, res);

  mqttPublish(res, mqttconfig.topic + "/result");
}
#endif

// -------------------------------------------------------
void ShineMqtt::loop() {
  uint32_t now = millis();

  // loop() mindestens alle 50ms aufrufen
  if ((uint32_t)(now - lastMqttLoop) >= 50) {
    mqttclient.loop();
    lastMqttLoop = now;
  }
}

#endif
