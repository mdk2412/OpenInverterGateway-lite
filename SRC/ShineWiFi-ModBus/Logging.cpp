#include "Logging.h"
#include <TLog.h>

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

void configureLogging(const String& syslogIp) {
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

  if (!syslogIp.isEmpty()) {
    syslogStream.setDestination(syslogIp.c_str());
    const std::shared_ptr<LOGBase> syslogStreamPtr =
        std::make_shared<SyslogStream>(syslogStream);
    Log.addPrintStream(syslogStreamPtr);
    Log.printf("Syslog Server IP: %s\n", syslogIp.c_str());
  }
}