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

bool lora_outgoing_packets::has_packets(){
    return this->lora_packets.size() > 0;
}

bool lora_outgoing_packets::del(const char * id){
    if(this->has_packets())
        for(uint8_t i = 0; i < this->lora_packets.size(); i++)
            if(strcmp(this->lora_packets[i].id, id) == 0){
                this->lora_packets.erase(this->lora_packets.begin() + i);
                return true;
            }
        return false;
}

lora_outgoing_packets::lora_outgoing_packets(int16_t (*transmit_func_callback)(uint8_t *, size_t)){
    this->transmit_func_callback = transmit_func_callback;
}

void lora_outgoing_packets::update_timeout(){
    uint32_t r = 100;
    // Calculate in miliseconds between 1 and 5 seconds
    r = rand() % 50;
    if(r < 10)
        r += 10;
    r *= 100;
    // Remove the confirmed packets and set a new timeout for the unconfirmed ones.
    for(int i = 0; i < this->lora_packets.size(); i++){
        // If a message gets an ack, delete it, if not, update his timeout ack.
        if(this->lora_packets[i].confirmed){
            Serial.printf("processTransmitingPackets - %s confirmed", this->lora_packets[i].id);
            this->lora_packets.erase(this->lora_packets.begin() + i);
        }else if(this->lora_packets[i].timeout > millis() && !this->lora_packets[i].confirmed){ // If timedup and not confirmed, so renew the timeout (between 1 and 5 seconds, increments in hundreds of miliseconds)
            this->lora_packets[i].timeout = millis() + r;
        }
    }
}

lora_packet * lora_outgoing_packets::check_packets(){
    
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
