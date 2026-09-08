#ifndef _SHINE_LOGGING_H_
#define _SHINE_LOGGING_H_

#include <Arduino.h>
#include "Config.h"

// Forward-Declarations externer Lib-Klassen
class SyslogStream;

#ifdef ENABLE_TELNET_DEBUG
class TelnetSerialStream;
extern TelnetSerialStream telnetSerialStream;
#endif

#ifdef ENABLE_WEB_DEBUG
class WebSerialStream;
extern WebSerialStream webSerialStream;
#endif

extern SyslogStream syslogStream;

/**
 * Initialisiert und konfiguriert alle aktivierten Log-Streams.
 * @param syslogIp IP-Adresse des Syslog-Servers (optional)
 */
void configureLogging(const String& syslogIp = "");

#endif // _SHINE_LOGGING_H_