/* Copyright 2008, 2012-2022 Dirk-Willem van Gulik <dirkx(at)webweaving(dot)org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "TLog.h"
#include "WebSerialStream.h"

#if (defined(ESP32) || defined(ESP8266))

size_t WebSerialStream::write(uint8_t c) {
  _buff[_at % sizeof(_buff)] = c;
  _at++;
  return 1;
}

WebSerialStream::~WebSerialStream() {
  stop();
}

void WebSerialStream::begin() {
  if (_server != NULL)
    return;

  _server = new WebServer(_webPort);

  _server->on("/", [this]() {
    _server->send(200, "text/html",
      "<html><head><title>log</title></head>"
      "<style>"
      "#log { font-family: 'Courier New', monospace; white-space: pre; }"
      "</style>"
      "<script>"
      "var at = 0;"
      "function st() { setTimeout(f, 750); };"
      "function f() {"
      "  fetch('log?at=' + at)"
      "    .then(r => r.json())"
      "    .then(j => {"
      "      var isAtEnd = (window.innerHeight + window.pageYOffset) >= document.body.offsetHeight - 4;"
      "      document.getElementById('log').innerHTML += j.buff;"
      "      at = j.at;"
      "      if (isAtEnd) window.scrollTo(0, document.body.scrollHeight);"
      "      st();"
      "    })"
      "    .catch(e => { console.log(e); st(); });"
      "}"
      "window.onload = f;"
      "</script>"
      "<body><div id=log></div></body></html>"
    );
  });

  _server->on("/log", [this]() {
    if (!_server->hasArg("at")) {
      _server->send(400, "text/plain", "Missing at argument.");
      return;
    }

    unsigned long prevAt = _server->arg("at").toInt();
    String out = "{\"at\":" + String(_at) + ",\"buff\":\"";

    if (prevAt > _at) {
      out += "<font color=red><hr><i>.. log reset..</i></font><hr>";
      prevAt = _at;
    }

    if (_at > sizeof(_buff) && prevAt < _at - sizeof(_buff)) {
      out += "<font color=red><hr><i>.. skipping " +
             String(_at - sizeof(_buff) - prevAt) +
             " bytes of log - no longer in buffer ..</i><hr></font>";
      prevAt = _at - sizeof(_buff);
    }

    for (; prevAt != _at; prevAt++) {
      char c = _buff[prevAt % sizeof(_buff)];
      switch (c) {
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '\b': out += "\\b"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\f': out += "\\f"; break;
        case '\t': out += "\\t"; break;
        case '"' : out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        default  : out += c; break;
      }
    }

    out += "\"}";
    _server->send(200, "application/json", out);
  });

  _server->begin();

  MDNS.addService("http", "tcp", _webPort);
}

void WebSerialStream::stop() {
  if (!_server)
    return;
  _server->stop();
  delete _server;
  _server = NULL;
}

void WebSerialStream::loop() {
  if (_server)
    _server->handleClient();

  static bool ipLogged = false;

  if (!ipLogged && WiFi.status() == WL_CONNECTED) {
    ipLogged = true;
    Log.print(F("Opened Serial Web Server: http://"));
    Log.print(WiFi.localIP());
    Log.print(F(":"));
    Log.println(_webPort);
  }
}

#endif
