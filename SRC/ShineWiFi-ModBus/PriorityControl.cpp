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
    // 1. Command: priority/set
    JsonDocument req1, res1;
    req1["mode"] = 0;
    req1["retry"] = 2;

    Inverter.HandleCommand("priority/set", req1, res1);

    // 2. Command: bdc/set/chargepowerrate
    JsonDocument req2, res2;
    req2["value"] = 100;
    req2["retry"] = 2;

    Inverter.HandleCommand("bdc/set/chargepowerrate", req2, res2);
  }

  if (priority == 0 && avg_ptogrid > ptogrid_threshold && soc < 95) {
    JsonDocument req, res;
    req["mode"] = 1;
    req["retry"] = 2;

    Inverter.HandleCommand("priority/set", req, res);
  }
}