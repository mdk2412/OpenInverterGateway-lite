// -----------------------------------------------------------------------------
//  Projekt-Konfiguration
// -----------------------------------------------------------------------------
#include "Config.h"
#ifndef _SHINE_CONFIG_H_
#error Please rename Config.h.example to Config.h
#endif

// -----------------------------------------------------------------------------
//  Projekt-Header
// -----------------------------------------------------------------------------
#include "ShineWifi.h"
#include "Index.h"
#include "Growatt.h"

#if MQTT_SUPPORTED == 1
#include "ShineMqtt.h"
#endif

#if MODBUS_TCP_SUPPORTED == 1
#include "ModbusTCP.h"
#endif

#include "ACChargeControl.h"
#include "BatteryStandby.h"
#include "SetLED.h"
#include "PriorityControl.h"
#include "SurplusCharge.h"

// -----------------------------------------------------------------------------
//  Externe Bibliotheken
// -----------------------------------------------------------------------------
#include <LittleFS.h>
#include <Preferences.h>
#include <StreamUtils.h>
#include <TLog.h>
#include <WiFiManager.h>

// -----------------------------------------------------------------------------
//  Plattformabhängige Includes
// -----------------------------------------------------------------------------
#include <Updater.h>

// -----------------------------------------------------------------------------
//  Optional: OTA
// -----------------------------------------------------------------------------
#if OTA_SUPPORTED == 1
#include <ArduinoOTA.h>
#endif

// -----------------------------------------------------------------------------
//  Optional: Pinger
// -----------------------------------------------------------------------------
#if PINGER_SUPPORTED == 1
#include <Pinger.h>
#include <PingerResponse.h>
#endif

// -----------------------------------------------------------------------------
//  Optional: Double Reset Detector
// -----------------------------------------------------------------------------
#if ENABLE_DOUBLE_RESET == 1
#define ESP_DRD_USE_LITTLEFS true
#define ESP_DRD_USE_EEPROM false
#define DRD_TIMEOUT 10
#define DRD_ADDRESS 0
#include <ESP_DoubleResetDetector.h>
DoubleResetDetector* drd;
#endif

// -----------------------------------------------------------------------------
//  Optional: NTP
// -----------------------------------------------------------------------------
#if defined(DEFAULT_NTP_SERVER) && defined(DEFAULT_TZ_INFO)
#include <time.h>
extern "C" uint8_t sntp_getreachability(uint8_t);
#endif

// -----------------------------------------------------------------------------
//  Globale Objekte
// -----------------------------------------------------------------------------
Preferences prefs;
Growatt Inverter;
bool StartedConfigAfterBoot = false;

// NEU:
#if MQTT_SUPPORTED == 1
ShineMqtt shineMqtt(Inverter);
#endif

#if MODBUS_TCP_SUPPORTED == 1
ModbusTCP modbusTCP(MODBUS_TCP_PORT);
#endif

#if defined(AP_BUTTON_PRESSED)
byte btnPressed = 0;
#endif

boolean readoutSucceeded = false;

#if PINGER_SUPPORTED == 1
Pinger pinger;
#endif

ESP8266WebServer httpServer(80);

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
} ConfigFiles;

struct WifiConfig {
  String hostname;
  String static_ip;
  String static_netmask;
  String static_gateway;
  String static_dns;

#if MQTT_SUPPORTED == 1
  MqttConfig mqtt;
#endif

  String syslog_ip;
  bool force_ap;
};

WifiConfig Wifi;

UserConfig User;

#define CONFIG_PORTAL_MAX_TIME_SECONDS 300

// -------------------------------------------------------
// Check the WiFi status and reconnect if necessary
// -------------------------------------------------------
void WiFi_Reconnect() {
  static bool wasConnecting = false;

  if (WiFi.status() != WL_CONNECTED) {
    wasConnecting = true;

    // echter reconnect
    WiFi.reconnect();

    Log.print(F("."));
    return;
  }

if (wasConnecting) {
    wasConnecting = false;

    WiFi.printDiag(Serial);
    Log.printf("WiFi reconnected | Local IP: %s | Hostname: %s\n",
               WiFi.localIP().toString().c_str(),
               WiFi.hostname().c_str());
  }
}

void loadConfig();
void saveConfig();
void saveParamCallback();
void setupWifiManagerConfigMenu(WiFiManager& wm);

void loadConfig() {
  Wifi.hostname = prefs.getString(ConfigFiles.hostname, DEFAULT_HOSTNAME);
  Wifi.static_ip = prefs.getString(ConfigFiles.static_ip, "");
  Wifi.static_netmask = prefs.getString(ConfigFiles.static_netmask, "");
  Wifi.static_gateway = prefs.getString(ConfigFiles.static_gateway, "");
  Wifi.static_dns = prefs.getString(ConfigFiles.static_dns, "");

#if MQTT_SUPPORTED == 1
  Wifi.mqtt.server = prefs.getString(ConfigFiles.mqtt_server, "");
  Wifi.mqtt.port = prefs.getString(ConfigFiles.mqtt_port, "1883");
  Wifi.mqtt.topic = prefs.getString(ConfigFiles.mqtt_topic, "");
  Wifi.mqtt.user = prefs.getString(ConfigFiles.mqtt_user, "");
  Wifi.mqtt.pwd = prefs.getString(ConfigFiles.mqtt_pwd, "");
#endif

  Wifi.syslog_ip = prefs.getString(ConfigFiles.syslog_ip, "");
  Wifi.force_ap = prefs.getBool(ConfigFiles.force_ap, false);
}

void saveConfig() {
  prefs.putString(ConfigFiles.hostname, Wifi.hostname);
  prefs.putString(ConfigFiles.static_ip, Wifi.static_ip);
  prefs.putString(ConfigFiles.static_netmask, Wifi.static_netmask);
  prefs.putString(ConfigFiles.static_gateway, Wifi.static_gateway);
  prefs.putString(ConfigFiles.static_dns, Wifi.static_dns);

#if MQTT_SUPPORTED == 1
  prefs.putString(ConfigFiles.mqtt_server, Wifi.mqtt.server);
  prefs.putString(ConfigFiles.mqtt_port, Wifi.mqtt.port);
  prefs.putString(ConfigFiles.mqtt_topic, Wifi.mqtt.topic);
  prefs.putString(ConfigFiles.mqtt_user, Wifi.mqtt.user);
  prefs.putString(ConfigFiles.mqtt_pwd, Wifi.mqtt.pwd);
#endif

  prefs.putString(ConfigFiles.syslog_ip, Wifi.syslog_ip);
  prefs.putBool(ConfigFiles.force_ap, Wifi.force_ap);
}

void saveParamCallback() {
  Log.println(F("[CALLBACK] saveParamCallback fired"));

  Wifi.hostname = customWMParams.hostname->getValue();
  Wifi.static_ip = customWMParams.static_ip->getValue();
  Wifi.static_netmask = customWMParams.static_netmask->getValue();
  Wifi.static_gateway = customWMParams.static_gateway->getValue();
  Wifi.static_dns = customWMParams.static_dns->getValue();

#if MQTT_SUPPORTED == 1
  Wifi.mqtt.server = customWMParams.mqtt_server->getValue();
  Wifi.mqtt.port = customWMParams.mqtt_port->getValue();
  Wifi.mqtt.topic = customWMParams.mqtt_topic->getValue();
  Wifi.mqtt.user = customWMParams.mqtt_user->getValue();
  Wifi.mqtt.pwd = customWMParams.mqtt_pwd->getValue();
#endif

  Wifi.syslog_ip = customWMParams.syslog_ip->getValue();

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
  if (!Wifi.syslog_ip.isEmpty()) {
    syslogStream.setDestination(Wifi.syslog_ip.c_str());
    const std::shared_ptr<LOGBase> syslogStreamPtr =
        std::make_shared<SyslogStream>(syslogStream);
    Log.addPrintStream(syslogStreamPtr);
    Log.printf("Syslog Server IP: %s\n", Wifi.syslog_ip.c_str());
  }
}

void setupWifiHost() {
  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  // ESP8266 needs this here (after WiFi.mode)
  WiFi.hostname(Wifi.hostname);
#if OTA_SUPPORTED == 0
  MDNS.begin(Wifi.hostname);
#endif
  Log.printf("setupWifiHost: hostname %s\n", Wifi.hostname.c_str());
}

// --- Zentrale Defaults
constexpr int DEFAULT_SLEEP_THR = 50;
constexpr int DEFAULT_WAKE_THR = 75;
constexpr int DEFAULT_AC_MAX = 3750;
constexpr int DEFAULT_OFFSET = -19;
constexpr int DEFAULT_PTOGRID_THR = 150;
constexpr int DEFAULT_PTOUSER_THR = 150;
constexpr int DEFAULT_POWER_LIMIT = 6132;

UserConfig validateUserConfig(const UserConfig& in) {
  UserConfig out = in;

  // BATTERY STANDBY
  // bool ist bereits bool → keine Validierung nötig

  // Sleep Threshold (>0)
  if (out.bat_slp_thr <= 0) out.bat_slp_thr = DEFAULT_SLEEP_THR;

  // Wake Threshold (>0)
  if (out.bat_wke_thr <= 0) out.bat_wke_thr = DEFAULT_WAKE_THR;

  // AC Max Power (2500–12500)
  if (out.ac_max_pow < 2500 || out.ac_max_pow > 12500)
    out.ac_max_pow = DEFAULT_AC_MAX;

  // Offset (-100 bis +100)
  if (out.ac_off_set < -100 || out.ac_off_set > 100)
    out.ac_off_set = DEFAULT_OFFSET;

  // Priority Control (bool)
  // bool → keine Validierung nötig

  // PTOGRID (>0)
  if (out.ptogrid_thr <= 0) out.ptogrid_thr = DEFAULT_PTOGRID_THR;

  // PTOUSER (>0)
  if (out.ptouser_thr <= 0) out.ptouser_thr = DEFAULT_PTOUSER_THR;

  // Power limit (>0)
  if (out.power_limit <= 0) out.power_limit = DEFAULT_POWER_LIMIT;

  return out;
}

void loadSettingsFromPrefs() {
  Preferences prefs;
  prefs.begin("config", true);

  UserConfig raw;

  raw.bat_standby = prefs.getBool("bat_standby", true);
  raw.bat_slp_thr = prefs.getInt("bat_slp_thr", DEFAULT_SLEEP_THR);
  raw.bat_wke_thr = prefs.getInt("bat_wke_thr", DEFAULT_WAKE_THR);
  raw.accharge = prefs.getBool("accharge", true);
  raw.ac_max_pow = prefs.getInt("ac_max_pow", DEFAULT_AC_MAX);
  raw.ac_off_set = prefs.getInt("ac_off_set", DEFAULT_OFFSET);
  raw.prioctrl = prefs.getBool("prioctrl", false);
  raw.ptogrid_thr = prefs.getInt("ptogrid_thr", DEFAULT_PTOGRID_THR);
  raw.ptouser_thr = prefs.getInt("ptouser_thr", DEFAULT_PTOUSER_THR);
  raw.surch = prefs.getBool("surch", false);
  raw.power_limit = prefs.getInt("power_limit", DEFAULT_POWER_LIMIT);

  prefs.end();

  User = validateUserConfig(raw);
}

#if MODBUS_TCP_SUPPORTED == 1
bool modbusReadHoldingRegister(uint16_t address, uint16_t* value) {
  return Inverter.ReadHoldingReg(address, value);
}

bool modbusReadInputRegister(uint16_t address, uint16_t* value) {
  return Inverter.ReadInputReg(address, value);
}

bool modbusWriteHoldingRegister(uint16_t address, uint16_t value) {
  return Inverter.WriteHoldingReg(address, value);
}
#endif

void setup() {
  // >>> LittleFS mounten (MUSS als erstes passieren)
  LittleFS.begin();
  httpServer.serveStatic("/pico.lime.min.css", LittleFS, "/pico.lime.min.css");

  WiFiManager wm;

  SetLED.begin();

#if ENABLE_DOUBLE_RESET == 1
  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
#endif

prefs.begin("ShineWiFi");
  loadConfig();
  loadSettingsFromPrefs();
  configureLogging();
  Log.begin();       // <-- MUSS direkt nach configureLogging() stehen!
  setupWifiHost();

  setupWifiManagerConfigMenu(wm);

  SetLED.on(LED_BLUE);
  SetLED.off(LED_RED);
  SetLED.off(LED_GREEN);
  // Set a timeout so the ESP doesn't hang waiting to be configured, for
  // instance after a power failure

  wm.setConfigPortalTimeout(CONFIG_PORTAL_MAX_TIME_SECONDS);

Log.printf("force_ap: %s\n", Wifi.force_ap ? "true" : "false");

#ifdef AP_BUTTON_PRESSED
  if (AP_BUTTON_PRESSED) {
Log.printf("AP Button pressed during power up -> force_ap set to true\n");
    Wifi.force_ap = true;
  }
#endif
#if ENABLE_DOUBLE_RESET == 1
  if (drd->detectDoubleReset()) {
    Log.println(F("Double reset detected"));
    Wifi.force_ap = true;
  }
#endif
  if (Wifi.force_ap) {
    prefs.putBool(ConfigFiles.force_ap, false);
    wm.startConfigPortal("GrowattConfig", APPassword);
    Log.printf("GrowattConfig finished\n");
    SetLED.on(LED_RED);
    delay(3000);
    ESP.restart();
  }

  // Set static ip
  if (!Wifi.static_ip.isEmpty() && !Wifi.static_netmask.isEmpty()) {
    IPAddress ip, netmask, gateway, dns;
    ip.fromString(Wifi.static_ip);
    netmask.fromString(Wifi.static_netmask);
    gateway.fromString(Wifi.static_gateway);
    dns.fromString(Wifi.static_dns);
Log.printf("Static IP Configuration:\n  IP:      %s\n  Netmask: %s\n  Gateway: %s\n  DNS:     %s\n",
             Wifi.static_ip.c_str(),
             Wifi.static_netmask.c_str(),
             Wifi.static_gateway.c_str(),
             Wifi.static_dns.c_str());
    if (!Wifi.static_dns.isEmpty()) {
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
  bool res = wm.autoConnect("GrowattConfig", APPassword);

  if (!res) {
Log.printf("Failed to connect WiFi!\n");
    SetLED.on(LED_RED);
    ESP.restart();
  }

  SetLED.off(LED_BLUE);
Log.println(F("WiFi connected"));

#if OTA_SUPPORTED == 1
#if !defined(OTA_PASSWORD)
#error "Please define an OTA_PASSWORD in Config.h"
#endif
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setHostname(Wifi.hostname.c_str());
  ArduinoOTA.begin();
#endif

#if MQTT_SUPPORTED == 1
#ifdef MQTTS_ENABLED
  espClient.setCACert(MQTTS_BROKER_CA_CERT);
#endif
  shineMqtt.mqttSetup(Wifi.mqtt);
#endif

  httpServer.on("/status", sendJsonSite);
  httpServer.on("/uiStatus", sendUiJsonSite);
  httpServer.on("/startAp", startConfigAccessPoint);
  httpServer.on("/reboot", rebootESP);
  httpServer.on("/loadfirst", loadFirst);
  httpServer.on("/batteryfirst", batteryFirst);
  httpServer.on("/gridfirst", gridFirst);
  //  #if ENABLE_MODBUS_COMMUNICATION == 1
  //    httpServer.on("/postCommunicationModbus", sendPostSite);
  httpServer.on("/postCommunicationModbus_p", HTTP_POST, handlePostData);
  //  #endif
  httpServer.on("/", sendMainPage);
#ifdef ENABLE_WEB_DEBUG
  httpServer.on("/debug", sendDebug);
#endif
  httpServer.onNotFound(handleNotFound);

  Inverter.InitProtocol();
  Inverter.begin(Serial);

  httpServer.on("/saveSettings", HTTP_POST,
                []() { handleSaveSettings(httpServer); });

  httpServer.on("/getSettings", HTTP_GET,
                []() { handleGetSettings(httpServer); });

  // --- OTA Firmware Upload (Web) ---
  httpServer.on(
      "/update", HTTP_POST, []() { handleUpdateFinished(httpServer); },
      []() { handleUpdateUpload(httpServer); });

  httpServer.begin();

// new
#if MODBUS_TCP_SUPPORTED == 1
  modbusTCP.readHoldingRegister = modbusReadHoldingRegister;
  modbusTCP.readInputRegister = modbusReadInputRegister;
  modbusTCP.writeHoldingRegister = modbusWriteHoldingRegister;
  modbusTCP.begin();
#endif

#if defined(DEFAULT_NTP_SERVER) && defined(DEFAULT_TZ_INFO)
  configTime(DEFAULT_TZ_INFO, DEFAULT_NTP_SERVER);
#endif
}

void handleSaveSettings(ESP8266WebServer& httpServer) {
  Preferences prefs;
  prefs.begin("config", false);

  UserConfig raw;

  raw.bat_standby = (httpServer.arg("bat_standby") == "on");
  raw.bat_slp_thr = httpServer.arg("bat_slp_thr").toInt();
  raw.bat_wke_thr = httpServer.arg("bat_wke_thr").toInt();
  raw.accharge = (httpServer.arg("accharge") == "on");
  raw.ac_max_pow = httpServer.arg("ac_max_pow").toInt();
  raw.ac_off_set = httpServer.arg("ac_off_set").toInt();
  raw.prioctrl = (httpServer.arg("prioctrl") == "on");
  raw.ptogrid_thr = httpServer.arg("ptogrid_thr").toInt();
  raw.ptouser_thr = httpServer.arg("ptouser_thr").toInt();
  raw.surch = (httpServer.arg("surch") == "on");
  raw.power_limit = httpServer.arg("power_limit").toInt();

  User = validateUserConfig(raw);

  prefs.putBool("bat_standby", User.bat_standby);
  prefs.putInt("bat_slp_thr", User.bat_slp_thr);
  prefs.putInt("bat_wke_thr", User.bat_wke_thr);
  prefs.putBool("accharge", User.accharge);
  prefs.putInt("ac_max_pow", User.ac_max_pow);
  prefs.putInt("ac_off_set", User.ac_off_set);
  prefs.putBool("prioctrl", User.prioctrl);
  prefs.putInt("ptogrid_thr", User.ptogrid_thr);
  prefs.putInt("ptouser_thr", User.ptouser_thr);
  prefs.putBool("surch", User.surch);
  prefs.putInt("power_limit", User.power_limit);

  prefs.end();
  httpServer.send(200, "text/plain", "Settings saved");
}

void handleGetSettings(ESP8266WebServer& httpServer) {
  Preferences prefs;
  prefs.begin("config", true);

  JsonDocument doc;

  // Battery Standby
  doc["bat_standby"] = prefs.getBool("bat_standby", User.bat_standby);
  doc["bat_slp_thr"] = prefs.getInt("bat_slp_thr", User.bat_slp_thr);
  doc["bat_wke_thr"] = prefs.getInt("bat_wke_thr", User.bat_wke_thr);

  // AC Charging
  doc["accharge"] = prefs.getBool("accharge", User.accharge);
  doc["ac_max_pow"] = prefs.getInt("ac_max_pow", User.ac_max_pow);
  doc["ac_off_set"] = prefs.getInt("ac_off_set", User.ac_off_set);

  // --- NEW: Priority Control ---
  doc["prioctrl"] = prefs.getBool("prioctrl", User.prioctrl);
  doc["ptogrid_thr"] = prefs.getInt("ptogrid_thr", User.ptogrid_thr);
  doc["ptouser_thr"] = prefs.getInt("ptouser_thr", User.ptouser_thr);
  doc["surch"] = prefs.getBool("surch", User.surch);
  doc["power_limit"] = prefs.getInt("power_limit", User.power_limit);

  prefs.end();
  sendJson(doc);
}

void handleUpdateFinished(ESP8266WebServer& httpServer) {
  bool ok = !Update.hasError();
  String msg = ok ? "Update successfull, rebooting..." : "Update failed!";
  httpServer.send(ok ? 200 : 500, "text/plain", msg);

  delay(1000);
  if (ok) {
    SetLED.on(LED_RED);
    ESP.restart();
  }
}

void handleUpdateUpload(ESP8266WebServer& httpServer) {
  HTTPUpload& upload = httpServer.upload();

  if (upload.status == UPLOAD_FILE_START) {
    size_t sketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(sketchSpace)) {
      Update.printError(Serial);
    }

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }

  } else if (upload.status == UPLOAD_FILE_END) {
    if (!Update.end(true)) {
      Update.printError(Serial);
    }
  }
}

void setupWifiManagerConfigMenu(WiFiManager& wm) {
  customWMParams.hostname = new WiFiManagerParameter(
      "hostname", "Hostname (no spaces or special characters)",
      Wifi.hostname.c_str(), 30);
  customWMParams.static_ip =
      new WiFiManagerParameter("staticip", "IP", Wifi.static_ip.c_str(), 15);
  customWMParams.static_netmask = new WiFiManagerParameter(
      "staticnetmask", "Netmask", Wifi.static_netmask.c_str(), 15);
  customWMParams.static_gateway = new WiFiManagerParameter(
      "staticgateway", "Gateway", Wifi.static_gateway.c_str(), 15);
  customWMParams.static_dns =
      new WiFiManagerParameter("staticdns", "DNS", Wifi.static_dns.c_str(), 15);
#if MQTT_SUPPORTED == 1
  customWMParams.mqtt_server = new WiFiManagerParameter(
      "mqttserver", "Server", Wifi.mqtt.server.c_str(), 40);
  customWMParams.mqtt_port =
      new WiFiManagerParameter("mqttport", "Port", Wifi.mqtt.port.c_str(), 6);
  customWMParams.mqtt_topic = new WiFiManagerParameter(
      "mqtttopic", "Topic", Wifi.mqtt.topic.c_str(), 64);
  customWMParams.mqtt_user = new WiFiManagerParameter(
      "mqttusername", "Username", Wifi.mqtt.user.c_str(), 40);
  customWMParams.mqtt_pwd = new WiFiManagerParameter("mqttpassword", "Password",
                                                     Wifi.mqtt.pwd.c_str(), 64);
#endif
  customWMParams.syslog_ip = new WiFiManagerParameter(
      "syslogip", "Syslog Server IP (leave blank for none)",
      Wifi.syslog_ip.c_str(), 15);
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

  // ESP8266: std::clamp verfügbar, serializeJson akzeptiert rvalue
  serializeJson(doc, httpServer.client());
}

void sendJsonSite(void) {
  if (!readoutSucceeded) {
    httpServer.send(503, F("text/plain"), F("Service Unavailable"));
    return;
  }

  JsonDocument doc;
  Inverter.CreateJson(doc, WiFi.macAddress(), Wifi.hostname);

  sendJson(doc);
}

void sendUiJsonSite(void) {
  JsonDocument doc;
  Inverter.CreateUIJson(doc, Wifi.hostname);

  sendJson(doc);
}

#if MQTT_SUPPORTED == 1
boolean sendMqttJson(void) {
  JsonDocument doc;

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
  httpServer.send(200, F("text/plain"), F("Load First"));

  JsonDocument req1, res1;
  const char* payload1 = "{\"mode\": 0, \"retry\": 2}";
  Inverter.HandleCommand("priority/set", (const byte*)payload1,
                         strlen(payload1), req1, res1);

  JsonDocument req2, res2;
  const char* payload2 = "{\"value\": 100, \"retry\": 2}";
  Inverter.HandleCommand("bdc/set/chargepowerrate", (const byte*)payload2,
                         strlen(payload2), req2, res2);
}

void batteryFirst(void) {
  httpServer.send(200, F("text/plain"), F("Battery First"));
  JsonDocument req, res;
  const char* payload = "{\"mode\": 1, \"retry\": 2}";
  Inverter.HandleCommand("priority/set", (const byte*)payload, strlen(payload),
                         req, res);
}

void gridFirst(void) {
  httpServer.send(200, F("text/plain"), F("Grid First"));
  JsonDocument req, res;
  const char* payload = "{\"mode\": 2, \"retry\": 2}";
  Inverter.HandleCommand("priority/set", (const byte*)payload, strlen(payload),
                         req, res);
}

#ifdef ENABLE_WEB_DEBUG
void sendDebug(void) {
  httpServer.sendHeader("Location",
                        "http://" + WiFi.localIP().toString() + ":8080/", true);
  httpServer.send(302, F("text/plain"), "");
}
#endif

void sendMainPage(void) { httpServer.send(200, F("text/html"), MAIN_page); }

void handlePostData() {
  char msg[256];

  // --- Parameter einlesen ---
  const String opStr = httpServer.arg(F("operation"));
  const String regStr = httpServer.arg(F("reg"));
  const String valStr = httpServer.arg(F("val"));
  const String widthStr = httpServer.arg(F("width"));
  const String typeStr = httpServer.arg(F("type"));

  const bool isWrite = (opStr == "W");
  const bool isRead = (opStr == "R");
  const bool is16 = (widthStr == "16b");
  const bool isInput = (typeStr == "I");
  const bool isHolding = (typeStr == "H");

  // --- Pflichtparameter prüfen ---
  if (!httpServer.hasArg(F("reg")) ||
      (isWrite && !httpServer.hasArg(F("val")))) {
    httpServer.send(400, F("text/plain"), F("400: Invalid Request"));
    return;
  }

  const uint16_t reg = regStr.toInt();

  // --- READ ---
  if (isRead) {
    if (!isInput && !isHolding) {
      httpServer.send(400, F("text/plain"), F("400: Invalid Type"));
      return;
    }

    const char* typeName = isInput ? "Input" : "Holding";

    if (is16) {
      uint16_t val = 0;
      bool ok = isInput ? Inverter.ReadInputReg(reg, &val)
                        : Inverter.ReadHoldingReg(reg, &val);

      if (ok) {
        snprintf_P(msg, sizeof(msg),
                   PSTR("Reading Value %u from 16-bit %s Register %u succeeded"),
                   val, typeName, reg);
      } else {
        snprintf_P(msg, sizeof(msg),
                   PSTR("Reading from 16-bit %s Register %u failed!"),
                   typeName, reg);
      }

    } else if (widthStr == "32b") {
      uint32_t val = 0;
      bool ok = isInput ? Inverter.ReadInputReg(reg, &val)
                        : Inverter.ReadHoldingReg(reg, &val);

      if (ok) {
        snprintf_P(msg, sizeof(msg),
                   PSTR("Reading Value %lu from 32-bit %s Register %u succeeded"),
                   val, typeName, reg);
      } else {
        snprintf_P(msg, sizeof(msg),
                   PSTR("Reading from 32-bit %s Register %u failed!"),
                   typeName, reg);
      }

    } else {
      snprintf_P(msg, sizeof(msg), PSTR("Unknown type (expected 16b or 32b)"));
    }

    Log.printf("Modbus Read: %s\n", msg);
    httpServer.send(200, F("text/plain"), msg);
    return;
  }

  // --- WRITE ---
  if (isWrite) {
    if (!isHolding) {
      snprintf_P(msg, sizeof(msg),
                 PSTR("Writing to Input Registers not possible!"));
      httpServer.send(200, F("text/plain"), msg);
      return;
    }

    if (!is16) {
      snprintf_P(msg, sizeof(msg),
                 PSTR("Writing to 32-bit Registers not supported!"));
      httpServer.send(200, F("text/plain"), msg);
      return;
    }

    uint16_t val = valStr.toInt();
    bool ok = Inverter.WriteHoldingReg(reg, val);

    if (ok) {
      snprintf_P(msg, sizeof(msg),
                 PSTR("Writing Value %u to Holding Register %u succeeded"),
                 val, reg);
    } else {
      snprintf_P(msg, sizeof(msg),
                 PSTR("Writing Value %u to Holding Register %u failed!"),
                 val, reg);
    }

    Log.printf("Modbus Write: %s\n", msg);
    httpServer.send(200, F("text/plain"), msg);
    return;
  }

  // --- Unbekannte Operation ---
  httpServer.send(400, F("text/plain"), F("400: Unknown operation"));
}

bool sendSingleValue(void) {
  if (!readoutSucceeded) {
    httpServer.send(503, F("text/plain"), F("Service unavailable"));
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
  String msg = "Not found: " + httpServer.uri();
  httpServer.send(404, F("text/plain"), msg);
}

#if defined(DEFAULT_NTP_SERVER) && defined(DEFAULT_TZ_INFO)
void handleNTPSync() {
  int reachable = sntp_getreachability(0);
  Log.printf("NTP Server: %s reachable %d\n", DEFAULT_NTP_SERVER,
             reachable & 1);

  if (reachable & 1) {
    JsonDocument req, res;
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

void updateStatusLEDs() {
  bool wifiOK = (WiFi.status() == WL_CONNECTED);
  bool modbusOK = readoutSucceeded;
  bool mqttOK = false;

#if MQTT_SUPPORTED == 1
  mqttOK = shineMqtt.mqttConnected();
#endif

  // --- Fall 1: Grün blinkt ---
  if (wifiOK && modbusOK && mqttOK) {
    SetLED.blink(LED_GREEN, 500);
    SetLED.off(LED_RED);
    SetLED.off(LED_BLUE);
    return;
  }

  // --- Fall 3: Blau blinkt ---
  if (wifiOK && modbusOK && !mqttOK) {
    SetLED.blink(LED_BLUE, 500);
    SetLED.off(LED_GREEN);
    SetLED.off(LED_RED);
    return;
  }

  // --- Fall 2: Rot blinkt ---
  if (modbusOK && !wifiOK) {
    SetLED.blink(LED_RED, 500);
    SetLED.off(LED_GREEN);
    SetLED.off(LED_BLUE);
    return;
  }

  // --- Default ---
  SetLED.off(LED_GREEN);
  SetLED.off(LED_RED);
  SetLED.off(LED_BLUE);
}

// -------------------------------------------------------
// Main loop
// -------------------------------------------------------
#if ENABLE_AP_BUTTON == 1
unsigned long ButtonTimer = 0;
#endif
unsigned long RefreshTimer = 0;
unsigned long BatteryStandbyTimer = 0;
unsigned long ACChargeControlTimer = 0;
#if defined(DEFAULT_NTP_SERVER) && defined(DEFAULT_TZ_INFO)
unsigned long NTPTimer = 0;
unsigned long lastSync = 0;
bool initialSyncDone = false;
#endif

void loop() {
#if ENABLE_DOUBLE_RESET
  drd->loop();
#endif
  SetLED.loop();

  Log.loop();
  unsigned long now = millis();
  wl_status_t wifiState = WiFi.status();

#ifdef AP_BUTTON_PRESSED
  if (now - ButtonTimer > BUTTON_TIMER) {
    ButtonTimer = now;
    if (AP_BUTTON_PRESSED) {
      btnPressed++;
      Log.printf("Button pressed (%d/5)\n", btnPressed);
      if (btnPressed > 5) {
        Log.println(F("Handle press"));
        StartedConfigAfterBoot = true;
      }
    } else {
      btnPressed = 0;
    }
  }
#endif

  if (StartedConfigAfterBoot) {
    Log.println(F("StartedConfigAfterBoot"));
    prefs.putBool(ConfigFiles.force_ap, true);
    SetLED.on(LED_RED);
    delay(3000);
    ESP.restart();
  }

  WiFi_Reconnect();

#if MQTT_SUPPORTED == 1
  if (wifiState == WL_CONNECTED && shineMqtt.mqttReconnect()) shineMqtt.loop();
#endif

  httpServer.handleClient();

#if MODBUS_TCP_SUPPORTED == 1
  if (modbusTCP.isEnabled()) modbusTCP.loop();
#endif

  // Inverter read
  if (now - RefreshTimer > REFRESH_TIMER) {
    RefreshTimer = now;

    readoutSucceeded = Inverter.ReadData(NUM_OF_RETRIES);

    updateStatusLEDs();

#if MQTT_SUPPORTED == 1
    if (readoutSucceeded && shineMqtt.mqttEnabled()) {
      sendMqttJson();
    } else {
      JsonDocument doc;
      doc["InverterStatus"] = -1;
      shineMqtt.mqttPublish(doc);
    }
#endif

#if PINGER_SUPPORTED == 1
    if (!pinger.Ping(GATEWAY_IP)) {
      SetLED.on(LED_RED);
      delay(3000);
      ESP.restart();
    }
#endif
  }

#if defined(DEFAULT_NTP_SERVER) && defined(DEFAULT_TZ_INFO)
  if (!initialSyncDone && now > 60000) {
    handleNTPSync();
    lastSync = now;
    initialSyncDone = true;
  } else if (initialSyncDone && now - lastSync > NTP_TIMER) {
    handleNTPSync();
    lastSync = now;
  }
#endif

#if OTA_SUPPORTED == 1
  ArduinoOTA.handle();
#endif

  if (User.bat_standby && now - BatteryStandbyTimer > BATTERY_STANDBY_TIMER) {
    BatteryStandbyTimer = now;
    batteryStandby();
    // Log.print("BatteryStandby active");
  }

  if (User.accharge && now - ACChargeControlTimer > ACCHARGE_CONTROL_TIMER) {
    ACChargeControlTimer = now;
    acchargeControl();
    if (User.surch) {
      surplusCharge();
    }
    // Log.print("ACControl active");
    if (User.prioctrl) {
      priorityControl();
      // Log.print("PriorityControl active");
    }
  }
}
