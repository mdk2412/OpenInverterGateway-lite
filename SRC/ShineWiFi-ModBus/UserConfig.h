#ifndef USERCONFIG_H
#define USERCONFIG_H

#include <Arduino.h>

struct UserConfig {
  bool bat_standby;
  int bat_slp_thr;
  int bat_wke_thr;

  bool accharge;
  int ac_max_pow;
  int ac_off_set;
};

extern UserConfig User;

#endif // USERCONFIG_H
