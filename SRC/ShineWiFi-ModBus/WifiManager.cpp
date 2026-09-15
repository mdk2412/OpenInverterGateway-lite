#include "WifiManager.h"
#include "UserConfig.h"
#include <TLog.h>

// --- Setup WiFi Host ---
void setupWifiHost() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  disconnectHandler = WiFi.onStationModeDisconnected(onStationModeDisconnected);

  WiFi.hostname(User.hostname);
#if OTA_SUPPORTED == 0
  MDNS.begin(User.hostname);
#endif
  Log.printf("Setup WiFi Host: hostname %s\n", User.hostname.c_str());
}

void setupMenu(WiFiManager& wm) {
  Log.println(F("Setting up WiFiManager menu"));
  std::vector<const char*> menu = {"wifi", "wifinoscan", "update"};
  menu.push_back("sep");
  menu.push_back("erase");
  menu.push_back("restart");

  wm.setMenu(menu);
}

void setupWifiManagerConfigMenu(WiFiManager& wm) {
  setupMenu(wm);
}