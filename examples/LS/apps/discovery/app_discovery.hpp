#pragma once
#include <Arduino.h>
#include <vector>
#include <lvgl.h>
#include <algorithm>
// App ID
#define APP_DISCOVERY 3
#define MAX_NEIGHBORS_COUNT 4
#define ITERATIONS 600
#define REPULSION_FORCE 200
#define ATTRACTION_FORCE 0.01

struct disc_node{
    char id[7] = {'\0'};
    char neighbors[MAX_NEIGHBORS_COUNT][7] = {{'\0'}};
};

class discovery_node{
    public:
        disc_node node;
        uint16_t hops = 0;
        discovery_node(uint8_t hops, disc_node dn);
};

class discovery_app{
    private:
        typedef struct{
            char id[7];
            double x;
            double y;
        }Node;

        std::vector<Node> nodes;

        void calculate_repulsion(double * dx, double * dy, uint16_t i, uint16_t j);
        void calculate_attraction(double * dx, double * dy, int i, int j);

        // LVGL objects
        // Main app
        lv_obj_t * parent;
        lv_obj_t * frm_discovery;
        lv_obj_t * frm_discovery_nodeList;
        lv_obj_t * frm_discovery_btn_title;
        lv_obj_t * frm_discovery_btn_title_lbl;
        lv_obj_t * frm_discovery_btn_back;
        lv_obj_t * frm_discovery_btn_back_lbl;
        lv_obj_t * frm_discovery_btn_graph;
        lv_obj_t * frm_discovery_btn_graph_lbl;

        // Graph form
        lv_obj_t * frm_graph_main;
        lv_obj_t * frm_graph_main_btn_back;
        lv_obj_t * frm_graph_main_btn_back_lbl;
        lv_obj_t * frm_graph_frame;

        lv_obj_t * createNodeListObj(lv_obj_t * btn, const char * node_id, uint16_t hops);

        pthread_mutex_t * lvgl_mutex;

        bool new_node;
        std::vector<discovery_node> list;

        bool ** adjacency_matrix;
    public:
        discovery_app(pthread_mutex_t * lvgl_mutex);
        bool exists(const char * node_id);
        bool add(discovery_node node);
        discovery_node * getNode(const char * node_id);
        uint16_t hopsTo(const char * node_id);
        // Main UI
        void showUI();
        void hideUI();
        void initUI(lv_obj_t * parent);
        void updateNodeList();

        // Graph UI
        void init_graph_ui();
        void show_graph_ui();
        void hide_graph_ui();
        void update_positions();
        void draw_graph();
        std::vector<discovery_node> list_demo;
        bool init_adj_matrix();
        bool clear_adj_matrix();
        void calculate_adj_matrix();
        bool init_positions();
};

