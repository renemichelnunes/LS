#pragma once
#include <vector>
#include <Arduino.h>
#include <exception>
/// @brief Class that is used to create a shorter LoRa packet with status info.
struct lora_packet_status{
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    char status[7] = "recv";
    uint8_t hops = 3;
};
/// @brief Class to create a complete LoRa packet.
struct lora_packet{
    char sender[7] = {'\0'};
    char destiny[7] = {'\0'};
    char status[7] = {'\0'};
    uint8_t hops = 3;
    bool me = false;
    char msg[160] = {'\0'};
    uint8_t msg_size = 0;
    char date_time[30] = {'\0'};
    char rssi[7] = {'\0'};
    char snr[7] = {'\0'};
};
/// @brief Struct that holds a list of LoRa packets related to a contact.
struct lora_contact_messages{
    // ID of the owner.
    char id[7];
    // List of owner's LoRa packets.
    std::vector <lora_packet> messages;
};
/// @brief Struct that holds the statistics of a received lora transmission.
struct lora_stats{
  char rssi[7] = {'\0'};
  char snr[7] = {'\0'};
  
};
/// @brief Class to create a list of contacts's LoRa packets.
class lora_incomming_messages{
  private:
    // List of ID and its list of LoRa packets.
    std::vector <lora_contact_messages> contacts_messages;
  public:
    lora_incomming_messages();
    ~lora_incomming_messages();
    bool addMessage(lora_packet packet);
    uint32_t find(const char * id);
    std::vector <lora_packet> getMessages(const char * id);
};