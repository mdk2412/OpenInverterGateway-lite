#include "WifiManager.h"
#include <TLog.h>

// Global Wifi Instanz
WifiConfig Wifi;

// --- Interne Datenstrukturen ---
static struct {
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
  const char* hostname       = "hostname";
  const char* static_ip      = "staticip";
  const char* static_netmask = "staticnetmask";
  const char* static_gateway = "staticgateway";
  const char* static_dns     = "staticdns";
#if MQTT_SUPPORTED == 1
  const char* mqtt_server    = "mqtts";
  const char* mqtt_port      = "mqttp";
  const char* mqtt_topic     = "mqttt";
  const char* mqtt_user      = "mqttu";
  const char* mqtt_pwd       = "mqttw";
#endif
  const char* syslog_ip      = "syslogip";
  const char* force_ap       = "forceap";
} ConfigFiles;

// --- Config Load / Save ---
void loadConfig() {
  Preferences p;
  p.begin("ShineWiFi", true); // Read-only

  Wifi.hostname       = p.getString(ConfigFiles.hostname, DEFAULT_HOSTNAME);
  Wifi.static_ip      = p.getString(ConfigFiles.static_ip, "");
  Wifi.static_netmask = p.getString(ConfigFiles.static_netmask, "");
  Wifi.static_gateway = p.getString(ConfigFiles.static_gateway, "");
  Wifi.static_dns     = p.getString(ConfigFiles.static_dns, "");

#if MQTT_SUPPORTED == 1
  Wifi.mqtt.server    = p.getString(ConfigFiles.mqtt_server, "");
  Wifi.mqtt.port      = p.getString(ConfigFiles.mqtt_port, "1883");
  Wifi.mqtt.topic     = p.getString(ConfigFiles.mqtt_topic, "");
  Wifi.mqtt.user      = p.getString(ConfigFiles.mqtt_user, "");
  Wifi.mqtt.pwd       = p.getString(ConfigFiles.mqtt_pwd, "");
#endif

  Wifi.syslog_ip      = p.getString(ConfigFiles.syslog_ip, "");
  Wifi.force_ap       = p.getBool(ConfigFiles.force_ap, false);

  p.end();
}

void saveConfig() {
  Preferences p;
  p.begin("ShineWiFi", false); // Read-Write

  p.putString(ConfigFiles.hostname, Wifi.hostname);
  p.putString(ConfigFiles.static_ip, Wifi.static_ip);
  p.putString(ConfigFiles.static_netmask, Wifi.static_netmask);
  p.putString(ConfigFiles.static_gateway, Wifi.static_gateway);
  p.putString(ConfigFiles.static_dns, Wifi.static_dns);

#if MQTT_SUPPORTED == 1
  p.putString(ConfigFiles.mqtt_server, Wifi.mqtt.server);
  p.putString(ConfigFiles.mqtt_port, Wifi.mqtt.port);
  p.putString(ConfigFiles.mqtt_topic, Wifi.mqtt.topic);
  p.putString(ConfigFiles.mqtt_user, Wifi.mqtt.user);
  p.putString(ConfigFiles.mqtt_pwd, Wifi.mqtt.pwd);
#endif

  p.putString(ConfigFiles.syslog_ip, Wifi.syslog_ip);
  p.putBool(ConfigFiles.force_ap, Wifi.force_ap);

  p.end();
}

// --- Setup WiFi Host ---
void setupWifiHost() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  disconnectHandler = WiFi.onStationModeDisconnected(onStationModeDisconnected);

  WiFi.hostname(Wifi.hostname);
#if OTA_SUPPORTED == 0
  MDNS.begin(Wifi.hostname);
#endif
  Log.printf("Setup WiFi Host: hostname %s\n", Wifi.hostname.c_str());
}

// --- Callbacks & WiFiManager Configuration ---
void saveParamCallback() {
  Log.println(F("[CALLBACK] saveParamCallback fired"));

  if (customWMParams.hostname == nullptr) return; // Null-Pointer Guard

  Wifi.hostname       = customWMParams.hostname->getValue();
  Wifi.static_ip      = customWMParams.static_ip->getValue();
  Wifi.static_netmask = customWMParams.static_netmask->getValue();
  Wifi.static_gateway = customWMParams.static_gateway->getValue();
  Wifi.static_dns     = customWMParams.static_dns->getValue();

#if MQTT_SUPPORTED == 1
  Wifi.mqtt.server    = customWMParams.mqtt_server->getValue();
  Wifi.mqtt.port      = customWMParams.mqtt_port->getValue();
  Wifi.mqtt.topic     = customWMParams.mqtt_topic->getValue();
  Wifi.mqtt.user      = customWMParams.mqtt_user->getValue();
  Wifi.mqtt.pwd       = customWMParams.mqtt_pwd->getValue();
#endif

  Wifi.syslog_ip      = customWMParams.syslog_ip->getValue();

  saveConfig();

  Log.println(F("[CALLBACK] saveParamCallback complete"));
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