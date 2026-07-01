# 1 "/tmp/tmp5s12cb9g"
#include <Arduino.h>
# 1 "/home/dirkm/Dokumente/GitHub/OpenInverterGateway-lite/SRC/ShineWiFi-ModBus/ShineWiFi-ModBus.ino"



#include "Config.h"
#ifndef _SHINE_CONFIG_H_
#error Please rename Config.h.example to Config.h
#endif




#include "ShineWifi.h"
#include "Index.h"
#include "Growatt.h"
#include "GrowattTLXH.h"

#if MQTT_SUPPORTED == 1
#include "ShineMqtt.h"
#endif

#if MODBUS_TCP_SUPPORTED == 1
#include "ModbusTCP.h"
#endif




#include <LittleFS.h>
#include <Preferences.h>
#include <StreamUtils.h>
#include <TLog.h>
#include <WiFiManager.h>




#if defined(ESP8266)
#include <Updater.h>
#elif defined(ESP32)
#include <Update.h>
#include <esp_task_wdt.h>
#endif




#if OTA_SUPPORTED == 1
#include <ArduinoOTA.h>
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




#if defined(DEFAULT_NTP_SERVER) && defined(DEFAULT_TZ_INFO)
#include <time.h>
extern "C" uint8_t sntp_getreachability(uint8_t);
#endif




Preferences prefs;
Growatt Inverter;
bool StartedConfigAfterBoot = false;

#if MQTT_SUPPORTED == 1
#if defined(MQTTS_ENABLED)
WiFiClientSecure espClient;
#else
WiFiClient espClient;
#endif
ShineMqtt shineMqtt(espClient, Inverter);
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

#if defined(ESP8266)
ESP8266WebServer httpServer(80);
#elif defined(ESP32)
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

struct UserConfig {
  bool bat_standby;
  int bat_slp_thr;
  int bat_wke_thr;

  bool accharge;
  int ac_max_pow;
  int ac_off_set;
};

WifiConfig Wifi;
UserConfig User;

#define CONFIG_PORTAL_MAX_TIME_SECONDS 300
void updateRedLed();
void WiFi_Reconnect();
void InverterReconnect(void);
void configureLogging();
void setupGPIO();
void setupWifiHost();
void startWdt();
void loadSettingsFromPrefs();
bool modbusReadHoldingRegister(uint16_t address, uint16_t* value);
bool modbusReadInputRegister(uint16_t address, uint16_t* value);
bool modbusWriteHoldingRegister(uint16_t address, uint16_t value);
void setup();
void setupMenu(WiFiManager& wm, bool enableCustomParams);
void sendJson(JsonDocument& doc);
void sendJsonSite(void);
void sendUiJsonSite(void);
void sendMetrics(void);
boolean sendMqttJson(void);
void startConfigAccessPoint(void);
void rebootESP(void);
void loadFirst(void);
void batteryFirst(void);
void gridFirst(void);
void sendDebug(void);
void sendMainPage(void);
void handlePostData();
bool sendSingleValue(void);
void handleNotFound();
void handleNTPSync();
bool writeWithRetry(uint16_t reg, uint16_t value);
void batteryStandby();
void acchargeControl();
void loop();
#line 182 "/home/dirkm/Dokumente/GitHub/OpenInverterGateway-lite/SRC/ShineWiFi-ModBus/ShineWiFi-ModBus.ino"
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




void WiFi_Reconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_GN, 0);

    while (WiFi.status() != WL_CONNECTED) {
      delay(200);
      Log.print(F("."));
      digitalWrite(LED_RT,
                   !digitalRead(LED_RT));
    }


    WiFi.printDiag(Serial);
    Log.print(F("Local IP: "));
    Log.println(WiFi.localIP());
    Log.print(F("Hostname: "));
    Log.println(Wifi.hostname);

    Log.println(F("WiFi reconnected"));

    updateRedLed();
  }
}
# 232 "/home/dirkm/Dokumente/GitHub/OpenInverterGateway-lite/SRC/ShineWiFi-ModBus/ShineWiFi-ModBus.ino"
void InverterReconnect(void) {

  Inverter.begin(Serial);

  if (Inverter.GetWiFiStickType() == ShineWiFi_X)
    Log.println(F("ShineWiFi-X (USB) found"));
  else if (Inverter.GetWiFiStickType() == ShineWiFi_S)
    Log.println(F("ShineWiFi-S (Serial) found"));
  else
    Log.println(F("Error: no ShineWiFi stick found!"));
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
    Log.print(F("Syslog Server IP: "));
    Log.println(Wifi.syslog_ip);
  }
}

void setupGPIO() {
  pinMode(LED_GN, OUTPUT);
  pinMode(LED_RT, OUTPUT);
  pinMode(LED_BL, OUTPUT);
}

void setupWifiHost() {
#ifdef ESP32

  WiFi.hostname(Wifi.hostname);
#endif
  WiFi.mode(WIFI_STA);
#ifdef ESP8266

  WiFi.hostname(Wifi.hostname);
#endif
#if OTA_SUPPORTED == 0
  MDNS.begin(Wifi.hostname);
#endif
  Log.print(F("setupWifiHost: hostname "));
  Log.println(Wifi.hostname);
}

#if defined(ESP32)
# 383 "/home/dirkm/Dokumente/GitHub/OpenInverterGateway-lite/SRC/ShineWiFi-ModBus/ShineWiFi-ModBus.ino"
void startWdt() {
  Log.print(F("Configuring WDT with Timeout of "));
  Log.print(WDT_TIMEOUT);
  Log.println(F(" Seconds"));
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
}
#endif
# 416 "/home/dirkm/Dokumente/GitHub/OpenInverterGateway-lite/SRC/ShineWiFi-ModBus/ShineWiFi-ModBus.ino"
constexpr int DEFAULT_SLEEP_THR = 50;
constexpr int DEFAULT_WAKE_THR = 75;
constexpr int DEFAULT_AC_MAX = 3750;
constexpr int DEFAULT_OFFSET = -1;

void loadSettingsFromPrefs() {
  Preferences prefs;
  prefs.begin("config", true);


  User.bat_standby = prefs.getBool("bat_standby", false);


  {
    int v = prefs.getString("bat_slp_thr", String(DEFAULT_SLEEP_THR)).toInt();
    if (v <= 0) v = 1;
    User.bat_slp_thr = v;
  }


  {
    int v = prefs.getString("bat_wke_thr", String(DEFAULT_WAKE_THR)).toInt();
    if (v <= 0) v = 1;
    User.bat_wke_thr = v;
  }


  User.accharge = prefs.getBool("accharge", false);


  {
    int v = prefs.getString("ac_max_pow", String(DEFAULT_AC_MAX)).toInt();
    if (v <= 0) v = 1;
    User.ac_max_pow = v;
  }


  {
    int v = prefs.getString("ac_off_set", String(DEFAULT_OFFSET)).toInt();
    if (v < -99) v = -99;
    if (v > 99) v = 99;
    User.ac_off_set = v;
  }

  prefs.end();
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

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed!");
  } else {
    Serial.println("LittleFS OK");
  }


  httpServer.serveStatic("/pico.min.css", LittleFS, "/pico.min.css");

  WiFiManager wm;

  Log.println(F("Setup()"));

  setupGPIO();

#if ENABLE_DOUBLE_RESET == 1
  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
#endif

  prefs.begin("ShineWiFi");
  loadConfig();
  loadSettingsFromPrefs();
  configureLogging();
  setupWifiHost();

#if OTA_SUPPORTED == 1
#if !defined(OTA_PASSWORD)
#error "Please define an OTA_PASSWORD in Config.h"
#endif
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setHostname(Wifi.hostname.c_str());
  ArduinoOTA.begin();
#endif

  Log.begin();

  setupWifiManagerConfigMenu(wm);

  digitalWrite(LED_BL, 1);
  digitalWrite(LED_RT, 0);
  digitalWrite(LED_GN, 0);




  wm.setConfigPortalTimeout(CONFIG_PORTAL_MAX_TIME_SECONDS);

  Log.print(F("force_ap: "));
  Log.println(Wifi.force_ap);

#ifdef AP_BUTTON_PRESSED
  if (AP_BUTTON_PRESSED) {
    Log.println(F("AP Button pressed during power up"));
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
#if defined(ESP32)
    esp_task_wdt_delete(NULL);
#endif
    wm.startConfigPortal("GrowattConfig", APPassword);
    Log.println(F("GrowattConfig finished"));
#if defined(ESP32)
    esp_task_wdt_add(NULL);
#endif
    digitalWrite(LED_BL, 0);
    delay(3000);
    ESP.restart();
  }


  if (!Wifi.static_ip.isEmpty() && !Wifi.static_netmask.isEmpty()) {
    IPAddress ip, netmask, gateway, dns;
    ip.fromString(Wifi.static_ip);
    netmask.fromString(Wifi.static_netmask);
    gateway.fromString(Wifi.static_gateway);
    dns.fromString(Wifi.static_dns);
    Log.print(F("static ip: "));
    Log.println(Wifi.static_ip);
    Log.print(F("static netmask: "));
    Log.println(Wifi.static_netmask);
    Log.print(F("static gateway: "));
    Log.println(Wifi.static_gateway);
    Log.print(F("static dns: "));
    Log.println(Wifi.static_dns);
    if (!Wifi.static_dns.isEmpty()) {
      wm.setSTAStaticIPConfig(ip, gateway, netmask, dns);
    } else {
      wm.setSTAStaticIPConfig(ip, gateway, netmask);
    }
  }




  int connect_timeout_seconds = 15;
  wm.setConnectTimeout(connect_timeout_seconds);
  bool res = wm.autoConnect("GrowattConfig",
                            APPassword);

  if (!res) {
    Log.println(F("Failed to connect WiFi!"));
    ESP.restart();
  } else {
    digitalWrite(LED_BL, 0);

    Log.println(F("WiFi connected"));
  }

  while (WiFi.status() != WL_CONNECTED) {
    WiFi_Reconnect();
  }

#if MQTT_SUPPORTED == 1
#ifdef MQTTS_ENABLED
  espClient.setCACert(MQTTS_BROKER_CA_CERT);
#endif
  shineMqtt.mqttSetup(Wifi.mqtt);
#endif

  httpServer.on("/status", sendJsonSite);
  httpServer.on("/uiStatus", sendUiJsonSite);
  httpServer.on("/metrics", sendMetrics);
  httpServer.on("/startAp", startConfigAccessPoint);
  httpServer.on("/reboot", rebootESP);
  httpServer.on("/loadfirst", loadFirst);
  httpServer.on("/batteryfirst", batteryFirst);
  httpServer.on("/gridfirst", gridFirst);


  httpServer.on("/postCommunicationModbus_p", HTTP_POST, handlePostData);

  httpServer.on("/", sendMainPage);
#ifdef ENABLE_WEB_DEBUG
  httpServer.on("/debug", sendDebug);
#endif
  httpServer.onNotFound(handleNotFound);

  Inverter.InitProtocol();
  InverterReconnect();
httpServer.on("/saveSettings", HTTP_POST, []() {
    Preferences prefs;
    prefs.begin("config", false);




    User.bat_standby = httpServer.hasArg("bat_standby");
    prefs.putBool("bat_standby", User.bat_standby);


    {
      int v = httpServer.arg("bat_slp_thr").toInt();
      if (v <= 0) v = 1;
      User.bat_slp_thr = v;
      prefs.putInt("bat_slp_thr", v);
    }


    {
      int v = httpServer.arg("bat_wke_thr").toInt();
      if (v <= 0) v = 1;
      User.bat_wke_thr = v;
      prefs.putInt("bat_wke_thr", v);
    }




    User.accharge = httpServer.hasArg("accharge");
    prefs.putBool("accharge", User.accharge);


    {
      int v = httpServer.arg("ac_max_pow").toInt();
      if (v <= 0) v = 1;
      User.ac_max_pow = v;
      prefs.putInt("ac_max_pow", v);
    }


    {
      int v = httpServer.arg("ac_off_set").toInt();
      if (v < -99) v = -99;
      if (v > 99) v = 99;
      User.ac_off_set = v;
      prefs.putInt("ac_off_set", v);
    }

    prefs.end();
    httpServer.send(200, "text/plain", "Settings saved");
});

httpServer.on("/getSettings", HTTP_GET, []() {
    Preferences prefs;
    prefs.begin("config", true);

    DynamicJsonDocument doc(512);




    doc["bat_standby"] = prefs.getBool("bat_standby", User.bat_standby);
    doc["bat_slp_thr"] = prefs.getInt("bat_slp_thr", User.bat_slp_thr);
    doc["bat_wke_thr"] = prefs.getInt("bat_wke_thr", User.bat_wke_thr);




    doc["accharge"] = prefs.getBool("accharge", User.accharge);
    doc["ac_max_pow"] = prefs.getInt("ac_max_pow", User.ac_max_pow);
    doc["ac_off_set"] = prefs.getInt("ac_off_set", User.ac_off_set);

    prefs.end();

    sendJson(doc);
});


  httpServer.on(
      "/update", HTTP_POST,
      []() {

        bool ok = !Update.hasError();
        String msg = ok ? "Update successfull, rebooting..." : "Update failed!";
        httpServer.send(ok ? 200 : 500, "text/plain", msg);
        delay(1000);
        if (ok) {
          ESP.restart();
        }
      },
      []() {

        HTTPUpload& upload = httpServer.upload();
        if (upload.status == UPLOAD_FILE_START) {
          Serial.printf("Update: %s\n", upload.filename.c_str());
#if defined(ESP32)
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
#else
          if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) &
                            0xFFFFF000)) {
#endif
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          if (Update.write(upload.buf, upload.currentSize) !=
              upload.currentSize) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_END) {
          if (Update.end(true)) {
            Serial.printf("Update Success: %u bytes\nRebooting...\n",
                          upload.totalSize);
          } else {
            Update.printError(Serial);
          }
        }
      });

  httpServer.begin();


#if MODBUS_TCP_SUPPORTED == 1
  modbusTCP.readHoldingRegister = modbusReadHoldingRegister;
  modbusTCP.readInputRegister = modbusReadInputRegister;
  modbusTCP.writeHoldingRegister = modbusWriteHoldingRegister;
  modbusTCP.begin();
#endif

#if defined(DEFAULT_NTP_SERVER) && defined(DEFAULT_TZ_INFO)
#if defined(ESP32)
  configTime(0, 0, DEFAULT_NTP_SERVER);
  setenv("TZ", DEFAULT_TZ_INFO, 1);
#else
  configTime(DEFAULT_TZ_INFO, DEFAULT_NTP_SERVER);
#endif
#endif

#if defined(ESP32)
  startWdt();
#endif
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






void setupMenu(WiFiManager& wm, bool enableCustomParams) {
  Log.println(F("Setting up WiFiManager menu"));
  std::vector<const char*> menu = {"wifi", "wifinoscan", "update"};
  if (enableCustomParams) {
    menu.push_back("param");
  }
  menu.push_back("sep");
  menu.push_back("erase");
  menu.push_back("restart");

  wm.setMenu(menu);
}

void sendJson(JsonDocument& doc) {
    httpServer.setContentLength(measureJson(doc));
    httpServer.send(200, "application/json", "");

#if defined(ESP8266)

    serializeJson(doc, httpServer.client());
#else

    WiFiClient client = httpServer.client();
    serializeJson(doc, client);
#endif
}

void sendJsonSite(void) {
  if (!readoutSucceeded) {
    httpServer.send(503, F("text/plain"), F("Service Unavailable"));
    return;
  }

  DynamicJsonDocument doc(JSON_DOCUMENT_SIZE);
  Inverter.CreateJson(doc, WiFi.macAddress(), Wifi.hostname);

  sendJson(doc);
}

void sendUiJsonSite(void) {
  DynamicJsonDocument doc(JSON_DOCUMENT_SIZE);
  Inverter.CreateUIJson(doc, Wifi.hostname);

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

  Inverter.CreateMetrics(metrics, WiFi.macAddress(), Wifi.hostname);

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
  httpServer.send(200, F("text/plain"), F("Load First"));
  StaticJsonDocument<128> req, res;
  const char* payload = "{\"mode\": 0}";
  Inverter.HandleCommand("priority/set", (const byte*)payload, strlen(payload),
                         req, res);
}

void batteryFirst(void) {
  httpServer.send(200, F("text/plain"), F("Battery First"));
  StaticJsonDocument<128> req, res;
  const char* payload = "{\"mode\": 1}";
  Inverter.HandleCommand("priority/set", (const byte*)payload, strlen(payload),
                         req, res);
}

void gridFirst(void) {
  httpServer.send(200, F("text/plain"), F("Grid First"));
  StaticJsonDocument<128> req, res;
  const char* payload = "{\"mode\": 2}";
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


  if (!httpServer.hasArg(F("reg")) ||
      (isWrite && !httpServer.hasArg(F("val")))) {
    httpServer.send(400, F("text/plain"), F("400: Invalid Request"));
    return;
  }

  const uint16_t reg = regStr.toInt();


  if (isRead) {
    if (!isInput && !isHolding) {
      httpServer.send(400, F("text/plain"), F("400: Invalid Type"));
      return;
    }

    if (is16) {
      uint16_t val;
      bool ok = isInput ? Inverter.ReadInputReg(reg, &val)
                        : Inverter.ReadHoldingReg(reg, &val);

      snprintf_P(
          msg, sizeof(msg),
          ok ? PSTR("Reading Value %u from 16-bit %s Register %u succeeded")
             : PSTR("Reading from 16-bit %s Register %u failed!"),
          val, isInput ? "Input" : "Holding", reg);

    } else if (widthStr == "32b") {
      uint32_t val;
      bool ok = isInput ? Inverter.ReadInputReg(reg, &val)
                        : Inverter.ReadHoldingReg(reg, &val);

      snprintf_P(
          msg, sizeof(msg),
          ok ? PSTR("Reading Value %lu from 32-bit %s Register %u succeeded")
             : PSTR("Reading from 32-bit %s Register %u failed!"),
          val, isInput ? "Input" : "Holding", reg);

    } else {
      snprintf_P(msg, sizeof(msg), PSTR("Unknown type (expected 16b or 32b)"));
    }

    httpServer.send(200, F("text/plain"), msg);
    return;
  }


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

    snprintf_P(msg, sizeof(msg),
               ok ? PSTR("Writing Value %u to Holding Register %u succeeded")
                  : PSTR("Writing Value %u to Holding Register %u failed!"),
               val, reg);

    httpServer.send(200, F("text/plain"), msg);
    return;
  }


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
  Log.print(F("NTP Server: "));
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


bool writeWithRetry(uint16_t reg, uint16_t value) {
  const int maxRetries = 4;
  const int retryInterval = 200;

  for (int attempts = 0; attempts < maxRetries; attempts++) {
    if (Inverter.WriteHoldingReg(reg, value)) {
      return true;
    }
    delay(retryInterval);
  }
  return false;
}

void batteryStandby() {

  uint32_t wake_threshold = User.bat_wke_thr * 10;
  uint32_t sleep_threshold = User.bat_slp_thr * 10;


  int32_t soc = Inverter._Protocol.InputRegisters[P3000_BDC_SOC].value;
  int32_t discharge_stop =
      Inverter._Protocol.HoldingRegisters[P3000_BDC_DISCHARGE_STOPSOC].value;
  int32_t discharge_stop_w =
      Inverter._Protocol.HoldingRegisters[P3000_BDC_DISCHARGE_STOPSOC + 5]
          .value;
  int32_t discharge_rate =
      Inverter._Protocol.HoldingRegisters[P3000_BDC_DISCHARGE_P_RATE].value;

  int32_t sysstate =
      Inverter._Protocol.InputRegisters[P3000_BDC_SYSSTATE].value;
  int32_t ptogrid =
      Inverter._Protocol.InputRegisters[P3000_PTOGRID_TOTAL].value;
  int32_t inverter_status =
      Inverter._Protocol.InputRegisters[P3000_INVERTER_STATUS].value;
  int32_t ppv = Inverter._Protocol.InputRegisters[P3000_PPV].value;


  if (soc >= 10 && soc <= discharge_stop) {
    if (discharge_rate != 0) {
      if (writeWithRetry(3036, 0)) {
        Log.println(F("Battery discharging deactivated"));
      } else {
        Log.println(F("Battery discharging still activated!"));
      }
    }
  }


  else if (soc >= discharge_stop_w) {
    if (discharge_rate != 100) {
      if (writeWithRetry(3036, 100)) {
        Log.println(F("Battery discharging activated"));
      } else {
        Log.println(F("Battery discharging still deactivated!"));
      }
    }
  }


  if (sysstate == 0) {
    if (ptogrid >= (int32_t)wake_threshold && inverter_status == 1) {
      if (writeWithRetry(0, 3)) {
        Log.println(F("Battery activated"));
      } else {
        Log.println(F("Battery still deactivated!"));
      }
    }
  }


  else if (sysstate == 1) {
    if (ptogrid <= (int32_t)sleep_threshold &&
        ppv <= (int32_t)sleep_threshold && soc >= 10 && soc <= discharge_stop) {
      if (writeWithRetry(0, 2)) {
        Log.println(F("Battery deactivated"));
      } else {
        Log.println(F("Battery still activated!"));
      }
    }
  }
}

void acchargeControl() {

  uint32_t max_power = User.ac_max_pow;
  if (max_power == 0) max_power = DEFAULT_AC_MAX;

  int32_t off_set = User.ac_off_set;
  if (off_set < -99) off_set = -99;
  if (off_set > 99) off_set = 99;


  int32_t priority = Inverter._Protocol.InputRegisters[P3000_PRIORITY].value;
  int32_t ac_enabled =
      Inverter._Protocol.HoldingRegisters[P3000_BDC_CHARGE_AC_ENABLED].value;
  int32_t soc = Inverter._Protocol.InputRegisters[P3000_BDC_SOC].value;

  int32_t p_chr = Inverter._Protocol.InputRegisters[P3000_BDC_PCHR].value;
  int32_t p_togrid =
      Inverter._Protocol.InputRegisters[P3000_PTOGRID_TOTAL].value;
  int32_t p_touser =
      Inverter._Protocol.InputRegisters[P3000_PTOUSER_TOTAL].value;

  uint16_t current_rate =
      Inverter._Protocol.HoldingRegisters[P3000_BDC_CHARGE_P_RATE].value;


  if (priority == 1 && ac_enabled == 1) {

    if (soc == 100) {
      loadFirst();
      return;
    }


    int64_t delta = (int64_t)p_chr + (int64_t)p_togrid - (int64_t)p_touser;


    int32_t rawRate = (delta * 10) / max_power;
    int32_t roundedRate = rawRate + off_set;


    uint16_t targetpowerrate =
#if defined(ESP8266)
        std::clamp<int32_t>(roundedRate, 0, 100);
#else
        (roundedRate < 0) ? 0 : (roundedRate > 100 ? 100 : roundedRate);
#endif


    if (current_rate != targetpowerrate) {
      if (writeWithRetry(3047, targetpowerrate)) {
        Log.print(F("Set BDCChargePowerRate: "));
        Log.print(targetpowerrate);
        Log.println(F(" %"));
      } else {
        Log.println(F("Failed to set BDCChargePowerRate!"));
      }
    }
  }
}




#if ENABLE_AP_BUTTON == 1
unsigned long ButtonTimer = 0;
#endif
unsigned long LEDTimer = 0;
unsigned long RefreshTimer = 0;
unsigned long WifiRetryTimer = 0;
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

  Log.loop();
  wl_status_t wifiState = WiFi.status();
  uint8_t stick = Inverter.GetWiFiStickType();
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

  if (wifiState != WL_CONNECTED) {
    WiFi_Reconnect();
  }

#if MQTT_SUPPORTED == 1
  if (wifiState == WL_CONNECTED) {
    if (shineMqtt.mqttReconnect()) {
      shineMqtt.loop();
    }
  }
#endif

  httpServer.handleClient();


#if MODBUS_TCP_SUPPORTED == 1
  if (modbusTCP.isEnabled()) {
    modbusTCP.loop();
  }
#endif




  if ((now - LEDTimer) > LED_TIMER) {
    if (wifiState == WL_CONNECTED)
      digitalWrite(LED_GN, !digitalRead(LED_GN));
    else
      digitalWrite(LED_GN, 0);

    LEDTimer = now;
  }



  if ((now - WifiRetryTimer) > WIFI_RETRY_TIMER) {
    if (stick == Undef_stick) InverterReconnect();
    WifiRetryTimer = now;
  }



  if ((now - RefreshTimer) > REFRESH_TIMER) {
    if (stick != Undef_stick) {
#if SIMULATE_INVERTER == 1
      readoutSucceeded = true;
#else
      readoutSucceeded = false;
      readoutSucceeded = Inverter.ReadData(NUM_OF_RETRIES);
      updateRedLed();
#endif
      if (readoutSucceeded) {


#if MQTT_SUPPORTED == 1
        if (shineMqtt.mqttEnabled()) {
          sendMqttJson();
        }
#endif
      } else {
#if MQTT_SUPPORTED == 1
        StaticJsonDocument<64> doc;
        doc["InverterStatus"] = -1;
        shineMqtt.mqttPublish(doc);

#endif
      }
    }
    updateRedLed();

#if PINGER_SUPPORTED == 1

    if (pinger.Ping(GATEWAY_IP) == false) {
      digitalWrite(LED_RT, 1);
      delay(3000);
      ESP.restart();
    }
#endif

    RefreshTimer = now;
#if defined(ESP32)
    esp_task_wdt_reset();
#endif
  }

#if defined(DEFAULT_NTP_SERVER) && defined(DEFAULT_TZ_INFO)

  if (!initialSyncDone && now > 60000) {
    handleNTPSync();
    lastSync = now;
    initialSyncDone = true;
  }

  if (initialSyncDone && (now - lastSync) > NTP_TIMER) {
    handleNTPSync();
    lastSync = now;
  }
#endif

#if OTA_SUPPORTED == 1

  ArduinoOTA.handle();
#else
#ifndef ESP32

  MDNS.update();
#endif
#endif

  if (User.bat_standby) {
    if ((now - BatteryStandbyTimer) > BATTERY_STANDBY_TIMER) {
      batteryStandby();
      BatteryStandbyTimer = now;
    }
  }

  if (User.accharge) {
    if ((now - ACChargeControlTimer) > ACCHARGE_CONTROL_TIMER) {
      acchargeControl();
      ACChargeControlTimer = now;
    }
  }
}