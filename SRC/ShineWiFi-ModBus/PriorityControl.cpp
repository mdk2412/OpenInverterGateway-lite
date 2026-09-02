#include "PriorityControl.h"
#include <ArduinoJson.h>
#include <TLog.h>

void priorityControl() {
  static float avg_ptouser = 0;
  static float avg_ptogrid = 0;
  const float alpha = 0.5f;  // Glättungsfaktor

  uint16_t ptogrid_threshold = User.ptogrid_thr;
  uint16_t ptouser_threshold = User.ptouser_thr;

  int32_t priority = Inverter._Protocol.InputRegisters[P3000_PRIORITY].value;
  int32_t soc = Inverter._Protocol.InputRegisters[P3000_BDC_SOC].value;
  int32_t p_togrid =
      Inverter._Protocol.InputRegisters[P3000_PTOGRID_TOTAL].value / 10;
  int32_t p_touser =
      Inverter._Protocol.InputRegisters[P3000_PTOUSER_TOTAL].value / 10;
  int32_t p_disch =
      Inverter._Protocol.InputRegisters[P3000_BDC_PDISCHR].value / 10;
  int32_t p_ch = Inverter._Protocol.InputRegisters[P3000_BDC_PCHR].value / 10;

  avg_ptouser += alpha * (p_touser - p_ch - avg_ptouser);
  avg_ptogrid += alpha * (p_togrid - p_disch - avg_ptogrid);

  if (priority == 1 && avg_ptouser > ptouser_threshold) {
    JsonDocument req1, res1;
    const char* json1 = "{\"mode\":0,\"retry\":2}";
    Inverter.HandleCommand("priority/set", (const byte*)json1,
                           strlen(json1), req1, res1);

    JsonDocument req2, res2;
    const char* json2 = "{\"value\":100,\"retry\":2}";
    Inverter.HandleCommand("bdc/set/chargepowerrate", (const byte*)json2,
                           strlen(json2), req2, res2);
  }

  if (priority == 0 && avg_ptogrid > ptogrid_threshold && soc < 95) {
    JsonDocument req1, res1;
    const char* payload = "{\"mode\":1,\"retry\":2}";
    Inverter.HandleCommand("priority/set", (const byte*)payload,
                           strlen(payload), req1, res1);
  }
}