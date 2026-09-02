#include "SurplusCharge.h"
#include <ArduinoJson.h>
#include <TLog.h>

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
    char json[32];
    snprintf(json, sizeof(json), "{\"value\":%u,\"retry\":2}", targetpowerrate);

    // ArduinoJson v7: Einfach JsonDocument nutzen (keine Größenangabe <128> nötig)
    JsonDocument req, res;
    Inverter.HandleCommand("bdc/set/chargepowerrate", (const byte*)json,
                           strlen(json), req, res);
  }
}