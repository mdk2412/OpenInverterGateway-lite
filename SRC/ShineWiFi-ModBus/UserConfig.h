#pragma once

#include <Arduino.h>
#include "Config.h"

#if MQTT_SUPPORTED == 1
struct MqttConfig {
    String server;
    String port;
    String topic;
    String user;
    String pwd;
};
#endif

struct UserConfig {
    String hostname;
    String static_ip;
    String static_netmask;
    String static_gateway;
    String static_dns;

#if MQTT_SUPPORTED == 1
    MqttConfig mqtt;
#endif

    String syslog_ip;
    bool force_ap;

    bool bat_standby;
    int bat_slp_thr;
    int bat_wke_thr;

    bool accharge;
    int ac_max_pow;
    int ac_off_set;

    bool prioctrl;
    int ptogrid_thr;
    int ptouser_thr;

    bool surch;
    int power_limit;
};

// nur Deklaration – keine Instanz!
extern UserConfig User;
