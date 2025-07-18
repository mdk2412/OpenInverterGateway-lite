#include "Config.h"
#ifndef _SHINE_CONFIG_H_
#error Please rename Config.h.example to Config.h
#endif

#include "ShineWifi.h"
#include <TLog.h>
#include "Index.h"
#include "Growatt.h"
#include <Preferences.h>
#include <WiFiManager.h>
#include <StreamUtils.h>
#include <time.h>

// #define LOG_PRINTLN_TS(msg) { \
//   time_t now = time(nullptr); \
//   struct tm* t = localtime(&now); \
//   char timestamp[20]; \
//   strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t); \
//   char buffer[256]; \
//   snprintf(buffer, sizeof(buffer), "[%s] %s", timestamp, msg); \
//   Log.println(buffer); \
// }

#if (ENABLE_BATTERY_STANDBY == 1 || ACCHARGE_POWERRATE == 1)
#include "GrowattTLXH.h"
#endif

#ifdef ESP32
#include <esp_task_wdt.h>
#endif

#if PINGER_SUPPORTED == 1
#include <Pinger.h>
#include <PingerResponse.h>
#endif

#if ENABLE_DOUBLE_RESET == 1
#define ESP_DRD_USE_LITTLEFS true
#define ESP_DRD_USE_EEPROM false
#define DRD_TIMEOUT 10
#define DRD_ADDRESS 0
#include <ESP_DoubleResetDetector.h>
DoubleResetDetector* drd;
#endif

#if MQTT_SUPPORTED == 1
#include "ShineMqtt.h"
#endif

#if OTA_SUPPORTED == 1
#include <ArduinoOTA.h>
#endif

#if defined(DEFAULT_NTP_SERVER) && defined(DEFAULT_TZ_INFO)
#include <time.h>
extern "C" uint8_t sntp_getreachability(uint8_t);
#endif

Preferences prefs;
Growatt Inverter;
bool StartedConfigAfterBoot = false;

#if MQTT_SUPPORTED == 1
#ifdef MQTTS_ENABLED
WiFiClientSecure espClient;
#else
WiFiClient espClient;
#endif
ShineMqtt shineMqtt(espClient, Inverter);
#endif

#ifdef AP_BUTTON_PRESSED
byte btnPressed = 0;
#endif

#define NUM_OF_RETRIES 5
boolean readoutSucceeded = false;

uint16_t u16PacketCnt = 0;
#if PINGER_SUPPORTED == 1
Pinger pinger;
#endif

#ifdef ESP8266
ESP8266WebServer httpServer(80);
#elif ESP32
WebServer httpServer(80);
#endif

struct {
  WiFiManagerParameter* hostname = NULL;
  WiFiManagerParameter* static_ip = NULL;
  WiFiManagerParameter* static_netmask = NULL;
  WiFiManagerParameter* static_gateway = NULL;
  WiFiManagerParameter* static_dns = NULL;
#if MQTT_SUPPORTED == 1
  WiFiManagerParameter* mqtt_server = NULL;
  WiFiManagerParameter* mqtt_port = NULL;
  WiFiManagerParameter* mqtt_topic = NULL;
  WiFiManagerParameter* mqtt_user = NULL;
  WiFiManagerParameter* mqtt_pwd = NULL;
#endif
  WiFiManagerParameter* syslog_ip = NULL;
#if ENABLE_BATTERY_STANDBY == 1
  WiFiManagerParameter* sleep_battery_threshold = NULL;
  WiFiManagerParameter* wake_battery_threshold = NULL;
#endif
} customWMParams;

static const struct {
  const char* hostname = "/hostname";
  const char* static_ip = "/staticip";
  const char* static_netmask = "/staticnetmask";
  const char* static_gateway = "/staticgateway";
  const char* static_dns = "/staticdns";
#if MQTT_SUPPORTED == 1
  const char* mqtt_server = "/mqtts";
  const char* mqtt_port = "/mqttp";
  const char* mqtt_topic = "/mqttt";
  const char* mqtt_user = "/mqttu";
  const char* mqtt_pwd = "/mqttw";
#endif
  const char* syslog_ip = "/syslogip";
  const char* force_ap = "/forceap";
#if ENABLE_BATTERY_STANDBY == 1
  const char* sleep_battery_threshold = "/sleepbatterythreshold";
  const char* wake_battery_threshold = "/wakebatterythreshold";
#endif
} ConfigFiles;

struct {
  String hostname;
  String static_ip;
  String static_netmask;
  String static_gateway;
  String static_dns;
#if MQTT_SUPPORTED == 1
  MqttConfig mqtt;
#endif
  String syslog_ip;
#if ENABLE_BATTERY_STANDBY == 1
  String sleep_battery_threshold;
  String wake_battery_threshold;
#endif
  bool force_ap;
} Config;

#define CONFIG_PORTAL_MAX_TIME_SECONDS 300

// -------------------------------------------------------
// Set the red led in case of error
// -------------------------------------------------------
void updateRedLed() {
  uint8_t state = 0;
  if (!readoutSucceeded) {
    state = 1;
  }
  if (Inverter.GetWiFiStickType() == Undef_stick) {
    state = 1;
  }
#if MQTT_SUPPORTED == 1
  if (shineMqtt.mqttEnabled() && !shineMqtt.mqttConnected()) {
    state = 1;
  }
#endif
  digitalWrite(LED_RT, state);
}

// -------------------------------------------------------
// Check the WiFi status and reconnect if necessary
// -------------------------------------------------------
void WiFi_Reconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_GN, 0);

    while (WiFi.status() != WL_CONNECTED) {
      delay(200);
      Log.print(F("."));
      digitalWrite(LED_RT,
                   !digitalRead(LED_RT));  // toggle red led on WiFi (re)connect
    }

    // todo: use Log
    WiFi.printDiag(Serial);
    Log.print(F("Local IP: "));
    Log.println(WiFi.localIP());
    Log.print(F("Hostname: "));
    Log.println(Config.hostname);

    Log.println(F("WiFi reconnected"));

    updateRedLed();
  }
}

// Connection can fail after sunrise. The stick powers up before the inverter.
// So the detection of the inverter will fail. If no inverter is detected, we
// have to retry later (s. loop() ) The detection without running inverter will
// take several seconds, because the ModBus-Lib has a timeout of 2s for each
// read access (and we do several of them). The WiFi can crash during this
// function. Perhaps we can fix this by using the callback function of the
// ModBus-Lib
void InverterReconnect(void) {
  // Baudrate will be set here, depending on the version of the stick
  Inverter.begin(Serial);

  if (Inverter.GetWiFiStickType() == ShineWiFi_S)
    Log.println(F("ShineWiFi-S (Serial) found"));
  else if (Inverter.GetWiFiStickType() == ShineWiFi_X)
    Log.println(F("ShineWiFi-X (USB) found"));
  else
    Log.println(F("Error: Unknown Shine Stick"));
}

void loadConfig();
void saveConfig();
void saveParamCallback();
void setupWifiManagerConfigMenu(WiFiManager& wm);

void loadConfig() {
  Config.hostname = prefs.getString(ConfigFiles.hostname, DEFAULT_HOSTNAME);
  Config.static_ip = prefs.getString(ConfigFiles.static_ip, "");
  Config.static_netmask = prefs.getString(ConfigFiles.static_netmask, "");
  Config.static_gateway = prefs.getString(ConfigFiles.static_gateway, "");
  Config.static_dns = prefs.getString(ConfigFiles.static_dns, "");
#if MQTT_SUPPORTED == 1
  Config.mqtt.server = prefs.getString(ConfigFiles.mqtt_server, "");
  Config.mqtt.port = prefs.getString(ConfigFiles.mqtt_port, "1883");
  Config.mqtt.topic = prefs.getString(ConfigFiles.mqtt_topic, "");
  Config.mqtt.user = prefs.getString(ConfigFiles.mqtt_user, "");
  Config.mqtt.pwd = prefs.getString(ConfigFiles.mqtt_pwd, "");
#endif
  Config.syslog_ip = prefs.getString(ConfigFiles.syslog_ip, "");
#if ENABLE_BATTERY_STANDBY == 1
  Config.sleep_battery_threshold =
      prefs.getString(ConfigFiles.sleep_battery_threshold, "10");
  Config.wake_battery_threshold =
      prefs.getString(ConfigFiles.wake_battery_threshold, "75");
#endif
  Config.force_ap = prefs.getBool(ConfigFiles.force_ap, false);
}

void saveConfig() {
  prefs.putString(ConfigFiles.hostname, Config.hostname);
  prefs.putString(ConfigFiles.static_ip, Config.static_ip);
  prefs.putString(ConfigFiles.static_netmask, Config.static_netmask);
  prefs.putString(ConfigFiles.static_gateway, Config.static_gateway);
  prefs.putString(ConfigFiles.static_dns, Config.static_dns);
#if MQTT_SUPPORTED == 1
  prefs.putString(ConfigFiles.mqtt_server, Config.mqtt.server);
  prefs.putString(ConfigFiles.mqtt_port, Config.mqtt.port);
  prefs.putString(ConfigFiles.mqtt_topic, Config.mqtt.topic);
  prefs.putString(ConfigFiles.mqtt_user, Config.mqtt.user);
  prefs.putString(ConfigFiles.mqtt_pwd, Config.mqtt.pwd);
#endif
  prefs.putString(ConfigFiles.syslog_ip, Config.syslog_ip);
#if ENABLE_BATTERY_STANDBY == 1
  prefs.putString(ConfigFiles.sleep_battery_threshold,
                  Config.sleep_battery_threshold);
  prefs.putString(ConfigFiles.wake_battery_threshold,
                  Config.wake_battery_threshold);
#endif
}

void saveParamCallback() {
  Log.println(F("[CALLBACK] saveParamCallback fired"));

  Config.hostname = customWMParams.hostname->getValue();
  if (Config.hostname.isEmpty()) {
    Config.hostname = DEFAULT_HOSTNAME;
  }
  Config.static_ip = customWMParams.static_ip->getValue();
  Config.static_netmask = customWMParams.static_netmask->getValue();
  Config.static_gateway = customWMParams.static_gateway->getValue();
  Config.static_dns = customWMParams.static_dns->getValue();
#if MQTT_SUPPORTED == 1
  Config.mqtt.server = customWMParams.mqtt_server->getValue();
  Config.mqtt.port = customWMParams.mqtt_port->getValue();
  Config.mqtt.topic = customWMParams.mqtt_topic->getValue();
  Config.mqtt.user = customWMParams.mqtt_user->getValue();
  Config.mqtt.pwd = customWMParams.mqtt_pwd->getValue();
#endif
  Config.syslog_ip = customWMParams.syslog_ip->getValue();
#if ENABLE_BATTERY_STANDBY == 1
  Config.sleep_battery_threshold =
      customWMParams.sleep_battery_threshold->getValue();
  Config.wake_battery_threshold =
      customWMParams.wake_battery_threshold->getValue();
#endif

  saveConfig();

  Log.println(F("[CALLBACK] saveParamCallback complete"));
}

#ifdef ENABLE_TELNET_DEBUG
#include <TelnetSerialStream.h>
TelnetSerialStream telnetSerialStream = TelnetSerialStream();
#endif

#ifdef ENABLE_WEB_DEBUG
#include <WebSerialStream.h>
WebSerialStream webSerialStream = WebSerialStream(8080);
#endif

#include <SyslogStream.h>
SyslogStream syslogStream = SyslogStream();

void configureLogging() {
#ifdef ENABLE_SERIAL_DEBUG
  Serial.begin(115200);
  Log.disableSerial(false);
#else
  Log.disableSerial(true);
#endif
#ifdef ENABLE_TELNET_DEBUG
  Log.addPrintStream(std::make_shared<TelnetSerialStream>(telnetSerialStream));
#endif
#ifdef ENABLE_WEB_DEBUG
  Log.addPrintStream(std::make_shared<WebSerialStream>(webSerialStream));
#endif
  if (!Config.syslog_ip.isEmpty()) {
    syslogStream.setDestination(Config.syslog_ip.c_str());
    // syslogStream.setRaw(true);
    const std::shared_ptr<LOGBase> syslogStreamPtr =
        std::make_shared<SyslogStream>(syslogStream);
    Log.addPrintStream(syslogStreamPtr);
    Log.print(F("Syslog Server IP: "));
    Log.println(Config.syslog_ip);
  }
}

void setupGPIO() {
  pinMode(LED_GN, OUTPUT);
  pinMode(LED_RT, OUTPUT);
  pinMode(LED_BL, OUTPUT);
}

void setupWifiHost() {
  WiFi.hostname(Config.hostname);
  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
#if OTA_SUPPORTED == 0
  MDNS.begin(Config.hostname);
#endif
  Log.print(F("SetupWiFiHost: hostname "));
  Log.println(Config.hostname);
}

void startWdt() {
#ifdef ESP32
  Log.println(F("Configuring WDT..."));
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
#endif
}

void handleWdtReset(boolean mqttSuccess) {
#if MQTT_SUPPORTED == 1
  if (mqttSuccess) {
    resetWdt();
  } else {
    if (!shineMqtt.mqttEnabled()) {
      resetWdt();
    }
  }
#else
  resetWdt();
#endif
}

void resetWdt() {
#ifdef ESP32
  Log.println(F("WDT reset..."));
  esp_task_wdt_reset();
#endif
}

void setup() {
  WiFiManager wm;

  Log.println(F("Setup()"));

  setupGPIO();

#if ENABLE_DOUBLE_RESET == 1
  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
#endif

  prefs.begin("ShineWiFi");

  loadConfig();
  configureLogging();
  setupWifiHost();

#if OTA_SUPPORTED == 1
#if !defined(OTA_PASSWORD)
#error "Please define an OTA_PASSWORD in Config.h"
#endif
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setHostname(Config.hostname.c_str());
  ArduinoOTA.begin();
#endif

  Log.begin();
  startWdt();

  setupWifiManagerConfigMenu(wm);

  digitalWrite(LED_BL, 1);
  digitalWrite(LED_RT, 0);
  digitalWrite(LED_GN, 0);

  // Set a timeout so the ESP doesn't hang waiting to be configured, for
  // instance after a power failure

  wm.setConfigPortalTimeout(CONFIG_PORTAL_MAX_TIME_SECONDS);

  Log.print(F("force_ap: "));
  Log.println(Config.force_ap);

#ifdef AP_BUTTON_PRESSED
  if (AP_BUTTON_PRESSED) {
    Log.println(F("AP Button pressed during power up"));
    Config.force_ap = true;
  }
#endif
#if ENABLE_DOUBLE_RESET == 1
  if (drd->detectDoubleReset()) {
    Log.println(F("Double reset detected"));
    Config.force_ap = true;
  }
#endif
  if (Config.force_ap) {
    prefs.putBool(ConfigFiles.force_ap, false);
    wm.startConfigPortal("GrowattConfig", APPassword);
    Log.println(F("GrowattConfig finished"));
    digitalWrite(LED_BL, 0);
    delay(3000);
    ESP.restart();
  }

  // Set static ip
  if (!Config.static_ip.isEmpty() && !Config.static_netmask.isEmpty()) {
    IPAddress ip, netmask, gateway, dns;
    ip.fromString(Config.static_ip);
    netmask.fromString(Config.static_netmask);
    gateway.fromString(Config.static_gateway);
    dns.fromString(Config.static_dns);
    Log.print(F("static ip: "));
    Log.println(Config.static_ip);
    Log.print(F("static netmask: "));
    Log.println(Config.static_netmask);
    Log.print(F("static gateway: "));
    Log.println(Config.static_gateway);
    Log.print(F("static dns: "));
    Log.println(Config.static_dns);
    if (!Config.static_dns.isEmpty()) {
      wm.setSTAStaticIPConfig(ip, gateway, netmask, dns);
    } else {
      wm.setSTAStaticIPConfig(ip, gateway, netmask);
    }
  }

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name
  // ("GrowattConfig")
  int connect_timeout_seconds = 15;
  wm.setConnectTimeout(connect_timeout_seconds);
  bool res = wm.autoConnect("GrowattConfig",
                            APPassword);  // password protected wificonfig ap

  if (!res) {
    Log.println(F("Failed to connect WiFi!"));
    ESP.restart();
  } else {
    digitalWrite(LED_BL, 0);
    // if you get here you have connected to the WiFi
    Log.println(F("WiFi connected"));
  }

  while (WiFi.status() != WL_CONNECTED) {
    WiFi_Reconnect();
  }

#if MQTT_SUPPORTED == 1
#ifdef MQTTS_ENABLED
  espClient.setCACert(MQTTS_BROKER_CA_CERT);
#endif
  shineMqtt.mqttSetup(Config.mqtt);
#endif

  httpServer.on("/status", sendJsonSite);
  httpServer.on("/uiStatus", sendUiJsonSite);
  httpServer.on("/metrics", sendMetrics);
  httpServer.on("/startAp", startConfigAccessPoint);
  httpServer.on("/reboot", rebootESP);
  httpServer.on("/loadfirst", loadFirst);
  httpServer.on("/batteryfirst", batteryFirst);
  httpServer.on("/gridfirst", gridFirst);
#if ENABLE_MODBUS_COMMUNICATION == 1
  httpServer.on("/postCommunicationModbus", sendPostSite);
  httpServer.on("/postCommunicationModbus_p", HTTP_POST, handlePostData);
#endif
  httpServer.on("/", sendMainPage);
#ifdef ENABLE_WEB_DEBUG
  httpServer.on("/debug", sendDebug);
#endif
  httpServer.onNotFound(handleNotFound);

  Inverter.InitProtocol();
  InverterReconnect();
  httpServer.begin();

#if defined(DEFAULT_NTP_SERVER) && defined(DEFAULT_TZ_INFO)
#ifdef ESP32
  configTime(0, 0, DEFAULT_NTP_SERVER);
  setenv("TZ", DEFAULT_TZ_INFO, 1);
#else
  configTime(DEFAULT_TZ_INFO, DEFAULT_NTP_SERVER);
#endif
#endif
}

void setupWifiManagerConfigMenu(WiFiManager& wm) {
  customWMParams.hostname = new WiFiManagerParameter(
      "hostname", "hostname (no spaces or special chars)",
      Config.hostname.c_str(), 30);
  customWMParams.static_ip =
      new WiFiManagerParameter("staticip", "ip", Config.static_ip.c_str(), 15);
  customWMParams.static_netmask = new WiFiManagerParameter(
      "staticnetmask", "netmask", Config.static_netmask.c_str(), 15);
  customWMParams.static_gateway = new WiFiManagerParameter(
      "staticgateway", "gateway", Config.static_gateway.c_str(), 15);
  customWMParams.static_dns = new WiFiManagerParameter(
      "staticdns", "dns", Config.static_dns.c_str(), 15);
#if MQTT_SUPPORTED == 1
  customWMParams.mqtt_server = new WiFiManagerParameter(
      "mqttserver", "server", Config.mqtt.server.c_str(), 40);
  customWMParams.mqtt_port =
      new WiFiManagerParameter("mqttport", "port", Config.mqtt.port.c_str(), 6);
  customWMParams.mqtt_topic = new WiFiManagerParameter(
      "mqtttopic", "topic", Config.mqtt.topic.c_str(), 64);
  customWMParams.mqtt_user = new WiFiManagerParameter(
      "mqttusername", "username", Config.mqtt.user.c_str(), 40);
  customWMParams.mqtt_pwd = new WiFiManagerParameter(
      "mqttpassword", "password", Config.mqtt.pwd.c_str(), 64);
#endif
  customWMParams.syslog_ip = new WiFiManagerParameter(
      "syslogip", "Syslog Server IP (leave blank for none)",
      Config.syslog_ip.c_str(), 15);
#if ENABLE_BATTERY_STANDBY == 1
  customWMParams.sleep_battery_threshold = new WiFiManagerParameter(
      "sleepbatterythreshold", "sleep battery threshold",
      Config.sleep_battery_threshold.c_str(), 4);
  customWMParams.wake_battery_threshold =
      new WiFiManagerParameter("wakebatterythreshold", "wake battery threshold",
                               Config.wake_battery_threshold.c_str(), 4);
#endif
  wm.addParameter(customWMParams.hostname);
#if MQTT_SUPPORTED == 1
  wm.addParameter(new WiFiManagerParameter(
      "<p><b>MQTT Settings</b> (leave server blank to disable)</p>"));
  wm.addParameter(customWMParams.mqtt_server);
  wm.addParameter(customWMParams.mqtt_port);
  wm.addParameter(customWMParams.mqtt_topic);
  wm.addParameter(customWMParams.mqtt_user);
  wm.addParameter(customWMParams.mqtt_pwd);
#endif
  wm.addParameter(new WiFiManagerParameter(
      "<p><b>Static IP</b> (leave blank for DHCP)</p>"));
  wm.addParameter(customWMParams.static_ip);
  wm.addParameter(customWMParams.static_netmask);
  wm.addParameter(customWMParams.static_gateway);
  wm.addParameter(customWMParams.static_dns);
  wm.addParameter(new WiFiManagerParameter("<p><b>Advanced Settings</b></p>"));
  wm.addParameter(customWMParams.syslog_ip);
  wm.addParameter(customWMParams.sleep_battery_threshold);
  wm.addParameter(customWMParams.wake_battery_threshold);

  wm.setSaveParamsCallback(saveParamCallback);

  setupMenu(wm, true);
}

/**
 * @brief create custom wifimanager menu entries
 *
 * @param enableCustomParams enable custom params aka. mqtt settings
 */
void setupMenu(WiFiManager& wm, bool enableCustomParams) {
  Log.println(F("Setting up WiFiManager menu"));
  std::vector<const char*> menu = {"wifi", "wifinoscan", "update"};
  if (enableCustomParams) {
    menu.push_back("param");
  }
  menu.push_back("sep");
  menu.push_back("erase");
  menu.push_back("restart");

  wm.setMenu(menu);  // custom menu, pass vector
}

void sendJson(JsonDocument& doc) {
  httpServer.setContentLength(measureJson(doc));
  httpServer.send(200, "application/json", "");
  WiFiClient client = httpServer.client();
  WriteBufferingStream bufferedWifiClient{client, BUFFER_SIZE};
  serializeJson(doc, bufferedWifiClient);
}

void sendJsonSite(void) {
  if (!readoutSucceeded) {
    httpServer.send(503, F("text/plain"), F("Service Unavailable"));
    return;
  }

  DynamicJsonDocument doc(JSON_DOCUMENT_SIZE);
  Inverter.CreateJson(doc, WiFi.macAddress(), Config.hostname);

  sendJson(doc);
}

void sendUiJsonSite(void) {
  DynamicJsonDocument doc(JSON_DOCUMENT_SIZE);
  Inverter.CreateUIJson(doc, Config.hostname);

  sendJson(doc);
}

void sendMetrics(void) {
  if (!readoutSucceeded) {
    httpServer.send(503, F("text/plain"), F("Service Unavailable"));
    return;
  }
  static unsigned maxMetricsSize = 0;
  String metrics;
  if (maxMetricsSize) {
    metrics.reserve(maxMetricsSize);
  }

  Inverter.CreateMetrics(metrics, WiFi.macAddress(), Config.hostname);

  httpServer.setContentLength(metrics.length());
  httpServer.send(200, "text/plain", "");
  WiFiClient client = httpServer.client();
  for (uint16_t i = 0; i < metrics.length(); i += TCP_MSS) {
    int len = min(TCP_MSS, (int)metrics.length() - i);
    client.write(metrics.c_str() + i, len);
  }
  maxMetricsSize = max(maxMetricsSize, metrics.length());
}

#if MQTT_SUPPORTED == 1
boolean sendMqttJson(void) {
  DynamicJsonDocument doc(JSON_DOCUMENT_SIZE);

  Inverter.CreateJson(doc, WiFi.macAddress(), "");
  return shineMqtt.mqttPublish(doc);
}
#endif

void startConfigAccessPoint(void) {
  char msg[384];

  snprintf_P(msg, sizeof(msg),
             PSTR("<html><body>Configuration Access Point started...<br /><br "
                  "/>Connect to WiFi \"GrowattConfig\" with your password "
                  "(default: \"growsolar\") and visit <a "
                  "href='http://192.168.4.1'>192.168.4.1</a><br /><br />The "
                  "stick will automatically return to normal operation after "
                  "%d seconds</body></html>"),
             CONFIG_PORTAL_MAX_TIME_SECONDS);
  httpServer.send(200, "text/html", msg);
  delay(2000);
  StartedConfigAfterBoot = true;
}

void rebootESP(void) {
  httpServer.send(200, F("text/html"),
                  F("<html><body>Rebooting...</body></html>"));
  delay(2000);
  ESP.restart();
}

void loadFirst(void) {
  httpServer.send(200, "text/plain", "Load First");
  StaticJsonDocument<128> req, res;
  const char* payload = "{\"mode\": 0}";
  Inverter.HandleCommand("priority/set", (const byte*)payload, strlen(payload),
                         req, res);
}

void batteryFirst(void) {
  httpServer.send(200, "text/plain", "Battery First");
  StaticJsonDocument<128> req, res;
  const char* payload = "{\"mode\": 1}";
  Inverter.HandleCommand("priority/set", (const byte*)payload, strlen(payload),
                         req, res);
}

void gridFirst(void) {
  httpServer.send(200, "text/plain", "Grid First");
  StaticJsonDocument<128> req, res;
  const char* payload = "{\"mode\": 2}";
  Inverter.HandleCommand("priority/set", (const byte*)payload, strlen(payload),
                         req, res);
}

#ifdef ENABLE_WEB_DEBUG
void sendDebug(void) {
  httpServer.sendHeader("Location",
                        "http://" + WiFi.localIP().toString() + ":8080/", true);
  httpServer.send(302, "text/plain", "");
}
#endif

void sendMainPage(void) { httpServer.send(200, "text/html", MAIN_page); }

#if ENABLE_MODBUS_COMMUNICATION == 1
void sendPostSite(void) {
  httpServer.send(200, "text/html", SendPostSite_page);
}
#endif

void handlePostData() {
  char msg[256];
  uint16_t u16Tmp;
  uint32_t u32Tmp;

  if (!httpServer.hasArg(F("reg")) || !httpServer.hasArg(F("val"))) {
    // If the POST request doesn't have data
    httpServer.send(400, F("text/plain"),
                    F("400: Invalid Request"));  // The request is invalid, so
                                                 // send HTTP status 400
    return;
  } else {
    if (httpServer.arg(F("operation")) == "R") {
      if (httpServer.arg(F("registerType")) == "I") {
        if (httpServer.arg(F("type")) == "16b") {
          if (Inverter.ReadInputReg(httpServer.arg(F("reg")).toInt(),
                                    &u16Tmp)) {
            snprintf_P(msg, sizeof(msg),
                       PSTR("Reading Value %d from 16-bit Input Register %ld "
                            "succeeded"),
                       u16Tmp, httpServer.arg("reg").toInt());
          } else {
            snprintf_P(msg, sizeof(msg),
                       PSTR("Reading from 16-bit Input Register %ld failed!"),
                       httpServer.arg("reg").toInt());
          }
        } else {
          if (Inverter.ReadInputReg(httpServer.arg(F("reg")).toInt(),
                                    &u32Tmp)) {
            snprintf_P(msg, sizeof(msg),
                       PSTR("Reading Value %d from 32-bit Input Register %ld "
                            "succeeded"),
                       u32Tmp, httpServer.arg("reg").toInt());
          } else {
            snprintf_P(msg, sizeof(msg),
                       PSTR("Reading from 32-bit Input Register %ld failed!"),
                       httpServer.arg("reg").toInt());
          }
        }
      } else {
        if (httpServer.arg(F("type")) == "16b") {
          if (Inverter.ReadHoldingReg(httpServer.arg(F("reg")).toInt(),
                                      &u16Tmp)) {
            snprintf_P(msg, sizeof(msg),
                       PSTR("Reading Value %d from 16-bit Holding Register %ld "
                            "succeeded"),
                       u16Tmp, httpServer.arg("reg").toInt());
          } else {
            snprintf_P(msg, sizeof(msg),
                       PSTR("Reading from 16-bit Holding Register %ld failed!"),
                       httpServer.arg("reg").toInt());
          }
        } else {
          if (Inverter.ReadHoldingReg(httpServer.arg(F("reg")).toInt(),
                                      &u32Tmp)) {
            snprintf_P(msg, sizeof(msg),
                       PSTR("Reading Value %d from 32-bit Holding Register %ld "
                            "succeeded"),
                       u32Tmp, httpServer.arg("reg").toInt());
          } else {
            snprintf_P(msg, sizeof(msg),
                       PSTR("Reading from 32-bit Holding Register %ld failed!"),
                       httpServer.arg("reg").toInt());
          }
        }
      }
    } else {
      if (httpServer.arg(F("registerType")) == "H") {
        if (httpServer.arg(F("type")) == "16b") {
          if (Inverter.WriteHoldingReg(httpServer.arg(F("reg")).toInt(),
                                       httpServer.arg(F("val")).toInt())) {
            snprintf_P(
                msg, sizeof(msg),
                PSTR("Writing Value %ld to Holding Register %ld succeeded"),
                httpServer.arg("val").toInt(), httpServer.arg("reg").toInt());
          } else {
            snprintf_P(
                msg, sizeof(msg),
                PSTR("Writing Value %ld to Holding Register %ld failed!"),
                httpServer.arg("val").toInt(), httpServer.arg("reg").toInt());
          }
        } else {
          snprintf_P(
              msg, sizeof(msg),
              PSTR("Writing to double (32-bit) Registers not supported!"));
        }
      } else {
        snprintf_P(msg, sizeof(msg),
                   PSTR("Writing to Input Registers not possible!"));
      }
    }
    httpServer.send(200, F("text/plain"), msg);
    return;
  }
}

bool sendSingleValue(void) {
  if (!readoutSucceeded) {
    httpServer.send(503, F("text/plain"), F("Service Unavailable"));
    return true;
  }
  const String& key = httpServer.uri().substring(7);
  double value;
  if (Inverter.GetSingleValueByName(key, value)) {
    httpServer.send(200, "text/plain", String(value));
    return true;
  }
  return false;
}

void handleNotFound() {
  if (httpServer.uri().startsWith(F("/value/")) &&
      httpServer.uri().length() > 7) {
    if (sendSingleValue()) {
      return;
    }
  }
  httpServer.send(404, F("text/plain"),
                  String("Not found: " + httpServer.uri()));
}

#if defined(DEFAULT_NTP_SERVER) && defined(DEFAULT_TZ_INFO)
void handleNTPSync() {
  int reachable = sntp_getreachability(0);
  Log.print(F("NTP server: "));
  Log.print(DEFAULT_NTP_SERVER);
  Log.print(F(" reachable "));
  Log.println(reachable & 1);

  if (reachable & 1) {
    StaticJsonDocument<128> req, res;
    char buff[32];
    struct tm tm;
    time_t t = time(NULL);
    localtime_r(&t, &tm);
    strftime(buff, sizeof(buff), "{\"value\":\"%Y-%m-%d %T\"}", &tm);
    Inverter.HandleCommand("datetime/set", (byte*)&buff, strlen(buff), req,
                           res);
  }
}
#endif

// battery standby
#if ENABLE_BATTERY_STANDBY == 1
const uint32_t wake_threshold =
    std::strtoul(Config.wake_battery_threshold.c_str(), nullptr, 10);
const uint32_t sleep_threshold =
    std::strtoul(Config.sleep_battery_threshold.c_str(), nullptr, 10);
void batteryStandby() {
  if (Inverter._Protocol.InputRegisters[P3000_BDC_SYSSTATE].value == 0)
  {
    if (Inverter._Protocol.InputRegisters[P3000_PTOGRID_TOTAL].value >
        wake_threshold * 10) {
      if (Inverter.WriteHoldingReg(0, 3)) {
        Log.println(F("Battery activated"));
      } else {
        Log.println(F("Battery still deactivated!"));
      }
    }
  } else if (Inverter._Protocol.InputRegisters[P3000_BDC_SYSSTATE].value == 1) {
    if ((Inverter._Protocol.InputRegisters[P3000_BDC_SOC].value <=
         Inverter._Protocol.HoldingRegisters[P3000_BDC_DISCHARGE_STOPSOC]
             .value) &&
        ((Inverter._Protocol.InputRegisters[P3000_PPV].value) <
         sleep_threshold * 10)) {
      if (Inverter.WriteHoldingReg(0, 2)) {
        Log.println(F("Battery deactivated"));
      } else {
        Log.println(F("Battery still activated!"));
      }
    }
  }
}
#endif

// ac charge power rate
#if ACCHARGE_POWERRATE == 1
void acchargePowerrate() {
  if (Inverter._Protocol.InputRegisters[P3000_BDC_SOC].value == 100) {
    return;
  }
  if (Inverter._Protocol.InputRegisters[P3000_PRIORITY].value == 1 &&
      Inverter._Protocol.HoldingRegisters[P3000_BDC_CHARGE_AC_ENABLED].value ==
          1) {
    int64_t delta =
        (static_cast<int64_t>(
             Inverter._Protocol.InputRegisters[P3000_BDC_PCHR].value) +
         static_cast<int64_t>(
             Inverter._Protocol.InputRegisters[P3000_PTOGRID_TOTAL].value) -
         static_cast<int64_t>(
             Inverter._Protocol.InputRegisters[P3000_PTOUSER_TOTAL].value));
    int64_t rawRate = (delta * 10) / ACCHARGE_MAXPOWER - ACCHARGE_OFFSET;
    uint32_t targetpowerrate =
        static_cast<uint32_t>(std::clamp<int64_t>(rawRate, 0, 100));
    if (Inverter._Protocol.HoldingRegisters[P3000_BDC_CHARGE_P_RATE].value !=
        targetpowerrate) {
      StaticJsonDocument<128> req, res;
      char payload[16];
      snprintf(payload, sizeof(payload), "{\"value\": %u}", targetpowerrate);
      Inverter.HandleCommand("bdc/set/chargepowerrate", (const byte*)payload,
                             strlen(payload), req, res);
    }
  }
}
#endif

// -------------------------------------------------------
// Main loop
// -------------------------------------------------------
#if ENABLE_AP_BUTTON == 1
unsigned long ButtonTimer = 0;
#endif
unsigned long LEDTimer = 0;
unsigned long RefreshTimer = 0;
unsigned long WifiRetryTimer = 0;
#if ENABLE_BATTERY_STANDBY == 1
unsigned long BatteryStandbyTimer = 0;  // battery standby
#endif
#if ACCHARGE_POWERRATE == 1
unsigned long ACChargeTimer = 0;  // ac charge power rate
#endif
#if defined(DEFAULT_NTP_SERVER) && defined(DEFAULT_TZ_INFO)
unsigned long NTPTimer = 0;
unsigned long lastSync = 0;
bool initialSyncDone = false;
#endif

void loop() {
#if ENABLE_DOUBLE_RESET
  drd->loop();
#endif

  Log.loop();
  unsigned long now = millis();

#ifdef AP_BUTTON_PRESSED
  if ((now - ButtonTimer) > BUTTON_TIMER) {
    ButtonTimer = now;

    if (AP_BUTTON_PRESSED) {
      if (btnPressed > 5) {
        Log.println(F("Handle press"));
        StartedConfigAfterBoot = true;
      } else {
        btnPressed++;
      }
      Log.print(F("Button pressed"));
    } else {
      btnPressed = 0;
    }
  }
#endif

  if (StartedConfigAfterBoot == true) {
    Log.println(F("StartedConfigAfterBoot"));
    prefs.putBool(ConfigFiles.force_ap, true);
    delay(3000);
    ESP.restart();
  }

  WiFi_Reconnect();

#if MQTT_SUPPORTED == 1
  if (shineMqtt.mqttReconnect()) {
    shineMqtt.loop();
  }
#endif

  httpServer.handleClient();

  // Toggle green LED with 1 Hz (alive)
  // ------------------------------------------------------------
  if ((now - LEDTimer) > LED_TIMER) {
    if (WiFi.status() == WL_CONNECTED)
      digitalWrite(LED_GN, !digitalRead(LED_GN));
    else
      digitalWrite(LED_GN, 0);

    LEDTimer = now;
  }

  // InverterReconnect() takes a long time --> wifi will crash
  // Do it only every two minutes
  if ((now - WifiRetryTimer) > WIFI_RETRY_TIMER) {
    if (Inverter.GetWiFiStickType() == Undef_stick) InverterReconnect();
    WifiRetryTimer = now;
  }

  // Read Inverter every REFRESH_TIMER ms [defined in config.h]
  // ------------------------------------------------------------
  if ((now - RefreshTimer) > REFRESH_TIMER) {
    if ((WiFi.status() == WL_CONNECTED) && (Inverter.GetWiFiStickType())) {
#if SIMULATE_INVERTER == 1
      readoutSucceeded = true;
#else
      readoutSucceeded =
          Inverter.ReadData(NUM_OF_RETRIES);  // get new data from inverter
#endif
      if (readoutSucceeded) {
        boolean mqttSuccess = false;
#if MQTT_SUPPORTED == 1
        if (shineMqtt.mqttEnabled()) {
          mqttSuccess = sendMqttJson();
        }
#endif
        handleWdtReset(mqttSuccess);
      } else {
#if MQTT_SUPPORTED == 1
        shineMqtt.mqttPublish(String(F("{\"InverterStatus\": -1 }")));
#endif
      }
    }

    updateRedLed();

#if PINGER_SUPPORTED == 1
    // frequently check if gateway is reachable
    if (pinger.Ping(GATEWAY_IP) == false) {
      digitalWrite(LED_RT, 1);
      delay(3000);
      ESP.restart();
    }
#endif

    RefreshTimer = now;
  }

#if defined(DEFAULT_NTP_SERVER) && defined(DEFAULT_TZ_INFO)
  // set inverter datetime, initially after 60 seconds and then after 1 hour
  if (!initialSyncDone && now >= 60000) {
    handleNTPSync();
    lastSync = now;
    initialSyncDone = true;
  }

  if (initialSyncDone && (now - lastSync) >= NTP_TIMER) {
    handleNTPSync();
    lastSync = now;
  }
#endif

#if OTA_SUPPORTED == 1
  // check for OTA updates
  ArduinoOTA.handle();
#else
#ifndef ESP32
  // Handle MDNS requests on ESP8266
  MDNS.update();
#endif
#endif

#if ENABLE_BATTERY_STANDBY == 1
  if ((now - BatteryStandbyTimer) > BATTERY_STANDBY_TIMER) {
    batteryStandby();
    BatteryStandbyTimer = now;
  }
#endif

#if ACCHARGE_POWERRATE == 1
  if ((now - ACChargeTimer) > ACCHARGE_TIMER) {
    acchargePowerrate();
    ACChargeTimer = now;
  }
#endif
}
