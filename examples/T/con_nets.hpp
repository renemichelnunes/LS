#pragma once
#include "Arduino.h"
#include <vector>
#include <SPIFFS.h>
#include "Arduino.h"
#include <vector>

struct wifi_info{
    char SSID[50] = {'\0'};
    wifi_auth_mode_t auth_type = WIFI_AUTH_OPEN;
    int32_t RSSI = 0;
    char login[100] = {'\0'};
    char pass[100] = {'\0'};
};

class Wifi_connected_nets{
    private:

    public:
        Wifi_connected_nets();
        ~Wifi_connected_nets();
        std::vector <wifi_info> list;
        wifi_info * find(char * ssid);
        bool add(wifi_info wi);
        bool save();
        bool load();
};