#include "ModbusTCP.h"
#include <TLog.h>

ModbusTCP::ModbusTCP(uint16_t serverPort)
    : port(serverPort), server(nullptr), client(), enabled(false) {}

ModbusTCP::~ModbusTCP() { stop(); }

void ModbusTCP::begin() {
  if (server == nullptr) {
    server = new WiFiServer(port);
    server->setNoDelay(true);
  }

  server->begin();
  enabled = true;

  Log.print(F("Modbus TCP Server started on Port "));
  Log.println(port);
}

void ModbusTCP::stop() {
  if (server != nullptr) {
    server->stop();
    delete server;
    server = nullptr;
  }
  if (client) {
    client.stop();
  }
  enabled = false;
}

void ModbusTCP::loop() {
  if (!enabled || server == nullptr) return;

  // Accept new client if needed
  if (!client || !client.connected()) {
    client = server->accept();
    if (!client) return;
  }

  // MBAP header = 7 bytes
  if (client.available() < 7) return;

  // Read MBAP header
  uint8_t mbap[7];
  if (client.read(mbap, 7) != 7) return;

  uint16_t transactionId = (mbap[0] << 8) | mbap[1];
  uint16_t protocolId = (mbap[2] << 8) | mbap[3];
  uint16_t lengthField = (mbap[4] << 8) | mbap[5];
  uint8_t unitId = mbap[6];

  // Validate protocol ID
  if (protocolId != 0) {
    client.stop();
    return;
  }

  // LengthField includes UnitID + PDU
  if (lengthField < 2 || lengthField > 253) {
    client.stop();
    return;
  }

  // Wait for full PDU
  if (client.available() < (lengthField - 1)) return;

  // Read PDU
  uint16_t pduLength = lengthField - 1;
  if (pduLength > sizeof(requestBuffer)) {
    client.stop();
    return;
  }

  if (client.read(requestBuffer, pduLength) != pduLength) return;

  // Extract function code
  uint8_t functionCode = requestBuffer[0];

  // Process request
  processRequest(transactionId, unitId, functionCode, pduLength);
}

void ModbusTCP::processRequest(uint16_t transactionId, uint8_t unitId,
                               uint8_t functionCode, uint16_t pduLength) {
  // Build MBAP header
  responseBuffer[0] = transactionId >> 8;
  responseBuffer[1] = transactionId & 0xFF;
  responseBuffer[2] = 0;
  responseBuffer[3] = 0;

  responseBuffer[6] = unitId;
  responseBuffer[7] = functionCode;

  uint16_t totalLength = 0;

  switch (functionCode) {
    case FC_READ_HOLDING_REGISTERS:
    case FC_READ_INPUT_REGISTERS: {
      if (pduLength < 5) {
        sendException(transactionId, unitId, functionCode,
                      EX_ILLEGAL_DATA_VALUE);
        return;
      }

      uint16_t startAddress = (requestBuffer[1] << 8) | requestBuffer[2];
      uint16_t quantity = (requestBuffer[3] << 8) | requestBuffer[4];

      if (quantity < 1 || quantity > 125) {
        sendException(transactionId, unitId, functionCode,
                      EX_ILLEGAL_DATA_VALUE);
        return;
      }

      uint8_t byteCount = quantity * 2;
      responseBuffer[8] = byteCount;

      for (uint16_t i = 0; i < quantity; i++) {
        uint16_t value = 0;
        bool ok = false;

        if (functionCode == FC_READ_HOLDING_REGISTERS && readHoldingRegister)
          ok = readHoldingRegister(startAddress + i, &value);
        else if (functionCode == FC_READ_INPUT_REGISTERS && readInputRegister)
          ok = readInputRegister(startAddress + i, &value);

        if (!ok) {
          sendException(transactionId, unitId, functionCode,
                        EX_ILLEGAL_DATA_ADDRESS);
          return;
        }

        responseBuffer[9 + i * 2] = value >> 8;
        responseBuffer[10 + i * 2] = value & 0xFF;
      }

      uint16_t pduLen = 2 + 1 + byteCount;  // FC + ByteCount + Data
      uint16_t mbapLen = 1 + pduLen;        // UnitID + PDU
      totalLength = 6 + mbapLen;

      responseBuffer[4] = mbapLen >> 8;
      responseBuffer[5] = mbapLen & 0xFF;

      client.write(responseBuffer, totalLength);
      client.flush();
      return;
    }

    case FC_WRITE_SINGLE_REGISTER: {
      if (pduLength < 5) {
        sendException(transactionId, unitId, functionCode,
                      EX_ILLEGAL_DATA_VALUE);
        return;
      }

      uint16_t address = (requestBuffer[1] << 8) | requestBuffer[2];
      uint16_t value = (requestBuffer[3] << 8) | requestBuffer[4];

      if (!writeHoldingRegister || !writeHoldingRegister(address, value)) {
        sendException(transactionId, unitId, functionCode,
                      EX_ILLEGAL_DATA_ADDRESS);
        return;
      }

      // Echo request
      responseBuffer[8] = requestBuffer[1];
      responseBuffer[9] = requestBuffer[2];
      responseBuffer[10] = requestBuffer[3];
      responseBuffer[11] = requestBuffer[4];

      uint16_t pduLen = 5;  // FC + Address(2) + Value(2)
      uint16_t mbapLen = 1 + pduLen;
      totalLength = 6 + mbapLen;

      responseBuffer[4] = mbapLen >> 8;
      responseBuffer[5] = mbapLen & 0xFF;

      client.write(responseBuffer, totalLength);
      client.flush();
      return;
    }

    default:
      sendException(transactionId, unitId, functionCode, EX_ILLEGAL_FUNCTION);
      return;
  }
}

void ModbusTCP::sendException(uint16_t transactionId, uint8_t unitId,
                              uint8_t functionCode, uint8_t exceptionCode) {
  responseBuffer[0] = transactionId >> 8;
  responseBuffer[1] = transactionId & 0xFF;
  responseBuffer[2] = 0;
  responseBuffer[3] = 0;

  responseBuffer[6] = unitId;
  responseBuffer[7] = functionCode | 0x80;
  responseBuffer[8] = exceptionCode;

  uint16_t mbapLen = 3;  // UnitID + FC + Exception
  uint16_t totalLength = 6 + mbapLen;

  responseBuffer[4] = 0;
  responseBuffer[5] = 3;

  client.write(responseBuffer, totalLength);
  client.flush();
}
