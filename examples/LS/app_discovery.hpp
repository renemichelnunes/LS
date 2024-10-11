#pragma once
#include <Arduino.h>
#include <vector>
#include <lvgl.h>

// App ID
#define APP_DISCOVERY 2

// LVGL objects
lv_obj_t frm_discovery;

struct grid_localization{
    char node_id[7] = {'\0'};
    uint16_t gridX = 0;
    uint16_t gridY = 0;
    uint16_t hops = 0;
};

class discovery_node{
    public:
        grid_localization gridLocalization;
        discovery_node(char * node_id, uint8_t hops, uint16_t gridX, uint16_t gridY);
};

class discovery_app{
    private:
        std::vector<discovery_node> list;
    public:
        bool exists(const char * node_id);
        bool add(discovery_node node);
        discovery_node * getNode(const char * node_id);
        uint32_t hopsTo(const char * node_id);
        void initUI(lv_obj_t * parent);
};

