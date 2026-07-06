#include "BatteryStandby.h"
#include <ArduinoJson.h>
#include <TLog.h>

void batteryStandby() {
  // --- User-Parameter (bereits als *10 skaliert) ---
  uint32_t wake_threshold = User.bat_wke_thr * 10;
  uint32_t sleep_threshold = User.bat_slp_thr * 10;

  // --- Register EINMAL auslesen ---
  int32_t soc = Inverter._Protocol.InputRegisters[P3000_BDC_SOC].value;
  int32_t discharge_stop =
      Inverter._Protocol.HoldingRegisters[P3000_BDC_DISCHARGE_STOPSOC].value;
  int32_t discharge_rate =
      Inverter._Protocol.HoldingRegisters[P3000_BDC_DISCHARGE_P_RATE].value;
  int32_t sysstate =
      Inverter._Protocol.InputRegisters[P3000_BDC_SYSSTATE].value;
  int32_t ptogrid =
      Inverter._Protocol.InputRegisters[P3000_PTOGRID_TOTAL].value;
  int32_t inverter_status =
      Inverter._Protocol.InputRegisters[P3000_INVERTER_STATUS].value;
  int32_t ppv = Inverter._Protocol.InputRegisters[P3000_PPV].value;

  // --- Disable discharging ---
  if (soc >= 10 && soc <= discharge_stop) {
    if (discharge_rate != 0) {
      StaticJsonDocument<128> req, res;
      const char* payload = "{\"value\":0,\"retry\":2}";

      Inverter.HandleCommand("bdc/set/dischargepowerrate", (const byte*)payload,
                             strlen(payload), req, res);

      if (res["success"] == true) {
        Log.println(F("Battery discharging deactivated"));
      } else {
        Log.println(F("Battery discharging still activated!"));
      }
    }
  }

  // --- Enable discharging with offset 5 ---
  else if (soc >= (discharge_stop + 5)) {
    if (discharge_rate != 100) {
      StaticJsonDocument<128> req, res;
      const char* payload = "{\"value\":100,\"retry\":2}";

      Inverter.HandleCommand("bdc/set/dischargepowerrate", (const byte*)payload,
                             strlen(payload), req, res);

      if (res["success"] == true) {
        Log.println(F("Battery discharging activated"));
      } else {
        Log.println(F("Battery discharging still deactivated!"));
      }
    }
  }

  // --- Battery OFF → wake ---
  if (sysstate == 0) {
    if (ptogrid >= (int32_t)wake_threshold && inverter_status == 1) {
      char json[64];
      snprintf(json, sizeof(json), "{\"value\":3,\"retry\":2}");

      StaticJsonDocument<256> req, res;

      Inverter.HandleCommand("onoff/set", (const byte*)json, strlen(json), req,
                             res);
    }
  }

  // --- Battery ON → sleep ---
  else if (sysstate == 1) {
    if (ptogrid <= (int32_t)sleep_threshold &&
        ppv <= (int32_t)sleep_threshold && soc >= 10 && soc <= discharge_stop) {
      char json[64];
      snprintf(json, sizeof(json), "{\"value\":2,\"retry\":2}");

      StaticJsonDocument<256> req, res;

      Inverter.HandleCommand("onoff/set", (const byte*)json, strlen(json), req,
                             res);
    }
  }
}
