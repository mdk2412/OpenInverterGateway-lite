#ifndef _MODBUS_TCP_H_
#define _MODBUS_TCP_H_

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

class ModbusTCP {
 private:
  uint16_t port;       // 1
  WiFiServer* server;  // 2
  WiFiClient client;   // 3
  bool enabled;        // 4
  // Modbus function codes
  static const uint8_t FC_READ_HOLDING_REGISTERS = 0x03;
  static const uint8_t FC_READ_INPUT_REGISTERS = 0x04;
  static const uint8_t FC_WRITE_SINGLE_REGISTER = 0x06;

  // Exception codes
  static const uint8_t EX_ILLEGAL_FUNCTION = 0x01;
  static const uint8_t EX_ILLEGAL_DATA_ADDRESS = 0x02;
  static const uint8_t EX_ILLEGAL_DATA_VALUE = 0x03;

  uint8_t requestBuffer[260];
  uint8_t responseBuffer[260];

  // Neue Signaturen passend zur CPP
  void processRequest(uint16_t transactionId, uint8_t unitId,
                      uint8_t functionCode, uint16_t pduLength);

  void sendException(uint16_t transactionId, uint8_t unitId,
                     uint8_t functionCode, uint8_t exceptionCode);

 public:
  ModbusTCP(uint16_t serverPort = 502);
  ~ModbusTCP();

  void begin();
  void loop();
  void stop();

  bool isEnabled() { return enabled; }

  // Callback functions - implemented in main code
  bool (*readHoldingRegister)(uint16_t address, uint16_t* value) = nullptr;
  bool (*readInputRegister)(uint16_t address, uint16_t* value) = nullptr;
  bool (*writeHoldingRegister)(uint16_t address, uint16_t value) = nullptr;
};

#endif
