#ifndef ACCHARGECONTROL_H
#define ACCHARGECONTROL_H

#include <Arduino.h>
#include "UserConfig.h"

class Growatt;

extern Growatt Inverter;

void acchargeControl();

#endif // ACCHARGECONTROL_H
