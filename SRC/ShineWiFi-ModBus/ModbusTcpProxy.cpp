#include "ModbusTcpProxy.h"
#include <TLog.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

  static WiFiServer modbusServer(MODBUS_TCP_PORT);
  bool modbusTcpProxyRunning = (MODBUS_TCP_PROXY == 1);

extern Growatt Inverter;

void ModbusTcpProxySetup() {
  if (modbusTcpProxyRunning) {
    modbusServer.begin();
    Log.print(F("Modbus TCP proxy started on port "));
    Log.println(MODBUS_TCP_PORT);
  }
}

static void sendException(WiFiClient &client, uint16_t transId, uint8_t unitId,
                          uint8_t function, uint8_t code) {
  uint8_t resp[9];
  resp[0] = transId >> 8;
  resp[1] = transId & 0xFF;
  resp[2] = 0;  // protocol
  resp[3] = 0;
  resp[4] = 0;
  resp[5] = 3;  // len
  resp[6] = unitId;
  resp[7] = function | 0x80;
  resp[8] = code;
  client.write(resp, 9);
}

static void handleClient(WiFiClient &client) {
  Log.print(F("Modbus request from "));
  Log.println(client.remoteIP());
  if (client.available() < 8) return;

  uint8_t header[7];
  client.readBytes(header, 7);
  uint16_t transId = (header[0] << 8) | header[1];
  uint16_t length = (header[4] << 8) | header[5];
  uint8_t unitId = header[6];

  if (length == 0 || client.available() < length) return;

  uint8_t pdu[256];
  if (length > sizeof(pdu)) {
    sendException(client, transId, unitId, 0, 3);
    return;
  }
  client.readBytes(pdu, length);
  uint8_t function = pdu[0];

  if (function == 3 || function == 4) {
    Log.printf("Function %d start %u count %u\n", function,
               (pdu[1] << 8) | pdu[2], (pdu[3] << 8) | pdu[4]);
    uint16_t start = (pdu[1] << 8) | pdu[2];
    uint16_t count = (pdu[3] << 8) | pdu[4];
    if (count == 0 || count > 125) {
      sendException(client, transId, unitId, function, 3);
      return;
    }
    uint16_t values[125];
    bool ok = false;
    if (function == 3) {
      ok = Inverter.ReadHoldingRegFrag(start, count, values);
    } else {
      ok = Inverter.ReadInputRegFrag(start, count, values);
    }
    if (!ok) {
      sendException(client, transId, unitId, function, 4);
      return;
    }
    uint8_t resp[260];
    uint16_t respLen = 9 + count * 2;
    resp[0] = transId >> 8;
    resp[1] = transId & 0xFF;
    resp[2] = 0;
    resp[3] = 0;
    resp[4] = (respLen - 6) >> 8;
    resp[5] = (respLen - 6) & 0xFF;
    resp[6] = unitId;
    resp[7] = function;
    resp[8] = count * 2;
    for (uint16_t i = 0; i < count; ++i) {
      resp[9 + i * 2] = values[i] >> 8;
      resp[10 + i * 2] = values[i] & 0xFF;
    }
    client.write(resp, respLen);
  } else if (function == 6) {
    uint16_t adr = (pdu[1] << 8) | pdu[2];
    uint16_t val = (pdu[3] << 8) | pdu[4];
    if (!Inverter.WriteHoldingReg(adr, val)) {
      sendException(client, transId, unitId, function, 4);
      return;
    }
    uint8_t resp[12];
    resp[0] = transId >> 8;
    resp[1] = transId & 0xFF;
    resp[2] = 0;
    resp[3] = 0;
    resp[4] = 0;
    resp[5] = 6;
    resp[6] = unitId;
    resp[7] = function;
    resp[8] = pdu[1];
    resp[9] = pdu[2];
    resp[10] = pdu[3];
    resp[11] = pdu[4];
    client.write(resp, 12);
  } else {
    sendException(client, transId, unitId, function, 1);
  }
}

void ModbusTcpProxyLoop() {
  if (!modbusTcpProxyRunning) return;
  WiFiClient client = modbusServer.available();
  if (client) {
    Log.print(F("Modbus client connected: "));
    Log.println(client.remoteIP());
    handleClient(client);
    if (!client.connected()) {
      client.stop();
      Log.println(F("Modbus client disconnected"));
    }
  }
}

void ModbusTcpProxyStart() {
  if (!modbusTcpProxyRunning) {
    modbusServer.begin();
    modbusTcpProxyRunning = true;
    Log.print(F("Modbus TCP proxy started on port "));
    Log.println(MODBUS_TCP_PORT);
  }
}

void ModbusTcpProxyStop() {
  if (modbusTcpProxyRunning) {
    modbusServer.close();
    modbusTcpProxyRunning = false;
    Log.println(F("Modbus TCP proxy stopped"));
  }
}

bool ModbusTcpProxyIsRunning() { return modbusTcpProxyRunning; }
