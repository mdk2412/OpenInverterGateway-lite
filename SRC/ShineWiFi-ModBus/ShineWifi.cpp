#include "ShineWifi.h"

bool ShineWifiDisconnect() {
#if defined(ESP8266) || defined(ESP32)

#ifdef WM_DEBUG_LEVEL
  Log.print(F("WiFi station disconnected"));
#endif

  // Verbindung sauber trennen
  bool ret = WiFi.disconnect(true);   // true = persistent (ESP8266), ignoriert auf ESP32

  // WLAN komplett ausschalten
  WiFi.mode(WIFI_OFF);

  return ret;

#else
  return false;
#endif
}
