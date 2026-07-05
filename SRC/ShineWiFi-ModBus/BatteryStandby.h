#ifndef BATTERYSTANDBY_H
#define BATTERYSTANDBY_H

#include <Arduino.h>

struct UserConfig {
  bool bat_standby;
  int bat_slp_thr;
  int bat_wke_thr;

  bool accharge;
  int ac_max_pow;
  int ac_off_set;
};

class Growatt;

extern Growatt Inverter;
extern UserConfig User;

void batteryStandby();

#endif // BATTERYSTANDBY_H
