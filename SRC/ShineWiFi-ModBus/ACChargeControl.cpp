#include "ACChargeControl.h"
#include <ArduinoJson.h>
#include <algorithm>  // std::clamp

void acchargeControl() {
  // --- User-Parameter laden ---
  uint32_t max_power = User.ac_max_pow;
  int32_t off_set = User.ac_off_set * 10;

  // --- Register EINMAL auslesen ---
  int32_t priority = Inverter._Protocol.InputRegisters[P3000_PRIORITY].value;
  int32_t ac_enabled =
      Inverter._Protocol.HoldingRegisters[P3000_BDC_CHARGE_AC_ENABLED].value;
  int32_t soc = Inverter._Protocol.InputRegisters[P3000_BDC_SOC].value;
  int32_t p_chr = Inverter._Protocol.InputRegisters[P3000_BDC_PCHR].value;
  int32_t p_togrid =
      Inverter._Protocol.InputRegisters[P3000_PTOGRID_TOTAL].value;
  int32_t p_touser =
      Inverter._Protocol.InputRegisters[P3000_PTOUSER_TOTAL].value;
  uint16_t current_rate =
      Inverter._Protocol.HoldingRegisters[P3000_BDC_CHARGE_P_RATE].value;

  // --- Bedingungen prüfen ---
  if (priority == 1 && ac_enabled == 1) {
    // Akku voll → auf LoadFirst umschalten
    if (soc == 100) {
      JsonDocument req1, res1;
      req1["mode"] = 0;
      req1["retry"] = 2;

      Inverter.HandleCommand("priority/set", req1, res1);

      JsonDocument req2, res2;
      req2["value"] = 100;
      req2["retry"] = 2;

      Inverter.HandleCommand("bdc/set/chargepowerrate", req2, res2);

      return;
    }

    // --- Delta berechnen ---
    int64_t delta = p_chr + p_togrid - p_touser + off_set;

    // --- Integer-Mathematik ---
    int32_t rawRate = (delta * 10) / max_power;

    // --- clamp auf 0–100 ---
    uint16_t targetpowerrate = std::clamp(rawRate, 0, 100);

    if (current_rate != targetpowerrate) {
      JsonDocument req, res;
      req["value"] = targetpowerrate;
      req["retry"] = 2;

      Inverter.HandleCommand("bdc/set/chargepowerrate", req, res);
    }
  }
}