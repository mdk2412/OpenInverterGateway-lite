#include "SurplusCharge.h"
#include <ArduinoJson.h>
#include <TLog.h>
#include <algorithm> // std::clamp, std::max

void surplusCharge() {
  // --- User-Parameter laden ---
  uint32_t max_power = User.ac_max_pow;
  uint64_t power_limit = User.power_limit * 10;

  // --- Register EINMAL auslesen ---
  int32_t p_chr = Inverter._Protocol.InputRegisters[P3000_BDC_PCHR].value;
  int32_t p_togrid =
      Inverter._Protocol.InputRegisters[P3000_PTOGRID_TOTAL].value;
  int32_t p_touser =
      Inverter._Protocol.InputRegisters[P3000_PTOUSER_TOTAL].value;
  uint16_t current_rate =
      Inverter._Protocol.HoldingRegisters[P3000_BDC_CHARGE_P_RATE].value;

  // --- Delta berechnen ---
  int64_t delta = p_chr + p_togrid - p_touser - power_limit;

  // clamp Delta auf >= 0
  delta = std::max<int64_t>(delta, 0);

  // Rate berechnen
  int32_t rate = (delta * 10) / max_power;

  // clamp auf 0–100
  uint16_t targetpowerrate = std::clamp(rate, 0, 100);

  if (current_rate < targetpowerrate) {
    // 1. Dokument strukturiert befüllen
    JsonDocument payloadDoc;
    payloadDoc["value"] = targetpowerrate;
    payloadDoc["retry"] = 2;

    // 2. Ohne snprintf in Puffer serialisieren
    char jsonBuf[32];
    size_t len = serializeJson(payloadDoc, jsonBuf, sizeof(jsonBuf));

    // 3. HandleCommand liest den Puffer ein
    JsonDocument req, res;
    Inverter.HandleCommand("bdc/set/chargepowerrate", (const byte*)jsonBuf,
                           len, req, res);
  }
}