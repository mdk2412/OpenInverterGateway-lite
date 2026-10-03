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
static uint8_t lastDisconnectReason = 0;  // Initial 0 (kein Grund)

// Nativer Event-Handler für Disconnects
WiFiEventHandler disconnectHandler;

// Nativer Event-Handler Callback (läuft direkt im ESP-SDK Context)
static void onStationModeDisconnected(
    const WiFiEventStationModeDisconnected& event) {
  if (!wasDisconnected) {
    wasDisconnected = true;
    disconnectedStart = millis();
    lastDisconnectReason = event.reason;  // Grund sichern
  }
}

// Helper-Funktion: Übersetzt die ESP8266 WiFi-Reason-ID in Klartext
const char* getWiFiReasonText(uint8_t reason) {
  switch (reason) {
    case 1:
      return "UNSPECIFIED";
    case 2:
      return "AUTH_EXPIRE";
    case 3:
      return "AUTH_LEAVE";
    case 4:
      return "ASSOC_EXPIRE";
    case 5:
      return "ASSOC_TOOMANY";
    case 6:
      return "NOT_AUTHED";
    case 7:
      return "NOT_ASSOCED";
    case 8:
      return "ASSOC_LEAVE";
    case 9:
      return "ASSOC_NOT_AUTHED";
    case 10:
      return "DISASSOC_PWRCAP_BAD";
    case 11:
      return "DISASSOC_SUPCHAN_BAD";
    case 15:
      return "4WAY_HANDSHAKE_TIMEOUT";
    case 16:
      return "GROUP_KEY_UPDATE_TIMEOUT";
    case 17:
      return "IE_IN_4WAY_DIFFERS";
    case 18:
      return "GROUP_CIPHER_INVALID";
    case 19:
      return "PAIRWISE_CIPHER_INVALID";
    case 20:
      return "AKMP_INVALID";
    case 21:
      return "UNSUPP_RSN_IE_VER";
    case 22:
      return "INVALID_RSN_IE_CAP";
    case 23:
      return "802_1X_AUTH_FAILED";
    case 24:
      return "CIPHER_SUITE_REJECTED";
    case 200:
      return "BEACON_TIMEOUT";
    case 201:
      return "NO_AP_FOUND";
    case 202:
      return "AUTH_FAIL";
    case 203:
      return "ASSOC_FAIL";
    case 204:
      return "HANDSHAKE_TIMEOUT";
    default:
      return "UNKNOWN";
  }
}

// Interne Menü-Konfiguration für den WiFiManager
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
  WiFi.setAutoReconnect(true);  // Nativen Auto-Reconnect des ESP8266 aktivieren
  WiFi.setSleepMode(
      WIFI_NONE_SLEEP);  // Verhindert Sleep-Probleme beim AP-Rekeying

  // Verhindert das dauerhafte Schreiben/Cachen ungültiger Session-Tokens im
  // Flash, was bei WPA2/WPA3-Transition-Mode oft zu Reason 6 führt.
  WiFi.persistent(false);

  // 2. Nativen Event-Handler registrieren
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

  Log.printf(PSTR("Setup ShineWiFi Host: hostname %s\n"),
             User.hostname.c_str());
}

// Überwachung & Logging nach Wiederverbindung
void WiFi_Reconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long currentMillis = millis();

    // Harter Neustart nach 5 Minuten (300.000 ms) ohne Verbindung
    if (wasDisconnected && (currentMillis - disconnectedStart > 300000)) {
      Log.println(F("WiFi Reconnect timed out (5 Minutes). Rebooting..."));
      ESP.restart();
    }

    return;
  }

  // Verbindung erfolgreich wiederhergestellt
  if (wasDisconnected) {
    wasDisconnected = false;

    // Nur Grund ausgeben, wenn er beim Disconnect auch tatsächlich gesetzt
    // wurde
    if (lastDisconnectReason > 0) {
      Log.printf(PSTR("WiFi reconnected | Reason: %d (%s)\n"),
                 lastDisconnectReason, getWiFiReasonText(lastDisconnectReason));
    } else {
      Log.printf(PSTR("WiFi reconnected\n"));
    }

    // Grund für das nächste Event zurücksetzen
    lastDisconnectReason = 0;
  }
}