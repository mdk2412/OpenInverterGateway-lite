#include "Config.h"
#ifndef _SHINE_CONFIG_H_
#error Please rename Config.h.example to Config.h
#endif

#include "GrowattTypes.h"
#include "Growatt.h"

#if GROWATT_MODBUS_VERSION == 120
#include "Growatt120.h"
#elif GROWATT_MODBUS_VERSION == 124
#include "Growatt124.h"
#elif GROWATT_MODBUS_VERSION == 305
#include "Growatt305.h"
#elif GROWATT_MODBUS_VERSION == 3000
#include "GrowattTLXH.h"
#elif GROWATT_MODBUS_VERSION == 5000
#include "GrowattSPF.h"
#elif GROWATT_MODBUS_VERSION == 6000
#include "GrowattBP.h"
#else
#error "Unsupported Growatt Modbus Version"
#endif

#include <ModbusMaster.h>
#include <ArduinoJson.h>
#include <TLog.h>

ModbusMaster Modbus;

// Constructor
Growatt::Growatt() {
  _PacketCnt = 0;
  _PacketCntFailed = 0;

  handlers = std::map<String, CommandHandlerFunc>();

  // register default handlers
  RegisterCommand("echo", [this](const JsonDocument& req, JsonDocument& res,
                                 Growatt& inverter) {
    return handleEcho(req, res, *this);
  });

  RegisterCommand("list", [this](const JsonDocument& req, JsonDocument& res,
                                 Growatt& inverter) {
    return handleCommandList(req, res, *this);
  });

  // RegisterCommand("modbus/get", [this](const JsonDocument& req,
  //                                      JsonDocument& res, Growatt& inverter) {
  //   return handleModbusGet(req, res, *this);
  // });

  // RegisterCommand("modbus/set", [this](const JsonDocument& req,
  //                                      JsonDocument& res, Growatt& inverter) {
  //   return handleModbusSet(req, res, *this);
  // });
}

void Growatt::InitProtocol() {
/**
 * @brief Initialize the protocol struct
 * @param version The version of the modbus protocol to use
 */
#if GROWATT_MODBUS_VERSION == 120
  init_growatt120(_Protocol, *this);
#elif GROWATT_MODBUS_VERSION == 124
  init_growatt124(_Protocol, *this);
#elif GROWATT_MODBUS_VERSION == 305
  init_growatt305(_Protocol, *this);
#elif GROWATT_MODBUS_VERSION == 3000
  init_growattTLXH(_Protocol, *this);
#elif GROWATT_MODBUS_VERSION == 5000
  init_growattSPF(_Protocol, *this);
#elif GROWATT_MODBUS_VERSION == 6000
  init_growattBP(_Protocol, *this);
#else
#error "Unsupported Growatt Modbus Version"
#endif
}

void Growatt::begin(Stream& serial) {
  Serial.begin(115200);
  delay(1000);

  Serial.flush();
  while (Serial.available()) Serial.read();

  Modbus.begin(1, serial);
  Modbus.setResponseTimeout(100);
}

bool Growatt::ReadInputRegisters(uint8_t& i) {
  /**
   * @brief Read the input registers from the inverter
   * @returns true if data was read successfully, false otherwise
   */
  uint16_t registerAddress;
  uint8_t res;
  int j = 0;

  // read each fragment separately
  for (; i < _Protocol.InputFragmentCount; i++) {
#ifdef DEBUG_MODBUS_OUTPUT
    Log.printf("Modbus: Reading Segment from 0x%02X with Length %d ...",
               _Protocol.InputReadFragments[i].StartAddress,
               _Protocol.InputReadFragments[i].FragmentSize);
#endif
    res =
        Modbus.readInputRegisters(_Protocol.InputReadFragments[i].StartAddress,
                                  _Protocol.InputReadFragments[i].FragmentSize);
// for debug logging replace previous 3 lines with:
// uint32_t start = millis();
// res = Modbus.readInputRegisters(
//     _Protocol.InputReadFragments[i].StartAddress,
//     _Protocol.InputReadFragments[i].FragmentSize);
// uint32_t duration = millis() - start;
// Log.printf(
//     "[MODBUS][INPUT] Addr=0x%04X Len=%u Result=%s (%u) Time=%lu ms\n",
//     _Protocol.InputReadFragments[i].StartAddress,
//     _Protocol.InputReadFragments[i].FragmentSize,
//     (res == Modbus.ku8MBSuccess) ? "OK" : "FAIL",
//     res,
//     duration);
    if (res == Modbus.ku8MBSuccess) {
#ifdef DEBUG_MODBUS_OUTPUT
      Log.println(F("ok"));
#endif
      for (; j < _Protocol.InputRegisterCount; j++) {
        // make sure the register we try to read is in the fragment
        if (_Protocol.InputRegisters[j].address >=
            _Protocol.InputReadFragments[i].StartAddress) {
          // when we exceed the fragment size, skip to new fragment
          if (_Protocol.InputRegisters[j].address >=
              _Protocol.InputReadFragments[i].StartAddress +
                  _Protocol.InputReadFragments[i].FragmentSize)
            break;
          // let's say the register address is 1013 and read window is 1000-1050
          // that means the response in the buffer is on position 1013 - 1000 =
          // 13
          registerAddress = _Protocol.InputRegisters[j].address -
                            _Protocol.InputReadFragments[i].StartAddress;
          if (_Protocol.InputRegisters[j].size == SIZE_16BIT ||
              _Protocol.InputRegisters[j].size == SIZE_16BIT_S) {
            _Protocol.InputRegisters[j].value =
                Modbus.getResponseBuffer(registerAddress);
          } else {
            _Protocol.InputRegisters[j].value =
                (Modbus.getResponseBuffer(registerAddress) << 16) +
                Modbus.getResponseBuffer(registerAddress + 1);
          }
        }
      }
    } else {
#ifdef DEBUG_MODBUS_OUTPUT
      Log.println(F("failed"));
#endif
      Modbus.clearResponseBuffer();
      return false;
    }
  }
#if GROWATT_MODBUS_VERSION == 3000
  _Protocol.InputRegisters[P3000_INVERTER_STATUS].value &= 0xff;
  _Protocol.InputRegisters[P3000_INVERTER_RUNSTATE].value >>= 8;
  _Protocol.InputRegisters[P3000_BDC_SYSSTATE].value &= 0xff;
  _Protocol.InputRegisters[P3000_BDC_SYSMODE].value >>= 8;
#endif
  return true;
}

bool Growatt::ReadHoldingRegisters(uint8_t& i) {
  /**
   * @brief Read the holding registers from the inverter
   * @returns true if data was read successfully, false otherwise
   */
  uint16_t registerAddress;
  uint8_t res;
  int j = 0;

  // read each fragment separately
  for (; i < _Protocol.HoldingFragmentCount; i++) {
    res = Modbus.readHoldingRegisters(
        _Protocol.HoldingReadFragments[i].StartAddress,
        _Protocol.HoldingReadFragments[i].FragmentSize);
// for debug logging replace previous 3 lines with:
// uint32_t start = millis();
// res = Modbus.readHoldingRegisters(
//     _Protocol.HoldingReadFragments[i].StartAddress,
//     _Protocol.HoldingReadFragments[i].FragmentSize);
// uint32_t duration = millis() - start;
// Log.printf(
//     "[MODBUS][HOLDING] Addr=0x%04X Len=%u Result=%s (%u) Time=%lu ms\n",
//     _Protocol.HoldingReadFragments[i].StartAddress,
//     _Protocol.HoldingReadFragments[i].FragmentSize,
//     (res == Modbus.ku8MBSuccess) ? "OK" : "FAIL",
//     res,
//     duration);      
    if (res == Modbus.ku8MBSuccess) {
      for (; j < _Protocol.HoldingRegisterCount; j++) {
        if (_Protocol.HoldingRegisters[j].address >=
            _Protocol.HoldingReadFragments[i].StartAddress) {
          if (_Protocol.HoldingRegisters[j].address >=
              _Protocol.HoldingReadFragments[i].StartAddress +
                  _Protocol.HoldingReadFragments[i].FragmentSize)
            break;
          registerAddress = _Protocol.HoldingRegisters[j].address -
                            _Protocol.HoldingReadFragments[i].StartAddress;
          if (_Protocol.HoldingRegisters[j].size == SIZE_16BIT ||
              _Protocol.HoldingRegisters[j].size == SIZE_16BIT_S) {
            _Protocol.HoldingRegisters[j].value =
                Modbus.getResponseBuffer(registerAddress);
          } else {
            _Protocol.HoldingRegisters[j].value =
                (Modbus.getResponseBuffer(registerAddress) << 16) +
                Modbus.getResponseBuffer(registerAddress + 1);
          }
        }
      }
    } else {
      Modbus.clearResponseBuffer();
      return false;
    }
  }
  return true;
}

bool Growatt::ReadData(uint8_t maxRetries) {
  /**
   * @brief Reads the data from the inverter and updates the internal data
   * structures
   * @returns true if data was read successfully, false otherwise
   */
  uint8_t inputFragOffs = 0;
  uint8_t holdingFragOffs = 0;
  bool res;
  uint8_t retryCnt = 0;
  while (inputFragOffs < _Protocol.InputFragmentCount &&
         retryCnt < maxRetries) {
    res = ReadInputRegisters(inputFragOffs);
    if (res) {
      _PacketCnt++;  // nur bei Erfolg
    } else {
      _PacketCntFailed++;
      retryCnt++;
      Modbus.clearResponseBuffer();
    }
  }
  retryCnt = 0;
  while (holdingFragOffs < _Protocol.HoldingFragmentCount &&
         retryCnt < maxRetries) {
    res = ReadHoldingRegisters(holdingFragOffs);
    if (res) {
      _PacketCnt++;  // nur bei Erfolg
    } else {
      _PacketCntFailed++;
      retryCnt++;
      Modbus.clearResponseBuffer();
    }
  }

  bool inputOk = (inputFragOffs == _Protocol.InputFragmentCount);
  bool holdingOk = (holdingFragOffs == _Protocol.HoldingFragmentCount);

  _GotData = inputOk && holdingOk;

  if (!_GotData) {
    Log.println(F("Reading Modbus Data not successful!"));
  }

  return _GotData;
}

sGrowattModbusReg_t Growatt::GetInputRegister(uint16_t reg) {
  /**
   * @brief get the internal representation of the input register
   * @param reg the register to get
   * @returns the register value
   */
  if (_GotData == false) {
    ReadData(NUM_OF_RETRIES);
  }
  return _Protocol.InputRegisters[reg];
}

sGrowattModbusReg_t Growatt::GetHoldingRegister(uint16_t reg) {
  /**
   * @brief get the internal representation of the holding register
   * @param reg the register to get
   * @returns the register value
   */
  if (_GotData == false) {
    ReadData(1);
  }
  return _Protocol.HoldingRegisters[reg];
}

bool Growatt::ReadHoldingReg(uint16_t adr, uint16_t* result) {
/**
 * @brief read 16b holding register
 * @param adr address of the register
 * @param result pointer to the result
 * @returns true if successful
 */
  uint8_t res = Modbus.readHoldingRegisters(adr, 1);
  if (res == Modbus.ku8MBSuccess) {
    *result = Modbus.getResponseBuffer(0);
    return true;
  }
  return false;
}

bool Growatt::ReadHoldingReg(uint16_t adr, uint32_t* result) {
/**
 * @brief read 32b holding register
 * @param adr address of the register
 * @param result pointer to the result
 * @returns true if successful
 */
  uint8_t res = Modbus.readHoldingRegisters(adr, 2);
  if (res == Modbus.ku8MBSuccess) {
    *result = (Modbus.getResponseBuffer(0) << 16) + Modbus.getResponseBuffer(1);
    return true;
  }
  return false;
}

bool Growatt::ReadHoldingRegFrag(uint16_t adr, uint8_t size, uint16_t* result) {
  /**
   * @brief read 16b holding register fragment
   * @param adr address of the register
   * @param size size of the register
   * @param result pointer to the result
   * @returns true if successful
   */
  uint8_t res = Modbus.readHoldingRegisters(adr, size);
  if (res == Modbus.ku8MBSuccess) {
    for (int i = 0; i < size; i++) {
      result[i] = Modbus.getResponseBuffer(i);
    }
    return true;
  }
  return false;
}

bool Growatt::ReadHoldingRegFrag(uint16_t adr, uint8_t size, uint32_t* result) {
  /**
   * @brief read 32b holding register fragment
   * @param adr address of the register
   * @param size size of the register
   * @param result pointer to the result
   * @returns true if successful
   */
  uint8_t res = Modbus.readHoldingRegisters(adr, size * 2);
  if (res == Modbus.ku8MBSuccess) {
    for (int i = 0; i < size; i++) {
      result[i] = (Modbus.getResponseBuffer(i * 2) << 16) +
                  Modbus.getResponseBuffer(i * 2 + 1);
    }
    return true;
  }
  return false;
}

bool Growatt::WriteHoldingReg(uint16_t adr, uint16_t value) {
/**
 * @brief write 16b holding register
 * @param adr address of the register
 * @param value value to write to the register
 * @returns true if successful
 */
  uint8_t res = Modbus.writeSingleRegister(adr, value);
  if (res == Modbus.ku8MBSuccess) {
    return true;
  }
  return false;
}

bool Growatt::WriteHoldingRegFrag(uint16_t adr, uint8_t size, uint16_t* value) {
  /**
   * @brief write 16b holding register
   * @param adr address of the register
   * @param value value to write to the register
   * @param size size of the register
   * @returns true if successful
   */
  for (int i = 0; i < size; i++) {
    Modbus.setTransmitBuffer(i, value[i]);
  }
  uint8_t res = Modbus.writeMultipleRegisters(adr, size);
  if (res == Modbus.ku8MBSuccess) {
    return true;
  }
  return false;
}

bool Growatt::ReadInputReg(uint16_t adr, uint16_t* result) {
/**
 * @brief read 16b input register
 * @param adr address of the register
 * @param result pointer to the result
 * @returns true if successful
 */
  uint8_t res = Modbus.readInputRegisters(adr, 1);
  if (res == Modbus.ku8MBSuccess) {
    *result = Modbus.getResponseBuffer(0);
    return true;
  }
  return false;
}

bool Growatt::ReadInputReg(uint16_t adr, uint32_t* result) {
/**
 * @brief read 32b input register
 * @param adr address of the register
 * @param result pointer to the result
 * @returns true if successful
 */
  uint8_t res = Modbus.readInputRegisters(adr, 2);
  if (res == Modbus.ku8MBSuccess) {
    *result = (Modbus.getResponseBuffer(0) << 16) + Modbus.getResponseBuffer(1);
    return true;
  }
  return false;
}

double Growatt::roundByResolution(const double& value,
                                  const float& resolution) {
    double res = 1 / resolution;
    double v = value * res;
    return (v >= 0 ? int32_t(v + 0.5) : int32_t(v - 0.5)) / res;
}

double Growatt::getRegValue(sGrowattModbusReg_t* reg) {
  double result = 0;
  RegisterSize_t size = reg->size;
  const float& mult = reg->multiplier;
  const uint32_t& value = reg->value;
  const float& resolution = reg->resolution;

  switch (size) {
    case SIZE_16BIT_S:
      result = (mult == (int)mult)
                   ? (int16_t)value * mult
                   : roundByResolution((int16_t)value * mult, resolution);
      break;
    case SIZE_32BIT_S:
      result = (mult == (int)mult)
                   ? (int32_t)value * mult
                   : roundByResolution((int32_t)value * mult, resolution);
      break;
    default:
      result = (mult == (int)mult)
                   ? value * mult
                   : roundByResolution(value * mult, resolution);
  }
  return result;
}

bool Growatt::GetSingleValueByName(const String& name, double& value) {
  for (int i = 0; i < _Protocol.InputRegisterCount; i++) {
    if (name.equalsIgnoreCase(_Protocol.InputRegisters[i].name)) {
      value = getRegValue(&_Protocol.InputRegisters[i]);
      return true;
    }
  }
  for (int i = 0; i < _Protocol.HoldingRegisterCount; i++) {
    if (name.equalsIgnoreCase(_Protocol.HoldingRegisters[i].name)) {
      value = getRegValue(&_Protocol.HoldingRegisters[i]);
      return true;
    }
  }
  return false;
}

void Growatt::CreateJson(JsonDocument& doc, const String& MacAddress,
                         const String& Hostname) {
  if (!Hostname.isEmpty()) {
    doc["Hostname"] = Hostname;
  }
  for (int i = 0; i < _Protocol.InputRegisterCount; i++) {
    doc[_Protocol.InputRegisters[i].name] = getRegValue(&_Protocol.InputRegisters[i]);
  }

  for (int i = 0; i < _Protocol.HoldingRegisterCount; i++) {
    doc[_Protocol.HoldingRegisters[i].name] = getRegValue(&_Protocol.HoldingRegisters[i]);
  }

  doc["Mac"] = MacAddress;
  doc["Cnt"] = _PacketCnt;
  doc["CntFailed"] = _PacketCntFailed;
  doc["Uptime"] = millis() / 1000;
  doc["WifiRSSI"] = WiFi.RSSI();
  doc["HeapFree"] = ESP.getFreeHeap();
  
  static uint32_t heap_min_free = ESP.getFreeHeap();
  heap_min_free = (std::min)(ESP.getFreeHeap(), heap_min_free);
  
  doc["HeapMaxAlloc"] = ESP.getMaxFreeBlockSize();
  doc["HeapMinFree"] = heap_min_free;
  doc["HeapFragmentation"] = ESP.getHeapFragmentation();

  if (doc.overflowed()) {
    Log.println(F("CreateJson: JsonDocument overflowed! Output will be truncated."));
  }
}

void Growatt::CreateUIJson(JsonDocument& doc, const String& Hostname) {
  const char* unitStr[] = {"",   "W",  "kWh", "V",  "A",    "s",  "%",
                           "Hz", "°C", "VA",  "mA", "kOhm", "var"};

  const char* statusStr[] = {"(Waiting)", "(Normal Operation)", "", "(Error)"};
  const int statusStrLength = sizeof(statusStr) / sizeof(char*);
  const char* onoffStr[] = {"(Inverter Off)", "(Inverter On)", "(BDC Off)", "(BDC On)"};
  const int onoffStrLength = sizeof(onoffStr) / sizeof(char*);
  const char* priorityStr[] = {"(Load First)", "(Battery First)", "(Grid First)"};
  const int priorityStrLength = sizeof(priorityStr) / sizeof(char*);
  const char* bdcModeStr[] = {"(Idle)", "(Charging)", "(Discharging)"};
  const int bdcModeStrLength = sizeof(bdcModeStr) / sizeof(char*);

  if (!Hostname.isEmpty()) {
    // ArduinoJson v7 Syntax: .to<JsonArray>() statt .createNestedArray()
    JsonArray arr = doc["Hostname"].to<JsonArray>();
    arr.add(Hostname);
    arr.add("");
  }

  for (int i = 0; i < _Protocol.InputRegisterCount; i++) {
    if (_Protocol.InputRegisters[i].frontend) {
      // ArduinoJson v7 Syntax:
      JsonArray arr = doc[_Protocol.InputRegisters[i].name].to<JsonArray>();

      // Value
      arr.add(getRegValue(&_Protocol.InputRegisters[i]));

      const auto regVal = _Protocol.InputRegisters[i].value;
      const String regName = _Protocol.InputRegisters[i].name;

      if ((regName == F("InverterStatus") || regName == F("BDCSysState")) &&
          regVal < statusStrLength) {
        arr.add(statusStr[regVal]);
      } else if (regName == F("BDCSysMode") && regVal < bdcModeStrLength) {
        arr.add(bdcModeStr[regVal]);
      } else if (regName == F("Priority") && regVal < priorityStrLength) {
        arr.add(priorityStr[regVal]);
      } else {
        arr.add(unitStr[_Protocol.InputRegisters[i].unit]);
      }
    }
  }

  for (int i = 0; i < _Protocol.HoldingRegisterCount; i++) {
    if (_Protocol.HoldingRegisters[i].frontend) {
      // ArduinoJson v7 Syntax:
      JsonArray arr = doc[_Protocol.HoldingRegisters[i].name].to<JsonArray>();

      // Value
      arr.add(getRegValue(&_Protocol.HoldingRegisters[i]));

      const auto regVal = _Protocol.HoldingRegisters[i].value;
      const String regName = _Protocol.HoldingRegisters[i].name;

      if (regName == F("InverterStatus") && regVal < statusStrLength) {
        arr.add(statusStr[regVal]);
      } else if (regName == F("OnOff") && regVal < onoffStrLength) {
        arr.add(onoffStr[regVal]);
      } else {
        arr.add(unitStr[_Protocol.HoldingRegisters[i].unit]);
      }
    }
  }

  if (doc.overflowed()) {
    Log.println(F("CreateUIJson: JsonDocument overflowed! Output will be truncated."));
  }
}

void Growatt::RegisterCommand(const String& command,
                              CommandHandlerFunc handler) {
  handlers[command] = handler;
}

void Growatt::HandleCommand(const String& command, const byte* payload,
                            const unsigned int length, JsonDocument& req,
                            JsonDocument& res) {
  req.clear();
  res.clear();

  // 1. JSON einmalig deserialisieren
  DeserializationError deserializationErr = deserializeJson(req, payload, length);

  if (deserializationErr) {
    String errMsg = "Failed to parse JSON Request in Command '" + command +
                    "': " + String(deserializationErr.c_str());
    Log.println(errMsg);
    res["command"] = command;
    res["success"] = false;
    res["message"] = errMsg;
    return;
  }

  // 2. Metadaten auslesen
  uint8_t retries = 0;
  if (req["retry"].is<uint8_t>()) {
    retries = req["retry"].as<uint8_t>();
  }

  if (req["correlationId"].is<String>()) {
    res["correlationId"] = req["correlationId"].as<String>();
  }

  // 3. Command-Handler suchen
  auto it = handlers.find(command);
  if (it == handlers.end()) {
    Log.println("Unknown Command: " + command);
    res["command"] = command;
    res["success"] = false;
    res["message"] = "Unknown Command: " + command;
    return;
  }

  // 4. Execution Loop mit korrekter Retry-Logik
  bool success = false;
  String message = "";

  for (uint8_t attempt = 0; attempt <= retries; attempt++) {
    if (attempt > 0) {
      Log.printf("Retrying Command '%s' (Attempt %d/%d)...\n", command.c_str(), attempt, retries);
      delay(50); // Kleines Delay vor dem erneuten Modbus-Zugriff
    }

    // Handler ausführen
    std::tie(success, message) = it->second(req, res, *this);

    if (success) {
      break; // Erfolg -> Schleife sofort verlassen
    }
  }

  // 5. Status im Response-JSON setzen
  res["command"] = command;
  res["success"] = success;
  res["message"] = message;

  Log.println(res["message"].as<String>());
}

std::tuple<bool, String> Growatt::handleEcho(const JsonDocument& req,
                                             JsonDocument& res,
                                             Growatt& inverter) {
  // v7 Syntax
  if (!req["text"].is<String>()) {
    return std::make_tuple(false, "'text' Field is required and must be a String");
  }
  String text = req["text"].as<String>();
  res["text"] = "Echo: " + text;
  return std::make_tuple(true, "");
}

std::tuple<bool, String> Growatt::handleCommandList(const JsonDocument& req,
                                                    JsonDocument& res,
                                                    Growatt& inverter) {
  // v7 Syntax
  JsonArray commands = res["commands"].to<JsonArray>();
  for (const auto& pair : handlers) {
    commands.add(pair.first);
  }
  return std::make_tuple(true, "");
}

// std::tuple<bool, String> Growatt::handleModbusGet(const JsonDocument& req,
//                                                   JsonDocument& res,
//                                                   Growatt& inverter) {
//   // 1. Parameter prüfen (Existenz & Typ-Prüfung nach v7 Standard)
//   if (!req["reg"].is<uint16_t>()) {
//     return std::make_tuple(false, "'Register ID' Field is required and must be an integer");
//   }
//   uint16_t reg = req["reg"].as<uint16_t>();

//   if (!req["width"].is<String>()) {
//     return std::make_tuple(false, "'Register Width' Field is required and must be a string");
//   }
//   String width = req["width"].as<String>();

//   if (width != "16b" && width != "32b") {
//     return std::make_tuple(false, "'Register Width' must be '16b' or '32b'");
//   }

//   if (!req["type"].is<String>()) {
//     return std::make_tuple(false, "'Register Type' Field is required and must be a string");
//   }
//   String type = req["type"].as<String>();

//   if (type != "H" && type != "I") {
//     return std::make_tuple(false, "'Register Type' must be 'H' (Holding) or 'I' (Input)");
//   }

//   // 2. Modbus Lesen
//   if (width == "16b") {
//     uint16_t value = 0;
//     bool ok = (type == "H") ? inverter.ReadHoldingReg(reg, &value)
//                             : inverter.ReadInputReg(reg, &value);
//     if (!ok) {
//       return std::make_tuple(false, "Failed to read 16-bit Register!");
//     }
//     res["value"] = value;
//   } else { // 32b
//     uint32_t value = 0;
//     bool ok = (type == "H") ? inverter.ReadHoldingReg(reg, &value)
//                             : inverter.ReadInputReg(reg, &value);
//     if (!ok) {
//       return std::make_tuple(false, "Failed to read 32-bit Register!");
//     }
//     res["value"] = value;
//   }

//   return std::make_tuple(true, "success");
// }

// std::tuple<bool, String> Growatt::handleModbusSet(const JsonDocument& req,
//                                                   JsonDocument& res,
//                                                   Growatt& inverter) {
//   // --- Parameter prüfen ---
//   if (!req["reg"].is<uint16_t>()) {
//     return std::make_tuple(false, "'Register ID' Field is required and must be an integer");
//   }
//   uint16_t reg = req["reg"].as<uint16_t>();

//   if (!req["width"].is<String>()) {
//     return std::make_tuple(false, "'Register Width' Field is required and must be a string");
//   }
//   String width = req["width"].as<String>();

//   if (width == "32b") {
//     return std::make_tuple(false, "Writing to double (32b) Registers is not supported");
//   }
//   if (width != "16b") {
//     return std::make_tuple(false, "'Width' must be '16b'");
//   }

//   if (!req["type"].is<String>()) {
//     return std::make_tuple(false, "'Register Type' Field is required and must be a string");
//   }
//   String type = req["type"].as<String>();

//   if (type == "I") {
//     return std::make_tuple(false, "It is not possible to write into Input Registers");
//   }
//   if (type != "H") {
//     return std::make_tuple(false, "'Register Type' must be 'H' (holding)");
//   }

//   if (!req["val"].is<uint16_t>()) {
//     return std::make_tuple(false, "'Register Value' Field is required and must be an integer");
//   }
//   uint16_t val = req["val"].as<uint16_t>();

//   // --- Write ---
//   if (!inverter.WriteHoldingReg(reg, val)) {
//     return std::make_tuple(false, "Failed to write into Holding Register!");
//   }

//   return std::make_tuple(true, "success");
// }