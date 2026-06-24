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

#include <ModbusRTU.h>
#include <ArduinoJson.h>
#include <TLog.h>

ModbusRTU Modbus;

// -------------------------------------------------------------
// Constructor
// -------------------------------------------------------------
Growatt::Growatt() {
    _eDevice = Undef_stick;
    _PacketCnt = 0;
    _PacketCntFailed = 0;
    _GotData = false;

    handlers = std::map<String, CommandHandlerFunc>();

    RegisterCommand("echo",
        [this](const JsonDocument& req, JsonDocument& res, Growatt& inv) {
            return handleEcho(req, res, *this);
        });

    RegisterCommand("list",
        [this](const JsonDocument& req, JsonDocument& res, Growatt& inv) {
            return handleCommandList(req, res, *this);
        });

    RegisterCommand("modbus/get",
        [this](const JsonDocument& req, JsonDocument& res, Growatt& inv) {
            return handleModbusGet(req, res, *this);
        });

    RegisterCommand("modbus/set",
        [this](const JsonDocument& req, JsonDocument& res, Growatt& inv) {
            return handleModbusSet(req, res, *this);
        });
}

// -------------------------------------------------------------
// InitProtocol
// -------------------------------------------------------------
void Growatt::InitProtocol() {
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
#endif
}

// -------------------------------------------------------------
// begin()
// -------------------------------------------------------------
void Growatt::begin(Stream& serial) {
#if SIMULATE_INVERTER == 1
    _eDevice = SIMULATE_DEVICE;
#else
    uint16_t dummy;

    // Try ShineWiFi-X (115200)
    Serial.begin(115200);
    Modbus.begin(&serial);
    Modbus.master();
    Modbus.setBaudrate(115200);

    if (ReadInputReg(0, &dummy)) {
        _eDevice = ShineWiFi_X;
        return;
    }

    // Try ShineWiFi-S (9600)
    Serial.begin(9600);
    Modbus.begin(&serial);
    Modbus.master();
    Modbus.setBaudrate(9600);

    if (ReadInputReg(0, &dummy)) {
        _eDevice = ShineWiFi_S;
    }
#endif
}

// -------------------------------------------------------------
// GetWiFiStickType
// -------------------------------------------------------------
eDevice_t Growatt::GetWiFiStickType() {
    return _eDevice;
}
// -------------------------------------------------------------
// ReadInputReg (16-bit)
// -------------------------------------------------------------
bool Growatt::ReadInputReg(uint16_t adr, uint16_t* result) {
#if SIMULATE_INVERTER != 1
    bool done = false;
    bool ok = false;

    Modbus.readIreg(
        1, adr, result, 1,
        [&](Modbus::ResultCode event, uint16_t, void*) -> bool {
            ok = (event == Modbus::EX_SUCCESS);
            done = true;
            return true;
        },
        1
    );

    while (!done) {
        Modbus.task();
        yield();
    }

    return ok;
#else
    *result = 0;
    return true;
#endif
}

// -------------------------------------------------------------
// ReadInputReg (32-bit)
// -------------------------------------------------------------
bool Growatt::ReadInputReg(uint16_t adr, uint32_t* result) {
#if SIMULATE_INVERTER != 1
    uint16_t buf[2];
    bool done = false;
    bool ok = false;

    Modbus.readIreg(
        1, adr, buf, 2,
        [&](Modbus::ResultCode event, uint16_t, void*) -> bool {
            ok = (event == Modbus::EX_SUCCESS);
            done = true;
            return true;
        },
        1
    );

    while (!done) {
        Modbus.task();
        yield();
    }

    if (ok) {
        *result = (uint32_t(buf[0]) << 16) | buf[1];
    }
    return ok;
#else
    *result = 0;
    return true;
#endif
}

// -------------------------------------------------------------
// ReadHoldingReg (16-bit)
// -------------------------------------------------------------
bool Growatt::ReadHoldingReg(uint16_t adr, uint16_t* result) {
#if SIMULATE_INVERTER != 1
    bool done = false;
    bool ok = false;

    Modbus.readHreg(
        1, adr, result, 1,
        [&](Modbus::ResultCode event, uint16_t, void*) -> bool {
            ok = (event == Modbus::EX_SUCCESS);
            done = true;
            return true;
        },
        1
    );

    while (!done) {
        Modbus.task();
        yield();
    }

    return ok;
#else
    *result = 0;
    return true;
#endif
}

// -------------------------------------------------------------
// ReadHoldingReg (32-bit)
// -------------------------------------------------------------
bool Growatt::ReadHoldingReg(uint16_t adr, uint32_t* result) {
#if SIMULATE_INVERTER != 1
    uint16_t buf[2];
    bool done = false;
    bool ok = false;

    Modbus.readHreg(
        1, adr, buf, 2,
        [&](Modbus::ResultCode event, uint16_t, void*) -> bool {
            ok = (event == Modbus::EX_SUCCESS);
            done = true;
            return true;
        },
        1
    );

    while (!done) {
        Modbus.task();
        yield();
    }

    if (ok) {
        *result = (uint32_t(buf[0]) << 16) | buf[1];
    }
    return ok;
#else
    *result = 0;
    return true;
#endif
}

// -------------------------------------------------------------
// WriteHoldingReg (single register)
// -------------------------------------------------------------
bool Growatt::WriteHoldingReg(uint16_t adr, uint16_t value) {
#if SIMULATE_INVERTER != 1
    bool done = false;
    bool ok = false;

    Modbus.writeHreg(
        1, adr, value,
        [&](Modbus::ResultCode event, uint16_t, void*) -> bool {
            ok = (event == Modbus::EX_SUCCESS);
            done = true;
            return true;
        },
        1
    );

    while (!done) {
        Modbus.task();
        yield();
    }

    return ok;
#else
    return true;
#endif
}

// -------------------------------------------------------------
// WriteHoldingRegFrag (multiple 16-bit registers)
// -------------------------------------------------------------
bool Growatt::WriteHoldingRegFrag(uint16_t adr, uint8_t size, uint16_t* value) {
#if SIMULATE_INVERTER != 1
    bool done = false;
    bool ok = false;

    Modbus.writeHreg(
        1, adr, value, size,
        [&](Modbus::ResultCode event, uint16_t, void*) -> bool {
            ok = (event == Modbus::EX_SUCCESS);
            done = true;
            return true;
        },
        1
    );

    while (!done) {
        Modbus.task();
        yield();
    }

    return ok;
#else
    return true;
#endif
}
// -------------------------------------------------------------
// ReadHoldingRegFrag (16-bit array)
// -------------------------------------------------------------
bool Growatt::ReadHoldingRegFrag(uint16_t adr, uint8_t size, uint16_t* result) {
#if SIMULATE_INVERTER != 1
    bool done = false;
    bool ok = false;

    Modbus.readHreg(
        1, adr, result, size,
        [&](Modbus::ResultCode event, uint16_t, void*) -> bool {
            ok = (event == Modbus::EX_SUCCESS);
            done = true;
            return true;
        },
        1
    );

    while (!done) {
        Modbus.task();
        yield();
    }

    return ok;
#else
    memset(result, 0, size * sizeof(uint16_t));
    return true;
#endif
}

// -------------------------------------------------------------
// ReadHoldingRegFrag (32-bit array)
// -------------------------------------------------------------
bool Growatt::ReadHoldingRegFrag(uint16_t adr, uint8_t size, uint32_t* result) {
#if SIMULATE_INVERTER != 1
    uint16_t buf[128];
    bool done = false;
    bool ok = false;

    Modbus.readHreg(
        1, adr, buf, size * 2,
        [&](Modbus::ResultCode event, uint16_t, void*) -> bool {
            ok = (event == Modbus::EX_SUCCESS);
            done = true;
            return true;
        },
        1
    );

    while (!done) {
        Modbus.task();
        yield();
    }

    if (!ok) return false;

    for (int i = 0; i < size; i++) {
        result[i] = (uint32_t(buf[i * 2]) << 16) | buf[i * 2 + 1];
    }

    return true;
#else
    memset(result, 0, size * sizeof(uint32_t));
    return true;
#endif
}

// -------------------------------------------------------------
// ReadData (fragment orchestrator)
// -------------------------------------------------------------
bool Growatt::ReadData(uint8_t maxRetries) {
    uint8_t inputFrag = 0;
    uint8_t holdingFrag = 0;
    bool ok;

    uint8_t retry = 0;
    while (inputFrag < _Protocol.InputFragmentCount && retry < maxRetries) {
        ok = ReadInputRegisters(inputFrag);
        if (ok) _PacketCnt++;
        else { retry++; _PacketCntFailed++; }
    }

    retry = 0;
    while (holdingFrag < _Protocol.HoldingFragmentCount && retry < maxRetries) {
        ok = ReadHoldingRegisters(holdingFrag);
        if (ok) _PacketCnt++;
        else { retry++; _PacketCntFailed++; }
    }

    _GotData = (retry < maxRetries);
    return _GotData;
}

// -------------------------------------------------------------
// ReadInputRegisters (fragment)
// -------------------------------------------------------------
bool Growatt::ReadInputRegisters(uint8_t& offs) {
#if SIMULATE_INVERTER != 1
    if (offs >= _Protocol.InputFragmentCount) return true;

    auto& frag = _Protocol.InputReadFragments[offs];
    uint16_t buf[128];

    bool done = false;
    bool ok = false;

    Modbus.readIreg(
        1, frag.StartAddress, buf, frag.FragmentSize,
        [&](Modbus::ResultCode event, uint16_t, void*) -> bool {
            ok = (event == Modbus::EX_SUCCESS);
            done = true;
            return true;
        },
        1
    );

    while (!done) {
        Modbus.task();
        yield();
    }

    if (!ok) return false;

    for (int i = 0; i < _Protocol.InputRegisterCount; i++) {
        auto& r = _Protocol.InputRegisters[i];
        if (r.address >= frag.StartAddress &&
            r.address < frag.StartAddress + frag.FragmentSize) {

            uint16_t idx = r.address - frag.StartAddress;

            if (r.size == SIZE_16BIT || r.size == SIZE_16BIT_S) {
                r.value = buf[idx];
            } else {
                r.value = (uint32_t(buf[idx]) << 16) | buf[idx + 1];
            }
        }
    }

    offs++;
    return true;
#else
    offs++;
    return true;
#endif
}

// -------------------------------------------------------------
// ReadHoldingRegisters (fragment)
// -------------------------------------------------------------
bool Growatt::ReadHoldingRegisters(uint8_t& offs) {
#if SIMULATE_INVERTER != 1
    if (offs >= _Protocol.HoldingFragmentCount) return true;

    auto& frag = _Protocol.HoldingReadFragments[offs];
    uint16_t buf[128];

    bool done = false;
    bool ok = false;

    Modbus.readHreg(
        1, frag.StartAddress, buf, frag.FragmentSize,
        [&](Modbus::ResultCode event, uint16_t, void*) -> bool {
            ok = (event == Modbus::EX_SUCCESS);
            done = true;
            return true;
        },
        1
    );

    while (!done) {
        Modbus.task();
        yield();
    }

    if (!ok) return false;

    for (int i = 0; i < _Protocol.HoldingRegisterCount; i++) {
        auto& r = _Protocol.HoldingRegisters[i];
        if (r.address >= frag.StartAddress &&
            r.address < frag.StartAddress + frag.FragmentSize) {

            uint16_t idx = r.address - frag.StartAddress;

            if (r.size == SIZE_16BIT || r.size == SIZE_16BIT_S) {
                r.value = buf[idx];
            } else {
                r.value = (uint32_t(buf[idx]) << 16) | buf[idx + 1];
            }
        }
    }

    offs++;
    return true;
#else
    offs++;
    return true;
#endif
}

// -------------------------------------------------------------
// GetInputRegister / GetHoldingRegister
// -------------------------------------------------------------
sGrowattModbusReg_t Growatt::GetInputRegister(uint16_t reg) {
    if (!_GotData) ReadData(NUM_OF_RETRIES);
    return _Protocol.InputRegisters[reg];
}

sGrowattModbusReg_t Growatt::GetHoldingRegister(uint16_t reg) {
    if (!_GotData) ReadData(1);
    return _Protocol.HoldingRegisters[reg];
}
// -------------------------------------------------------------
// Value conversion helpers
// -------------------------------------------------------------
double Growatt::roundByResolution(const double& value, const float& resolution) {
    double res = 1.0 / resolution;
    return int32_t(value * res + 0.5) / res;
}

double Growatt::getRegValue(sGrowattModbusReg_t* reg) {
    double result = 0;
    RegisterSize_t size = reg->size;
    const float& mult = reg->multiplier;
    const uint32_t& raw = reg->value;
    const float& resolution = reg->resolution;

    switch (size) {
        case SIZE_16BIT_S:
            result = (mult == (int)mult)
                ? (int16_t)raw * mult
                : roundByResolution((int16_t)raw * mult, resolution);
            break;

        case SIZE_32BIT_S:
            result = (mult == (int)mult)
                ? (int32_t)raw * mult
                : roundByResolution((int32_t)raw * mult, resolution);
            break;

        default:
            result = (mult == (int)mult)
                ? raw * mult
                : roundByResolution(raw * mult, resolution);
    }

    return result;
}

// -------------------------------------------------------------
// GetSingleValueByName
// -------------------------------------------------------------
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

// -------------------------------------------------------------
// CreateJson
// -------------------------------------------------------------
void Growatt::CreateJson(JsonDocument& doc,
                         const String& MacAddress,
                         const String& Hostname) {

    if (!Hostname.isEmpty())
        doc["Hostname"] = Hostname;

#if SIMULATE_INVERTER != 1
    for (int i = 0; i < _Protocol.InputRegisterCount; i++) {
        auto& r = _Protocol.InputRegisters[i];
        doc[r.name] = getRegValue(&r);
    }

    for (int i = 0; i < _Protocol.HoldingRegisterCount; i++) {
        auto& r = _Protocol.HoldingRegisters[i];
        doc[r.name] = getRegValue(&r);
    }
#else
    // Simulation mode
    doc["Status"] = 1;
    doc["DcPower"] = 230;
    doc["DcVoltage"] = 70.5;
    doc["DcInputCurrent"] = 8.5;
    doc["AcFreq"] = 50.00;
    doc["AcVoltage"] = 230.0;
    doc["AcPower"] = 0.00;
    doc["EnergyToday"] = 0.3;
    doc["EnergyTotal"] = 49.1;
    doc["OperatingTime"] = 123456;
    doc["Temperature"] = 21.12;
    doc["AccumulatedEnergy"] = 320;
#endif

    doc["Mac"] = MacAddress;
    doc["Cnt"] = _PacketCnt;
    doc["CntFailed"] = _PacketCntFailed;
    doc["Uptime"] = millis() / 1000;
    doc["WifiRSSI"] = WiFi.RSSI();
    doc["HeapFree"] = ESP.getFreeHeap();

#if defined(ESP32)
    doc["HeapSize"] = ESP.getHeapSize();
    doc["HeapMaxAlloc"] = ESP.getMaxAllocHeap();
    doc["HeapMinFree"] = ESP.getMinFreeHeap();
    doc["HeapFragmentation"] =
        100 - (100 * ESP.getMaxAllocHeap() / ESP.getFreeHeap());
#else
    static uint32_t heap_min_free = ESP.getFreeHeap();
    heap_min_free = min(heap_min_free, ESP.getFreeHeap());

    doc["HeapMaxAlloc"] = ESP.getMaxFreeBlockSize();
    doc["HeapMinFree"] = heap_min_free;
    doc["HeapFragmentation"] = ESP.getHeapFragmentation();
#endif
}

// -------------------------------------------------------------
// CreateUIJson
// -------------------------------------------------------------
void Growatt::CreateUIJson(JsonDocument& doc, const String& Hostname) {
    doc["Hostname"] = Hostname;

    JsonObject input = doc.createNestedObject("input");
    for (int i = 0; i < _Protocol.InputRegisterCount; i++) {
        auto& r = _Protocol.InputRegisters[i];
        input[r.name] = getRegValue(&r);
    }

    JsonObject holding = doc.createNestedObject("holding");
    for (int i = 0; i < _Protocol.HoldingRegisterCount; i++) {
        auto& r = _Protocol.HoldingRegisters[i];
        holding[r.name] = getRegValue(&r);
    }
}
// -------------------------------------------------------------
// Metrics helpers
// -------------------------------------------------------------
void Growatt::camelCaseToSnakeCase(const String& input, char* output) {
    int out = 0;

    for (uint i = 0; input[i] != '\0'; i++) {
        char c = input[i];

        // Insert underscore before uppercase transitions
        if (i > 0 &&
            isUpperCase(c) &&
            (isLowerCase(input[i - 1]) ||
             (i < input.length() - 1 && isLowerCase(input[i + 1])))) {
            output[out++] = '_';
        }

        output[out++] = toLowerCase(c);
    }

    output[out] = '\0';
}

void Growatt::metricsAddValue(const String& name,
                              const double& value,
                              const float& resolution,
                              String& metrics,
                              const String& labels) {

    String svalue = (resolution == 1.0)
        ? String((int32_t)value)
        : String(value);

char snake[64];
camelCaseToSnakeCase(name, snake);
metrics += "growatt_";
metrics += String(snake);
    metrics += "{";
    metrics += labels;
    metrics += "} ";
    metrics += svalue;
    metrics += "\n";
}

// -------------------------------------------------------------
// CreateMetrics
// -------------------------------------------------------------
void Growatt::CreateMetrics(String& metrics,
                            const String& MacAddress,
                            const String& Hostname) {

    String labels = "mac=\"" + MacAddress + "\",host=\"" + Hostname + "\"";

    // Input registers
    for (int i = 0; i < _Protocol.InputRegisterCount; i++) {
        auto& r = _Protocol.InputRegisters[i];
        metricsAddValue(r.name,
                        getRegValue(&r),
                        r.resolution,
                        metrics,
                        labels);
    }

    // Holding registers
    for (int i = 0; i < _Protocol.HoldingRegisterCount; i++) {
        auto& r = _Protocol.HoldingRegisters[i];
        metricsAddValue(r.name,
                        getRegValue(&r),
                        r.resolution,
                        metrics,
                        labels);
    }

    // Internal counters
    metrics += "growatt_packets_total{" + labels + "} " +
               String(_PacketCnt) + "\n";

    metrics += "growatt_packets_failed{" + labels + "} " +
               String(_PacketCntFailed) + "\n";
}
// -------------------------------------------------------------
// RegisterCommand
// -------------------------------------------------------------
void Growatt::RegisterCommand(const String& command,
                              CommandHandlerFunc handler) {
    handlers[command] = handler;
}

// -------------------------------------------------------------
// HandleCommand (MQTT / UI / API entrypoint)
// -------------------------------------------------------------
void Growatt::HandleCommand(const String& command,
                            const byte* payload,
                            unsigned int length,
                            JsonDocument& req,
                            JsonDocument& res) {

    // Unknown command?
    if (!handlers.count(command)) {
        res["success"] = false;
        res["error"] = "Unknown command: " + command;
        return;
    }

    // Parse JSON payload if present
    if (length > 0) {
        DeserializationError err = deserializeJson(req, payload, length);
        if (err) {
            res["success"] = false;
            res["error"] = "JSON parse error";
            return;
        }
    }

    // Execute handler
    auto& handler = handlers[command];
    auto result = handler(req, res, *this);

    bool ok = std::get<0>(result);
    String msg = std::get<1>(result);

    res["success"] = ok;
    if (!msg.isEmpty())
        res["message"] = msg;
}

// -------------------------------------------------------------
// handleEcho
// -------------------------------------------------------------
std::tuple<bool, String>
Growatt::handleEcho(const JsonDocument& req,
                    JsonDocument& res,
                    Growatt& inverter) {

    if (req.containsKey("msg"))
        res["echo"] = req["msg"].as<String>();
    else
        res["echo"] = "no message";

    return { true, "" };
}

// -------------------------------------------------------------
// handleCommandList
// -------------------------------------------------------------
std::tuple<bool, String>
Growatt::handleCommandList(const JsonDocument& req,
                           JsonDocument& res,
                           Growatt& inverter) {

    JsonArray arr = res.createNestedArray("commands");

    for (auto const& kv : handlers) {
        arr.add(kv.first);
    }

    return { true, "" };
}

// -------------------------------------------------------------
// handleModbusGet
// -------------------------------------------------------------
std::tuple<bool, String>
Growatt::handleModbusGet(const JsonDocument& req,
                         JsonDocument& res,
                         Growatt& inverter) {

    if (!req.containsKey("name"))
        return { false, "Missing field: name" };

    String name = req["name"].as<String>();
    double value;

    if (!inverter.GetSingleValueByName(name, value))
        return { false, "Unknown register: " + name };

    res["value"] = value;
    return { true, "" };
}

// -------------------------------------------------------------
// handleModbusSet
// -------------------------------------------------------------
std::tuple<bool, String>
Growatt::handleModbusSet(const JsonDocument& req,
                         JsonDocument& res,
                         Growatt& inverter) {

    if (!req.containsKey("address"))
        return { false, "Missing field: address" };
    if (!req.containsKey("value"))
        return { false, "Missing field: value" };

    uint16_t adr = req["address"].as<uint16_t>();
    uint16_t val = req["value"].as<uint16_t>();

    bool ok = inverter.WriteHoldingReg(adr, val);
    if (!ok)
        return { false, "Write failed" };

    res["written"] = val;
    return { true, "" };
}
// -------------------------------------------------------------
// Additional helper: check if data was received
// -------------------------------------------------------------
bool Growatt::HasValidData() const {
    return _GotData;
}

// -------------------------------------------------------------
// Reset counters
// -------------------------------------------------------------
void Growatt::ResetCounters() {
    _PacketCnt = 0;
    _PacketCntFailed = 0;
}

// -------------------------------------------------------------
// Debug dump of protocol registers (optional)
// -------------------------------------------------------------
void Growatt::DumpRegisters(Stream& out) {
    out.println("=== Growatt Register Dump ===");

    out.println("-- Input Registers --");
    for (int i = 0; i < _Protocol.InputRegisterCount; i++) {
        auto& r = _Protocol.InputRegisters[i];
        out.print(r.name);
        out.print(" = ");
        out.println(getRegValue(&r));
    }

    out.println("-- Holding Registers --");
    for (int i = 0; i < _Protocol.HoldingRegisterCount; i++) {
        auto& r = _Protocol.HoldingRegisters[i];
        out.print(r.name);
        out.print(" = ");
        out.println(getRegValue(&r));
    }
}

// -------------------------------------------------------------
// Manual refresh of all data
// -------------------------------------------------------------
bool Growatt::Refresh() {
    _GotData = false;
    return ReadData(NUM_OF_RETRIES);
}

// -------------------------------------------------------------
// Access to protocol structure
// -------------------------------------------------------------
sGrowattProtocol_t& Growatt::GetProtocol() {
    return _Protocol;
}

// -------------------------------------------------------------
// Access to raw register arrays
// -------------------------------------------------------------
sGrowattModbusReg_t* Growatt::GetInputRegisterPtr() {
    return _Protocol.InputRegisters;
}

sGrowattModbusReg_t* Growatt::GetHoldingRegisterPtr() {
    return _Protocol.HoldingRegisters;
}

// -------------------------------------------------------------
// Get number of registers
// -------------------------------------------------------------
uint16_t Growatt::GetInputRegisterCount() const {
    return _Protocol.InputRegisterCount;
}

uint16_t Growatt::GetHoldingRegisterCount() const {
    return _Protocol.HoldingRegisterCount;
}

// -------------------------------------------------------------
// Get Modbus device type
// -------------------------------------------------------------
String Growatt::DeviceTypeToString() const {
    switch (_eDevice) {
        case Undef_stick: return "Unknown";
        case ShineWiFi_S: return "ShineWiFi-S";
        case ShineWiFi_X: return "ShineWiFi-X";
        case ShineWiFi_F: return "ShineWiFi-F";
        default:          return "Unknown";
    }
}
// -------------------------------------------------------------
// Get device type enum
// -------------------------------------------------------------
eDevice_t Growatt::GetDeviceType() const {
    return _eDevice;
}

// -------------------------------------------------------------
// Get packet counters
// -------------------------------------------------------------
uint32_t Growatt::GetPacketCount() const {
    return _PacketCnt;
}

uint32_t Growatt::GetPacketFailedCount() const {
    return _PacketCntFailed;
}

// -------------------------------------------------------------
// Get uptime of inverter data (seconds since last refresh)
// -------------------------------------------------------------
uint32_t Growatt::GetDataUptime() const {
    return millis() / 1000;
}

// -------------------------------------------------------------
// Simple status string
// -------------------------------------------------------------
String Growatt::StatusString() const {
    if (!_GotData) return "no data";
    return "ok";
}

// -------------------------------------------------------------
// END OF FILE
// -------------------------------------------------------------
