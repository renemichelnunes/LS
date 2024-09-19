#include "lora_messages.hpp"
//#include <stdio.h>
#include <cstdint>
#include <cstring>

void lora_incomming_packets::add(lora_packet pkt){
    try{
        this->lora_packets.push_back(pkt);
    }
    catch(std::exception &e){
        //Serial.println("lora_incomming_packets::add error");
        //Serial.println(e.what());
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

lora_outgoing_packets::lora_outgoing_packets(int16_t (*transmit_func_callback)(uint8_t *, size_t)){
    this->transmit_func_callback = transmit_func_callback;
    this->transmit_func_callback((uint8_t*)"aaaaaaaaa", 10);
}
