#pragma once
#include <Arduino.h>
#include <vector>
#include <lvgl.h>
#include <algorithm>
// App ID
#define APP_DISCOVERY 2



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
        // LVGL objects
        lv_obj_t * parent;
        lv_obj_t * frm_discovery;
        lv_obj_t * frm_discovery_nodeList;
        lv_obj_t * frm_discovery_btn_title;
        lv_obj_t * frm_discovery_btn_title_lbl;
        lv_obj_t * frm_discovery_btn_back;
        lv_obj_t * frm_discovery_btn_back_lbl;
        void initUI();
        
        std::vector<discovery_node> list;
    public:
        discovery_app(lv_obj_t * parent);
        bool exists(const char * node_id);
        bool add(discovery_node node);
        discovery_node * getNode(const char * node_id);
        uint32_t hopsTo(const char * node_id);
        void showUI();
        void hideUI();
};

