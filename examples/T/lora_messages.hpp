#pragma once
#include <vector>
#include <Arduino.h>
#include <exception>

struct lora_packet{
    char id[7] = {'\0'};
    bool me = false;
    char msg[200] = {'\0'};
    char status[7] = {'\0'};
};

struct lora_contact_messages{
    char id[7];
    bool newMessages = false;
    std::vector <lora_packet> messages;
};

class lora_incomming_messages{
  private:
    std::vector <lora_contact_messages> contacts_messages;
  public:
    lora_incomming_messages();
    ~lora_incomming_messages();
    bool addMessage(lora_packet packet, bool newMsg = true);
    uint32_t find(char * id);
    std::vector <lora_packet> getMessages(char * id);
};