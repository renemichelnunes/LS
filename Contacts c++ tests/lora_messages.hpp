#pragma once
#include <vector>
//#include <Arduino.h>
//#include <exception>

#include <cstdint>
#include <cstring>
#include <stdio.h>
#include <cstdlib>
#include <string>
#include <ctime>
#include <chrono>

#define LORA_PKT_EMPTY 0
#define LORA_PKT_ANNOUNCE 1
#define LORA_PKT_ACK 2
#define LORA_PKT_DATA 3
#define LORA_PKT_COMM 4
#define LORA_PKT_PING 5

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
    char data[80] = {'\0'};
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
};

/// @brief Class to instatiate a queue of inscomming lora_packet gathered from the LoRa radio and managing functions.
class lora_incomming_packets{
    private:
        // Vector that simulates a queue of received lora_packets.
        std::vector<lora_packet> lora_packets;
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
    public:
        // Instatiate a lora_outgoing_packets object passing a LoRa radio transmit function.
        lora_outgoing_packets(int16_t (*transmit_func_callback)(uint8_t *, size_t));
        // Add generic packets to the transmit queue.
        void add(lora_packet pkt);
        // Loops through the queue for packets to being transmited, returns a copy of a lora_packet transmitted or a empty one.
        lora_packet check_packets();
        // Delete a lora_packet from the transmission queue.
        bool del(const char * id);
        // True if the queue has packets.
        bool has_packets();
        // Generate a timeout in ms
        uint16_t genPktTimeout(uint16_t t1, uint16_t t2);
};
/// @brief Class to instantiate a queue of lora_packet IDs used as history of packets that already have passed through the node.
class lora_pkt_history{
    private:
        // Vector that holds the IDs.
        std::vector<char *> history;
        // Vector max capacity.
        uint8_t max = 20;
    public:
        // Adds a packet ID to the list.
        bool add(char * pkt_id);
        // Checks if a packet ID already exists.
        bool exists(char * pkt_id);
};

