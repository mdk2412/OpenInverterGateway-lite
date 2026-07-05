#ifndef ACCHARGECONTROL_H
#define ACCHARGECONTROL_H

#include <Arduino.h>

class Growatt;
struct UserConfig;

extern Growatt Inverter;
extern UserConfig User;

void acchargeControl();

#endif // ACCHARGECONTROL_H
