#pragma once
#include <vector>
#include <Arduino.h>
#include <exception>

#define lora_id char[7]

#define STATUS_PACKET 0x0
#define MESSAGE_PACKET 0x1
#define COMMAND_PACKET 0x2

/// @brief Struct that is used to create a shorter LoRa packet with status info.
struct lora_packet_status{
    char id[7] = {'\0'};
    uint8_t type = 0x0;
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    char status[7] = "recv";
    uint8_t hops = 3;
};

/// @brief Struct that is used when we send messages.
struct lora_packet_msg{
    char id[7] = {'\0'};
    uint8_t type = 0x1;
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    char status[7] = {'\0'};
    uint8_t hops = 3;
    char msg[160] = {'\0'};
    uint8_t msg_size = 0;
};

/// @brief Struct to create a complete LoRa packet info, saved in a list.
struct lora_packet{
    char id[7] = {'\0'};
    uint8_t type;
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    char status[7] = {'\0'};
    uint8_t hops = 3;
    char msg[160] = {'\0'};
    uint8_t msg_size = 0;
    char date_time[30] = {'\0'};
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