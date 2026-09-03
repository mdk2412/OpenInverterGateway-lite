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
    JsonDocument doc1;
    doc1["mode"] = 0;
    doc1["retry"] = 2;

    char buf1[32];
    size_t len1 = serializeJson(doc1, buf1, sizeof(buf1));

    JsonDocument req1, res1;
    Inverter.HandleCommand("priority/set", (const byte*)buf1, len1, req1, res1);

    // 2. Command: bdc/set/chargepowerrate
    JsonDocument doc2;
    doc2["value"] = 100;
    doc2["retry"] = 2;

    char buf2[32];
    size_t len2 = serializeJson(doc2, buf2, sizeof(buf2));

    JsonDocument req2, res2;
    Inverter.HandleCommand("bdc/set/chargepowerrate", (const byte*)buf2, len2, req2, res2);
  }

  if (priority == 0 && avg_ptogrid > ptogrid_threshold && soc < 95) {
    JsonDocument doc;
    doc["mode"] = 1;
    doc["retry"] = 2;

    char buf[32];
    size_t len = serializeJson(doc, buf, sizeof(buf));

    JsonDocument req1, res1;
    Inverter.HandleCommand("priority/set", (const byte*)buf, len, req1, res1);
  }
}