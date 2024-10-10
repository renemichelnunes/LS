#pragma once
#include <Arduino.h>
#include <vector>

#define APP_DISCOVERY 1

struct node_vicinity{
    char node_id[7] = {'\0'};
    char nb1[7] = {'\0'};
    char nb1[7] = {'\0'};
    char nb1[7] = {'\0'};
    char nb1[7] = {'\0'};
    char nb1[7] = {'\0'};
    char nb1[7] = {'\0'};
    char nb1[7] = {'\0'};
    char nb1[7] = {'\0'};
};

class discovery_node{
    public:
        char node_id[7];
        uint8_t hops;
        discovery_node(char * node_id, uint8_t hops);
};

class discovery_app{
    private:
        std::vector<discovery_node> list;
    public:
        bool exists(const char * node_id);
        bool add(discovery_node node);
        discovery_node * getNode(const char * node_id);
        uint32_t estimatedDeliveryTime(const char * node_id);
        uint32_t estimatedResponseTime(const char * node_id);
};