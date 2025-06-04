#pragma once

#include "Growatt.h"
#include "ShineWifi.h"

#ifndef MODBUS_TCP_PROXY
#define MODBUS_TCP_PROXY 1
#endif

#ifndef MODBUS_TCP_PORT
#define MODBUS_TCP_PORT 502
#endif

#if MODBUS_TCP_PROXY == 1
void ModbusTcpProxySetup();
void ModbusTcpProxyLoop();
#endif
