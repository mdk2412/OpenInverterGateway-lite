#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>
#include <functional>
#include "GrowattTypes.h"

#ifndef NUM_OF_RETRIES
#define NUM_OF_RETRIES 3
#endif

typedef std::tuple<bool, String> CommandResult;
typedef std::function<CommandResult(const JsonDocument&, JsonDocument&, class Growatt&)> CommandHandlerFunc;

class Growatt {
public:
    Growatt();

    void InitProtocol();
    void begin(Stream& serial);

    // Modbus basic
    bool ReadInputReg(uint16_t adr, uint16_t* result);
    bool ReadInputReg(uint16_t adr, uint32_t* result);
    bool ReadHoldingReg(uint16_t adr, uint16_t* result);
    bool ReadHoldingReg(uint16_t adr, uint32_t* result);
    bool WriteHoldingReg(uint16_t adr, uint16_t value);

    bool ReadHoldingRegFrag(uint16_t adr, uint8_t size, uint16_t* result);
    bool ReadHoldingRegFrag(uint16_t adr, uint8_t size, uint32_t* result);
    bool WriteHoldingRegFrag(uint16_t adr, uint8_t size, uint16_t* value);

    bool ReadData(uint8_t maxRetries = NUM_OF_RETRIES);
    bool ReadInputRegisters(uint8_t& offs);
    bool ReadHoldingRegisters(uint8_t& offs);

    // Register access
    sGrowattModbusReg_t GetInputRegister(uint16_t reg);
    sGrowattModbusReg_t GetHoldingRegister(uint16_t reg);
    bool GetSingleValueByName(const String& name, double& value);

    // JSON / UI / Metrics
    void CreateJson(JsonDocument& doc, const String& MacAddress, const String& Hostname);
    void CreateUIJson(JsonDocument& doc, const String& Hostname);
    void CreateMetrics(String& metrics, const String& MacAddress, const String& Hostname);

    // Command system
    void RegisterCommand(const String& command, CommandHandlerFunc handler);
    void HandleCommand(const String& command,
                       const byte* payload,
                       unsigned int length,
                       JsonDocument& req,
                       JsonDocument& res);

    CommandResult handleEcho(const JsonDocument& req, JsonDocument& res, Growatt& inverter);
    CommandResult handleCommandList(const JsonDocument& req, JsonDocument& res, Growatt& inverter);
    CommandResult handleModbusGet(const JsonDocument& req, JsonDocument& res, Growatt& inverter);
    CommandResult handleModbusSet(const JsonDocument& req, JsonDocument& res, Growatt& inverter);

    // Helpers
    bool HasValidData() const;
    void ResetCounters();
    void DumpRegisters(Stream& out);
    bool Refresh();

    sGrowattProtocol_t& GetProtocol();
    sGrowattModbusReg_t* GetInputRegisterPtr();
    sGrowattModbusReg_t* GetHoldingRegisterPtr();

    uint16_t GetInputRegisterCount() const;
    uint16_t GetHoldingRegisterCount() const;

    eDevice_t GetWiFiStickType();
    eDevice_t GetDeviceType() const;

    uint32_t GetPacketCount() const;
    uint32_t GetPacketFailedCount() const;
    uint32_t GetDataUptime() const;

    String DeviceTypeToString() const;
    String StatusString() const;

    // öffentlich, weil dein .ino direkt darauf zugreift
    sGrowattProtocol_t _Protocol;

private:
    double getRegValue(sGrowattModbusReg_t* reg);
    double roundByResolution(const double& value, const float& resolution);

    void camelCaseToSnakeCase(const String& input, char* output);
    void metricsAddValue(const String& name,
                         const double& value,
                         const float& resolution,
                         String& metrics,
                         const String& labels);

    eDevice_t _eDevice;
    bool _GotData;
    uint32_t _PacketCnt;
    uint32_t _PacketCntFailed;

    std::map<String, CommandHandlerFunc> handlers;
};
