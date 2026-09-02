#include "ShineMqtt.h"
#include "Growatt.h"

#if MQTT_SUPPORTED == 1
#include <TLog.h>
#include <MQTT.h>

ShineMqtt::ShineMqtt(WiFiClient& wc, Growatt& inverter)
    : wifiClient(wc), inverter(inverter), mqttclient(4096) {
  snprintf(clientId, sizeof(clientId), "growatt-min_tl-xh-%08x",
           (uint32_t)ESP.getChipId());
  
  // Initialize MQTT client with WiFi connection
  mqttclient.begin(wifiClient);
  
#if MQTT_COMMANDS == 1
  // Set up message callback for the MQTT library
  mqttclient.onMessage([this](String &topic, String &payload) {
    this->onMqttMessage((char*)topic.c_str(), (byte*)payload.c_str(), payload.length());
  });
#endif
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

  Log.printf(
      "MQTT Configuration:\n  MQTT Server: %s\n  MQTT User:   %s\n  MQTT Port: "
      "  %u\n  MQTT Topic:  %s\n",
      mqttconfig.server.c_str(), mqttconfig.user.c_str(), port,
      mqttconfig.topic.c_str());

  // Configure MQTT connection parameters
  mqttclient.setHost(mqttconfig.server.c_str(), port);
  mqttclient.setCleanSession(true);
  
  mqttConfigured = true;
}

// -------------------------------------------------------
// Stabile Reconnect-Logik
// -------------------------------------------------------
bool ShineMqtt::mqttReconnect() {
  if (!mqttEnabled() || !mqttConfigured || WiFi.status() != WL_CONNECTED) 
    return false;
  
  if (mqttclient.connected()) 
    return true;

  // Intervall prüfen (5 Sekunden)
  uint32_t now = millis();
  if (now - previousConnectTryMillis < 5000) 
    return false;
  previousConnectTryMillis = now;

  Log.print(F("MQTT Connection... "));

  // Sicherstellen, dass alte Zustände aufgeräumt sind
  mqttclient.disconnect();

  bool ok;
  if (!mqttconfig.user.isEmpty()) {
    ok = mqttclient.connect(clientId, mqttconfig.user.c_str(), mqttconfig.pwd.c_str());
  } else {
    ok = mqttclient.connect(clientId);
  }

  if (!ok) {
    Log.println(F("failed"));
    return false;
  }

  Log.println(F("succeeded"));

#if MQTT_COMMANDS == 1
  char commandTopic[128];
  snprintf(commandTopic, sizeof(commandTopic), "%s/command/#",
           mqttconfig.topic.c_str());
  
  bool success = mqttclient.subscribe(commandTopic);
  Log.printf("%s: %s\n", success ? "Subscribed" : "Subscribe failed",
             commandTopic);
#endif

  return true;
}

// -------------------------------------------------------
// Publish JSON-Dokument
// -------------------------------------------------------
boolean ShineMqtt::mqttPublish(JsonDocument& doc, const String& topic) {
  if (!mqttclient.connected()) 
    return false;
  
  const String& t = !topic.isEmpty() ? topic : mqttconfig.topic;

  // Puffer verarbeiten vor dem Erstellen des Payloads
  mqttclient.loop();

  // Serialize JSON to a String buffer
  String payload;
  serializeJson(doc, payload);

  // QoS 0 (Standard) und Retain=true verwenden
  bool success = mqttclient.publish(t, payload, true, 0);

  if (!success) {
    Log.printf("MQTT Error: Publish to %s failed\n", t.c_str());
    // Verbindung trennen, um sauberen Reconnect im nächsten Cycle zu erzwingen
    mqttclient.disconnect();
    return false;
  }

  mqttclient.loop();
  return true;
}

// -------------------------------------------------------
// MQTT Commands
// -------------------------------------------------------
#if MQTT_COMMANDS == 1
void ShineMqtt::onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String strTopic(topic);
  String prefix = mqttconfig.topic + "/command/";

  if (!strTopic.startsWith(prefix)) 
    return;

  String command = strTopic.substring(prefix.length());
  if (command.isEmpty()) 
    return;

  // %.*s erwartet zuerst die Länge als int und dann den Zeiger char*
  Log.printf("Received Command via MQTT: %s %.*s\n", command.c_str(),
             (int)length, (char*)payload);

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