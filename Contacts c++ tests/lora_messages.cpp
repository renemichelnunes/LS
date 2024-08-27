#include <lora_messages.hpp>
#include <stdio.h>
#include <cstring>

lora_incomming_messages::lora_incomming_messages()
{
    
}

lora_incomming_messages::~lora_incomming_messages()
{

}
/// @brief Add a LoRa packet to the contact's list of packets.
/// @param packet 
/// @return bool
bool lora_incomming_messages::addMessage(lora_packet packet)
{   
    try{
        // We use the destiny ID(contact). ID == -1 means the contact has no packets in his list.
        uint32_t index = find(packet.destiny);
        if(index == -1){//New contact
            Serial.println("new contact messages");
            // Create a new entry on the list.
            lora_contact_messages a;
            // Set the contact's ID.
            strcpy(a.id, packet.destiny);
            // Add the packet to his list of packets.
            a.messages.push_back(packet);
            Serial.println(a.id);
            // Add the contact and his list of packets to the list of contacts.
            this->contacts_messages.push_back(a);
        }else{//exsting contact
            Serial.println("existing contact messages");
            // If we have the index of a contact, just add the packet to his list of packets.
            this->contacts_messages[index].messages.push_back(packet);
        }
    }
    catch (std::exception e){
        return false;
    }
    return true;
}
/// @brief Return a index of a contact that has messages. Returns -1 if the contact has no packets on his list.
/// @param id 
/// @return uint32_t
uint32_t lora_incomming_messages::find(const char *id)
{
    if(this->contacts_messages.size() == 0)
        return -1;
    for(uint32_t i = 0; i < contacts_messages.size(); i++){
        if(strcmp(this->contacts_messages[i].id, id) == 0)
            return i;
    }
    return -1;
}
/// @brief Return a list of LoRa packets related to the contact.
/// @param id 
/// @return vector<lora_packet>
std::vector<lora_packet> lora_incomming_messages::getMessages(const char *id)
{
    // If the contact has packets, return the entire list, otherwise return an empty list.
    uint32_t index = find(id);
    if(index != -1){
        return this->contacts_messages[index].messages;
    }
    return std::vector<lora_packet>();
}

void lora_incomming_packets::add(lora_packet pkt){
    try{
        this->lora_packets.push_back(pkt);
    }
    catch(std::exception &e){
        Serial.println("lora_incomming_packets::add error");
        Serial.println(e.what());
    }
}

lora_packet lora_incomming_packets::get(){
    lora_packet p = this->lora_packets[0];
    this->lora_packets.erase(this->lora_packets.begin());
    return p;
}

bool lora_incomming_packets::has_packets(){
    return this->lora_packets.size() > 0;
}