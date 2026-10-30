#include "ACChargeControl.h"
#include <ArduinoJson.h>
#include <algorithm>

uint16_t calculateAcChargeTargetPowerRate() {
  int32_t max_power = User.ac_max_pow;

  int32_t off_set = User.ac_off_set * 10;
  int32_t p_chr = Inverter._Protocol.InputRegisters[P3000_BDC_PCHR].value;
  int32_t p_togrid = Inverter._Protocol.InputRegisters[P3000_PTOGRID_TOTAL].value;
  int32_t p_touser = Inverter._Protocol.InputRegisters[P3000_PTOUSER_TOTAL].value;

  int64_t delta = static_cast<int64_t>(p_chr) + p_togrid - p_touser + off_set;
  int32_t rawRate = (delta * 10) / max_power;

  // Aktuelle p_chr-Rate im selben Maßstab
  int32_t current_p_chr_rate = (p_chr * 10) / max_power;

  // --- HHYSTERESE / TOLERANZBAND FÜR DIE KAPPUNG ---
  int32_t desired_cap = current_p_chr_rate + 5;
  const int32_t hysteresis = 2; // 2 Prozentpunkte / Einheiten Toleranz

  static int32_t active_cap = -1;
  if (active_cap < 0) {
    active_cap = desired_cap; // Initialisierung beim ersten Start
  } else {
    // Die Grenze passt sich erst an, wenn der Zielwert das Hysterese-Fenster verlässt
    if (desired_cap > active_cap + hysteresis) {
      active_cap = desired_cap;
    } else if (desired_cap < active_cap - hysteresis) {
      active_cap = desired_cap;
    }
    // Dazwischen (innerhalb von ±2 Einheiten) bleibt active_cap eingefroren!
  }

  // Kappung anwenden
  int32_t cappedRate = std::min(rawRate, active_cap);

  return static_cast<uint16_t>(std::max<int32_t>(0, std::min<int32_t>(cappedRate, 100)));
}

void acchargeControl() {
  // --- User-Parameter laden ---
  // --- Register EINMAL auslesen ---
  int32_t priority = Inverter._Protocol.InputRegisters[P3000_PRIORITY].value;
  int32_t ac_enabled =
      Inverter._Protocol.HoldingRegisters[P3000_BDC_CHARGE_AC_ENABLED].value;
  int32_t soc = Inverter._Protocol.InputRegisters[P3000_BDC_SOC].value;
  uint16_t current_rate =
      Inverter._Protocol.HoldingRegisters[P3000_BDC_CHARGE_P_RATE].value;

  // --- Bedingungen prüfen ---
  if (priority == 1 && ac_enabled == 1) {
    // Akku voll → auf LoadFirst umschalten
    if (soc == 100) {
      JsonDocument req1, res1;
      req1[F("mode")] = 0;
      req1[F("retry")] = NUM_WRITE_RETRIES;

      Inverter.HandleCommand("priority/set", req1, res1);

      JsonDocument req2, res2;
      req2[F("value")] = 100;
      req2[F("retry")] = NUM_WRITE_RETRIES;

      Inverter.HandleCommand("bdc/set/chargepowerrate", req2, res2);

      return;
    }

    uint16_t targetpowerrate = calculateAcChargeTargetPowerRate();

    if (current_rate != targetpowerrate) {
      JsonDocument req, res;
      req[F("value")] = targetpowerrate;
      req[F("retry")] = NUM_WRITE_RETRIES;

      Inverter.HandleCommand("bdc/set/chargepowerrate", req, res);
    }
  }
}