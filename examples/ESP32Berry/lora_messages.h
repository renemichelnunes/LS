#pragma once
#include <vector>

struct lora_packet{
    char id[7];
    char msg[200];
};

struct lora_contact_messages{
    char id[7];
    std::vector <lora_packet> messages;
};

class lora_incomming_messages{
  private:
    std::vector <lora_contact_messages> messages;
  public:
    bool addMessage(char * id, char * message);
    std::vector <lora_packet> getMessages(char *id);
};