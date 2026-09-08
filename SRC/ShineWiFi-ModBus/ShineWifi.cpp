#include "ShineWifi.h"
#include <TLog.h>

// Globale Instanz des WiFi-Clients
#ifdef MQTTS_ENABLED
WiFiClientSecure espClient;
#else
WiFiClient espClient;
#endif

// Statische Variablen für die Reconnect-Überwachung
static bool wasDisconnected = false;
static unsigned long disconnectedStart = 0;

bool ShineWifiDisconnect() {
#ifdef WM_DEBUG_LEVEL
  Log.print(F("WiFi station disconnected"));
#endif

  // Verbindung sauber trennen
  bool ret = WiFi.disconnect(true);   // true = persistent (ESP8266), ignoriert auf ESP32

  // WLAN komplett ausschalten
  WiFi.mode(WIFI_OFF);

  return ret;
}

void onStationModeDisconnected(const WiFiEventStationModeDisconnected& event) {
  if (!wasDisconnected) {
    wasDisconnected = true;
    disconnectedStart = millis();
    Log.printf("WiFi disconnected! Reason: %d. Attempting Reconnect...\n", event.reason);
  }
}

void WiFi_Reconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    // Hard-Reset / Reboot falls WiFi nach 5 Minuten (300.000 ms) nicht wiederhergestellt ist
    if (wasDisconnected && (millis() - disconnectedStart > 300000)) { 
      Log.println(F("WiFi Reconnect timed out (5 minutes). Rebooting..."));
      ESP.restart();
    }
    return;
  }

  if (wasDisconnected) {
    wasDisconnected = false;
    Log.printf("WiFi reconnected | Local IP: %s | Hostname: %s\n",
               WiFi.localIP().toString().c_str(), WiFi.hostname().c_str());
  }
}