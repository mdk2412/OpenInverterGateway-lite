#include "LoadSettings.h"
#include "UserConfig.h"
#include <Preferences.h>

// Local defaults (kept in sync with original sketch)
constexpr int DEFAULT_SLEEP_THR = 50;
constexpr int DEFAULT_WAKE_THR = 75;
constexpr int DEFAULT_AC_MAX = 3750;
constexpr int DEFAULT_OFFSET = -25;

void loadSettingsFromPrefs() {
  Preferences prefs;
  prefs.begin("config", true);

  // Battery Standby (bool)
  User.bat_standby = prefs.getBool("bat_standby", true);

  // Sleep Threshold (>0)
  {
    int v = prefs.getInt("bat_slp_thr", DEFAULT_SLEEP_THR);
    if (v <= 0) v = DEFAULT_SLEEP_THR;
    User.bat_slp_thr = v;
  }

  // Wake Threshold (>0)
  {
    int v = prefs.getInt("bat_wke_thr", DEFAULT_WAKE_THR);
    if (v <= 0) v = DEFAULT_WAKE_THR;
    User.bat_wke_thr = v;
  }

  // AC Charging enabled?
  User.accharge = prefs.getBool("accharge", true);

  // AC Max Power (valid range 2500–12500)
  int v = prefs.getInt("ac_max_pow", DEFAULT_AC_MAX);
  if (v < 2500 || v > 12500) v = DEFAULT_AC_MAX;
  User.ac_max_pow = v;

  // Offset (valid range -100 to +100)
  {
    int v = prefs.getInt("ac_off_set", DEFAULT_OFFSET);
    if (v < -100 || v > 100) v = DEFAULT_OFFSET;
    User.ac_off_set = v;
  }

  prefs.end();
}
