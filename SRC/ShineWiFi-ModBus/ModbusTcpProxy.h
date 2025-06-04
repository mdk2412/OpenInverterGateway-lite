#pragma once

#include "Growatt.h"
#include "ShineWifi.h"

#if MODBUS_TCP_PROXY == 1
void ModbusTcpProxySetup();
void ModbusTcpProxyLoop();
#endif
