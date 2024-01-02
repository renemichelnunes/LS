#include <lora_messages.hpp>
#include <stdio.h>
#include <cstring>

lora_incomming_messages::lora_incomming_messages()
{
    
}

lora_incomming_messages::~lora_incomming_messages()
{

}

int inc = 0;
bool lora_incomming_messages::addMessage(lora_packet packet, bool newMsg)
{   
    try{
        uint32_t index = find(packet.id);
        if(index == -1){//New contact
            Serial.println("new contact messages");
            lora_contact_messages a;
            strcpy(a.id, packet.id);
            a.newMessages = newMsg;
            a.messages.push_back(packet);
            Serial.println(a.id);
            this->contacts_messages.push_back(a);
        }else{//exsting contact
            Serial.println("existing contact messages");
            this->contacts_messages[index].messages.push_back(packet);
            this->contacts_messages[index].newMessages = newMsg;
        }
    }
    catch (std::exception e){
        return false;
    }
    inc++;
    Serial.print("inc ");
    Serial.println(inc);
    return true;
}

uint32_t lora_incomming_messages::find(char *id)
{
    if(this->contacts_messages.size() == 0)
        return -1;
    for(uint32_t i = 0; i < contacts_messages.size(); i++){
        if(strcmp(this->contacts_messages[i].id, id) == 0)
            return i;
    }
    return -1;
}

std::vector<lora_packet> lora_incomming_messages::getMessages(char *id)
{
    uint32_t index = find(id);
    if(index != -1){
        this->contacts_messages[index].newMessages = false;
        return this->contacts_messages[index].messages;
    }
    //Serial.println("getMessages empty");
    return std::vector<lora_packet>();
}

