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

// Nativer Event-Handler für Disconnects
WiFiEventHandler disconnectHandler;

static void onStationModeDisconnected(const WiFiEventStationModeDisconnected& event) {
  if (!wasDisconnected) {
    wasDisconnected = true;
    disconnectedStart = millis();
    Log.printf(PSTR("WiFi disconnected! Reason: %d. Native stack handling reconnect...\n"), event.reason);
  }
}

// Interne Menü-Konfiguration für den WiFiManager (bleibt erhalten)
static void setupMenu(WiFiManager& wm) {
  Log.println(F("Setting up WiFiManager menu"));
  std::vector<const char*> menu = {"wifi", "wifinoscan", "update"};
  menu.push_back("sep");
  menu.push_back("erase");
  menu.push_back("restart");

  wm.setMenu(menu);
}

// Zentrale Netzwerk-Initialisierung
void setupShineWifi(WiFiManager& wm) {
  // 1. Native Stack Grundeinstellungen vornehmen
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true); // Nativen Auto-Reconnect des ESP8266 aktivieren
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  // 2. Nativen Event-Handler registrieren (läuft direkt im ESP-SDK)
#ifdef ESP8266
  disconnectHandler = WiFi.onStationModeDisconnected(onStationModeDisconnected);
#endif

  WiFi.hostname(User.hostname.c_str());
#if OTA_SUPPORTED == 0 && defined(ESP8266)
  MDNS.begin(User.hostname.c_str());
#endif

  // 3. WiFiManager-spezifische Konfiguration für das Portal
  setupMenu(wm);
  wm.setHostname(User.hostname.c_str());
  wm.setConfigPortalTimeout(CONFIG_PORTAL_MAX_TIME_SECONDS);

  Log.printf("Setup ShineWiFi Host: hostname %s\n", User.hostname.c_str());
}

// Überwachung (reduziert auf Watchdog/Hard-Reset, da nativer Stack den Reconnect macht)
void WiFi_Reconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long currentMillis = millis();

    // Harter Neustart nach 5 Minuten (300.000 ms) ohne Verbindung
    if (wasDisconnected && (currentMillis - disconnectedStart > 300000)) {
      Log.println(F("WiFi Reconnect timed out (5 minutes). Rebooting ESP8266..."));
      ESP.restart();
    }
    
    // Hinweis: Kein manuelles WiFi.reconnect() nötig, 
    // da WiFi.setAutoReconnect(true) und der native Stack das im Hintergrund übernehmen.
    return;
  }

  // Verbindung erfolgreich wiederhergestellt
  if (wasDisconnected) {
    wasDisconnected = false;
    Log.printf(PSTR("WiFi reconnected | Local IP: %s | RSSI: %d dBm\n"),
               WiFi.localIP().toString().c_str(), WiFi.RSSI());
  }
}