#include "ACChargeControl.h"
#include <ArduinoJson.h>
#include <algorithm>   // std::clamp

void acchargeControl() {
    // --- User-Parameter laden ---
    uint32_t max_power = User.ac_max_pow;
    int32_t off_set = User.ac_off_set * 10;

    // --- Register EINMAL auslesen ---
    int32_t priority =
        Inverter._Protocol.InputRegisters[P3000_PRIORITY].value;
    int32_t ac_enabled =
        Inverter._Protocol.HoldingRegisters[P3000_BDC_CHARGE_AC_ENABLED].value;
    int32_t soc =
        Inverter._Protocol.InputRegisters[P3000_BDC_SOC].value;
    int32_t p_chr =
        Inverter._Protocol.InputRegisters[P3000_BDC_PCHR].value;
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
            StaticJsonDocument<128> req1, res1;
            Inverter.HandleCommand(
                "priority/set",
                (const byte*)"{\"mode\":0,\"retry\":2}",
                strlen("{\"mode\":0,\"retry\":2}"),
                req1, res1
            );

            StaticJsonDocument<128> req2, res2;
            Inverter.HandleCommand(
                "bdc/set/chargepowerrate",
                (const byte*)"{\"value\":100,\"retry\":2}",
                strlen("{\"value\":100,\"retry\":2}"),
                req2, res2
            );

            return;
        }

        // --- Delta berechnen ---
        int64_t delta =
            (int64_t)p_chr +
            (int64_t)p_togrid -
            (int64_t)p_touser +
            off_set;

        // --- Integer-Mathematik ---
        int32_t rawRate = (delta * 10) / max_power;
        int32_t roundedRate = rawRate;

        // --- clamp auf 0–100 ---
        uint16_t targetpowerrate =
            std::clamp<int32_t>(roundedRate, 0, 100);

        if (current_rate != targetpowerrate) {
            char json[64];
            snprintf(json, sizeof(json),
                     "{\"value\":%d,\"retry\":2}", targetpowerrate);

            StaticJsonDocument<256> req, res;
            Inverter.HandleCommand(
                "bdc/set/chargepowerrate",
                (const byte*)json,
                strlen(json),
                req, res
            );
        }
    }
}
