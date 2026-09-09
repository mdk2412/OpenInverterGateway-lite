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

void onStationModeDisconnected(const WiFiEventStationModeDisconnected& event) {
  if (!wasDisconnected) {
    wasDisconnected = true;
    disconnectedStart = millis();
    Log.printf("WiFi disconnected! Reason: %d. Attempting Reconnect...\n",
               event.reason);
  }
}

void WiFi_Reconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long currentMillis = millis();

    // 1. Hard-Reset nach 5 Minuten (300.000 ms)
    if (wasDisconnected && (currentMillis - disconnectedStart > 300000)) {
      Log.println(F("WiFi Reconnect timed out (5 minutes). Rebooting..."));
      ESP.restart();
    }

    // 2. Aktiver Reconnect-Versuch alle 60 Sekunden (60.000 ms)
    static unsigned long lastReconnectAttempt = 0;
    if (currentMillis - lastReconnectAttempt > 60000) {
      lastReconnectAttempt = currentMillis;
      Log.println(F("Active Reconnect attempt..."));
      WiFi.reconnect();  // Weist den ESP an, sich aktiv neu zu verbinden
    }

    return;
  }

  // 3. Wenn die Verbindung wiederhergestellt ist
  if (wasDisconnected) {
    wasDisconnected = false;
    Log.printf("WiFi reconnected | Local IP: %s | Hostname: %s\n",
               WiFi.localIP().toString().c_str(), WiFi.hostname().c_str());
  }
}