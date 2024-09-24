#pragma once
#include <vector>
//#include <Arduino.h>
//#include <exception>

#include <cstdint>
#include <cstring>

#define LORA_PKT_ANNOUNCE 0
#define LORA_PKT_ACK 1
#define LORA_PKT_DATA 2
#define LORA_PKT_COMM 3
#define LORA_PKT_PING 4

/// @brief Struct that is used to create a shorter LoRa packet with announcement info.
struct lora_packet_announce{
    uint8_t type = LORA_PKT_ANNOUNCE;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = 10;
};
/// @brief Struct that is used to create a shorter LoRa packet with ack info.
struct lora_packet_ack{
    uint8_t type = LORA_PKT_ACK;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = 10;
    char status[7] = "recv"; // can be used to ack (sender's message id)
};
/// @brief Struct that is used to create a shorter LoRa packet with command and parameters info.
struct lora_packet_comm{
    uint8_t type = LORA_PKT_COMM;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = 10;
    uint8_t command;
    char param[160] = {'\0'};
};
/// @brief Struct that is used to create a shorter LoRa packet with ping info.
struct lora_packet_ping{
    uint8_t type = LORA_PKT_PING;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = 10;
    char status[7] = "recv"; // can be used to ack (sender's message id)
};

/// @brief Struct that is used when we send messages.
struct lora_packet_data{
    uint8_t type = LORA_PKT_DATA;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = 10;
    char status[7] = {'\0'};
    char data[208] = {'\0'};
    uint8_t data_size = 0;
};

/// @brief Struct to create a complete LoRa packet info.
struct lora_packet{
    uint8_t type = 0;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = 10;
    char status[7] = {'\0'};
    char data[208] = {'\0'};
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

class lora_outgoing_packets{
    private:
        std::vector<lora_packet> lora_packets;
        int16_t (*transmit_func_callback)(uint8_t *, size_t);
    public:
        lora_outgoing_packets(int16_t (*transmit_func_callback)(uint8_t *, size_t));
        void add(lora_packet pkt);
        lora_packet get();
        bool del(const char * id);
        void update_timeout();
};

class lora_pkt_history{
    private:
        std::vector<char *> history;
        uint8_t max = 20;
    public:
        bool add(char * pkt_id);
        bool exists(char * pkt_id);
};
