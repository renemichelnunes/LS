#pragma once
#include <vector>
#include <Arduino.h>
#include <exception>
#include <WiFi.h>

#define LORA_PKT_EMPTY 0
#define LORA_PKT_ANNOUNCE 1
#define LORA_PKT_ACK 2
#define LORA_PKT_DATA 3
#define LORA_PKT_DATA_SMALL 4
#define LORA_PKT_COMM 5
#define LORA_PKT_PING 6
#define LORA_PKT_DATA_16 7
#define LORA_PKT_DATA_32 8
#define LORA_PKT_DATA_48 9
#define LORA_PKT_DATA_64 10
#define LORA_PKT_DATA_80 11
#define LORA_PKT_DATA_96 12
#define LORA_PKT_DATA_112 13
#define LORA_PKT_DATA_128 14
#define LORA_PKT_DATA_144 15
#define LORA_PKT_DATA_160 16
#define LORA_PKT_DATA_176 17
#define LORA_PKT_DATA_192 18
#define LORA_PKT_DATA_208 19

#define MAX_HOPS 200

/// @brief Struct that is used to create a shorter LoRa packet with announcement info.
struct lora_packet_announce{
    uint8_t type = LORA_PKT_ANNOUNCE;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    //char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
};
/// @brief Struct that is used to create a shorter LoRa packet with ack info.
struct lora_packet_ack{
    uint8_t type = LORA_PKT_ACK;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    char status[7] = "recv"; // can be used to ack (sender's message id)
    uint8_t app_id = 0;
};
/// @brief Struct that is used to create a shorter LoRa packet with command and parameters info.
struct lora_packet_comm{
    uint8_t type = LORA_PKT_COMM;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    uint8_t command;
    char param[160] = {'\0'};
};
/// @brief Struct that is used to create a shorter LoRa packet with ping info.
struct lora_packet_ping{
    uint8_t type = LORA_PKT_PING;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    char status[7] = "recv"; // can be used to ack (sender's message id)
};

/// @brief Struct that is used when we send messages.
struct lora_packet_data{
    uint8_t type = LORA_PKT_DATA;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    //char status[7] = {'\0'};
    char data[208] = {'\0'};
    uint8_t data_size = 0;
    uint8_t app_id = 0;
    uint32_t crc = 0;
};

/// @brief Struct that is used when we send messages 16 bytes.
struct lora_packet_data_16{
    uint8_t type = LORA_PKT_DATA_16;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    //char status[7] = {'\0'};
    char data[16] = {'\0'};
    uint8_t data_size = 0;
    uint8_t app_id = 0;
    uint32_t crc = 0;
};

/// @brief Struct that is used when we send messages 32 bytes.
struct lora_packet_data_32{
    uint8_t type = LORA_PKT_DATA_32;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    //char status[7] = {'\0'};
    char data[32] = {'\0'};
    uint8_t data_size = 0;
    uint8_t app_id = 0;
    uint32_t crc = 0;
};

/// @brief Struct that is used when we send messages 48 bytes.
struct lora_packet_data_48{
    uint8_t type = LORA_PKT_DATA_48;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    //char status[7] = {'\0'};
    char data[48] = {'\0'};
    uint8_t data_size = 0;
    uint8_t app_id = 0;
    uint32_t crc = 0;
};

/// @brief Struct that is used when we send messages 64 bytes.
struct lora_packet_data_64{
    uint8_t type = LORA_PKT_DATA_64;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    //char status[7] = {'\0'};
    char data[64] = {'\0'};
    uint8_t data_size = 0;
    uint8_t app_id = 0;
    uint32_t crc = 0;
};

/// @brief Struct that is used when we send messages 80 bytes.
struct lora_packet_data_80{
    uint8_t type = LORA_PKT_DATA_80;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    //char status[7] = {'\0'};
    char data[80] = {'\0'};
    uint8_t data_size = 0;
    uint8_t app_id = 0;
    uint32_t crc = 0;
};

/// @brief Struct that is used when we send messages 96 bytes.
struct lora_packet_data_96{
    uint8_t type = LORA_PKT_DATA_96;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    //char status[7] = {'\0'};
    char data[96] = {'\0'};
    uint8_t data_size = 0;
    uint8_t app_id = 0;
    uint32_t crc = 0;
};

/// @brief Struct that is used when we send messages 112 bytes.
struct lora_packet_data_112{
    uint8_t type = LORA_PKT_DATA_112;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    //char status[7] = {'\0'};
    char data[112] = {'\0'};
    uint8_t data_size = 0;
    uint8_t app_id = 0;
    uint32_t crc = 0;
};

/// @brief Struct that is used when we send messages 128 bytes.
struct lora_packet_data_128{
    uint8_t type = LORA_PKT_DATA_128;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    //char status[7] = {'\0'};
    char data[128] = {'\0'};
    uint8_t data_size = 0;
    uint8_t app_id = 0;
    uint32_t crc = 0;
};

/// @brief Struct that is used when we send messages 144 bytes.
struct lora_packet_data_144{
    uint8_t type = LORA_PKT_DATA_144;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    //char status[7] = {'\0'};
    char data[144] = {'\0'};
    uint8_t data_size = 0;
    uint8_t app_id = 0;
    uint32_t crc = 0;
};

/// @brief Struct that is used when we send messages 160 bytes.
struct lora_packet_data_160{
    uint8_t type = LORA_PKT_DATA_160;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    //char status[7] = {'\0'};
    char data[160] = {'\0'};
    uint8_t data_size = 0;
    uint8_t app_id = 0;
    uint32_t crc = 0;
};

/// @brief Struct that is used when we send messages 176 bytes.
struct lora_packet_data_176{
    uint8_t type = LORA_PKT_DATA_176;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    //char status[7] = {'\0'};
    char data[176] = {'\0'};
    uint8_t data_size = 0;
    uint8_t app_id = 0;
    uint32_t crc = 0;
};

/// @brief Struct that is used when we send messages 192 bytes.
struct lora_packet_data_192{
    uint8_t type = LORA_PKT_DATA_192;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    //char status[7] = {'\0'};
    char data[192] = {'\0'};
    uint8_t data_size = 0;
    uint8_t app_id = 0;
    uint32_t crc = 0;
};

/// @brief Struct that is used when we send messages 208 bytes.
struct lora_packet_data_208{
    uint8_t type = LORA_PKT_DATA_208;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    //char status[7] = {'\0'};
    char data[208] = {'\0'};
    uint8_t data_size = 0;
    uint8_t app_id = 0;
    uint32_t crc = 0;
};

/// @brief Struct that is used when we send messages.
struct lora_packet_data_small{
    uint8_t type = LORA_PKT_DATA_SMALL;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    //char status[7] = {'\0'};
    char data[64] = {'\0'};
    uint8_t data_size = 0;
    uint8_t app_id = 0;
    uint32_t crc = 0;
};

/// @brief Struct to create a complete LoRa packet info.
struct lora_packet{
    uint8_t type = 0;
    char id[7] = {'\0'};
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    uint8_t hops = MAX_HOPS;
    char status[7] = {'\0'};
    char data[208] = {'\0'};
    uint8_t data_size = 0;
    uint8_t app_id = 0;
    char date_time[30] = {'\0'};
    bool confirmed = false;
    uint32_t timeout = 0;
    uint32_t crc = 0;
    float rssi;
    float snr;
};

std::string generate_ID(uint8_t size);
uint32_t calculate_data_crc(const void * data, size_t length);

/// @brief Class to instatiate a queue of inscomming lora_packet gathered from the Três Livros de Filosofia OcultaLoRa radio and managing functions.
class lora_incomming_packets{
    private:
        // Vector that simulates a queue of received lora_packets.
        std::vector<lora_packet> lora_packets;
        uint8_t max = 200;
    public:
        // Adds a lora_packet to the queue.
        void add(lora_packet pkt);
        // Returns the first removed lora_packet from the queue.
        lora_packet get();
        // Checks if a received lora_packet already exists by ID.
        bool has_packets();
};

/// @brief Class to instatiate a queue of lora_packet and functions to manage a transmission using the LoRa radio transmission function.
class lora_outgoing_packets{
    private:
        // Vector that represents a queue of lora_packet to transmit.
        std::vector<lora_packet> lora_packets;
        // LoRa radio transmit callback function.
        int16_t (*transmit_func_callback)(uint8_t *, size_t);
        int16_t (*finish_transmit_func_callback)();
        uint32_t time_on_air = 0;
        uint8_t max = 30;
    public:
        // Instatiate a lora_outgoing_packets object passing a LoRa radio transmit function.
        lora_outgoing_packets(int16_t (*transmit_func_callback)(uint8_t *, size_t), int16_t (*finish_transmit_func_callback)());
        // Add generic packets to the transmit queue.
       void add(lora_packet pkt);
        // Loops through the queue for packets to being transmited, returns a copy of a lora_packet transmitted or a empty one.
        lora_packet check_packets();
        // Delete a lora_packet from the transmission queue.
        bool del(const char * id);
        // True if the queue has packets.
        bool has_packets();
        // Returns in milliseconds a calculated time out between two numbers in seconds
        uint32_t genPktTimeout(uint16_t seconds);
        void setOnAirTime(uint32_t time);
        bool hasType(uint8_t lora_pkt_type);
};
/// @brief Class to instantiate a queue of lora_packet IDs used as history of packets that already have passed through the node.
class lora_pkt_history{
    private:
        // Vector that holds the IDs.
        std::vector<String> history;
        // Vector max capacity.
        uint8_t max = 30;
    public:
        // Adds a packet ID to the list.
        bool add(char * pkt_id);
        // Checks if a packet ID already exists.
        bool exists(char * pkt_id);
};