#include "PriorityControl.h"
#include <ArduinoJson.h>
#include <TLog.h>

void priorityControl() {
  static float avg_ptouser = 0;
  static float avg_ptogrid = 0;
  const float alpha = 0.1f;  // Glättungsfaktor

  uint16_t ptogrid_threshold = User.ptogrid_thr;
  uint16_t ptouser_threshold = User.ptouser_thr;
  // Log.print("PtoGrid: ");
  // Log.println(ptogrid_threshold);
  // Log.print("PtoUser: ");
  // Log.println(ptouser_threshold);

  int32_t priority = Inverter._Protocol.InputRegisters[P3000_PRIORITY].value;
  // int32_t soc      = Inverter._Protocol.InputRegisters[P3000_BDC_SOC].value;
  int32_t p_togrid = Inverter._Protocol.InputRegisters[P3000_PTOGRID_TOTAL].value / 10;
  int32_t p_touser = Inverter._Protocol.InputRegisters[P3000_PTOUSER_TOTAL].value / 10;

  if (avg_ptouser == 0) avg_ptouser = p_touser;
  if (avg_ptogrid == 0) avg_ptogrid = p_togrid;

  // --- Gleitende Mittelwerte aktualisieren ---
  // if (p_touser == 0) avg_ptouser = 0;
  // if (p_togrid == 0) avg_ptogrid = 0;

  if (p_touser == 0) {
    avg_ptouser = 0;
  } else {
    avg_ptouser += alpha * (p_touser - avg_ptouser);
  }

  if (p_togrid == 0) {
    avg_ptogrid = 0;
  } else {
    avg_ptogrid += alpha * (p_togrid - avg_ptogrid);
  }

  // Log.print("Netzbezug: ");
  // Log.println(avg_ptouser);
  // Log.print("Einspeisung: ");
  // Log.println(avg_ptogrid);

  // --- Bedingungen prüfen ---
  if (priority == 1 && avg_ptouser > ptouser_threshold) {
    StaticJsonDocument<128> req1, res1;
    Inverter.HandleCommand("priority/set",
                           (const byte*)"{\"mode\":0,\"retry\":2}",
                           strlen("{\"mode\":0,\"retry\":2}"), req1, res1);
    StaticJsonDocument<128> req2, res2;
    Inverter.HandleCommand("bdc/set/chargepowerrate",
                           (const byte*)"{\"value\":100,\"retry\":2}",
                           strlen("{\"value\":100,\"retry\":2}"), req2, res2);
  }

  if (priority == 0 && avg_ptogrid > ptogrid_threshold) {
    StaticJsonDocument<128> req1, res1;
    Inverter.HandleCommand("priority/set",
                           (const byte*)"{\"mode\":1,\"retry\":2}",
                           strlen("{\"mode\":1,\"retry\":2}"), req1, res1);
  }
}
