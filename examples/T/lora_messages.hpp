#pragma once
#include <vector>
#include <Arduino.h>
#include <exception>

#define LORA_PKT_STATUS 0
#define LORA_PKT_DATA 1
#define LORA_PKT_COMM 2
#define LORA_PKT_ACK 3
#define LORA_PKT_PING 4

/// @brief Struct that is used to create a shorter LoRa packet with status info.
struct lora_packet_status_ack{
    uint8_t type = LORA_PKT_STATUS;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    char status[7] = "recv"; // can be used to ack (sender's message id)
    uint8_t hops = 10;
};

struct lora_packet_comm{
    uint8_t type = LORA_PKT_COMM;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = 10;
    uint8_t command;
    char param[160] = {'\0'};
};

struct lora_packet_ping{
    uint8_t type = LORA_PKT_PING;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    char status[7] = "recv"; // can be used to ack (sender's message id)
    uint8_t hops = 10;
};

/// @brief Struct that is used when we send messages.
struct lora_packet_data{
    uint8_t type = LORA_PKT_DATA;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    char status[7] = {'\0'};
    uint8_t hops = 3;
    char data[160] = {'\0'};
    uint8_t data_size = 0;
};

/// @brief Struct to create a complete LoRa packet info, saved in a list.
struct lora_packet{
    uint8_t type;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    char status[7] = {'\0'};
    uint8_t hops = 3;
    char data[160] = {'\0'};
    uint8_t data_size = 0;
    char date_time[30] = {'\0'};
    bool confirmed = false;
    uint32_t timeout = 0;
};

class lora_incomming_packets{
    private:
        std::vector<lora_packet> lora_packets;
    public:
        void add(lora_packet pkt);
        lora_packet get();
        bool has_packets();
};

class lora_pkt_history{
    private:
        std::vector<char *> history;
        uint8_t max = 20;
    public:
        bool add(char * pkt_id);
        bool exists(char * pkt_id);
};