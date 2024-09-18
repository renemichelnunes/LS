#include <lora_messages.hpp>
#include <stdio.h>
#include <cstring>

void lora_incomming_packets::add(lora_packet pkt){
    try{
        this->lora_packets.push_back(pkt);
    }
    catch(std::exception &e){
        Serial.println("lora_incomming_packets::add error ");
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

void lora_outgoing_packets::add(lora_packet pkt){
    this->lora_packets.push_back(pkt);
}

lora_packet lora_outgoing_packets::get(){
    
}

bool lora_pkt_history::add(char * pkt_id){
    if(this->history.size() > 20)
        this->history.erase(this->history.begin());
    try{
        this->history.push_back(pkt_id);
        return true;
    }catch(std::exception &e){
        Serial.print("lora_pkt_history::add error ");
        Serial.println(e.what());
        return false;
    }
}

bool lora_pkt_history::exists(char * pkt_id){
    for(uint8_t i = 0; i < this->history.size(); i++)
        if(strcmp(this->history[i], pkt_id) == 0)
            return true;
    return false;
}
