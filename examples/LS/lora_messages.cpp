#include <lora_messages.hpp>
#include <stdio.h>
#include <cstring>

void lora_incomming_packets::add(lora_packet pkt){
    try{
        this->lora_packets.push_back(pkt);
    }
    catch(std::exception &e){
        Serial.printf("lora_incomming_packets::add error - %s\n", e.what());
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

uint32_t lora_outgoing_packets::genPktTimeout(uint16_t seconds){
    uint32_t r = 100;
    if(seconds > 0){
        r = rand() % seconds*10;
        if(r < 10)
            r += 10;
        r *= 100;
    }
    return r;
}

bool lora_outgoing_packets::hasType(uint8_t lora_pkt_type)
{
    for(lora_packet pkt : this->lora_packets)
        if(pkt.type == LORA_PKT_ANNOUNCE)
            return true;
    return false;
}

void lora_outgoing_packets::add(lora_packet pkt){
    this->lora_packets.push_back(pkt);
}

bool lora_outgoing_packets::has_packets(){
    if(this->lora_packets.size() == 0)
        return false;
    else
        return true;
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
    this->lora_packets.clear();
    this->transmit_func_callback = transmit_func_callback;
}

/// @brief Creates a string with a sequence of 6 chars between letters and numbers randomly, the contact's id.
/// @param size 
/// @return std::string
std::string generate_ID(uint8_t size){
    //char * s = (char*)calloc(size + 1, sizeof(char));
    char s[size + 1] = {'\0'};
    srand(millis());
    static const char alphanum[] = "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < size; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    return s;
}

lora_packet lora_outgoing_packets::check_packets(){
    uint32_t r = 100;
    uint32_t pkt_size = 0;
    lora_packet p;

    if(this->has_packets()){
        for(int i = 0; i < this->lora_packets.size(); i++){
            p = lora_packets[i];
            if(p.timeout < millis()){
                // Creating a packet by type
                if(p.type == LORA_PKT_ANNOUNCE){
                    pkt_size = sizeof(lora_packet_announce);
                    Serial.println("Announcement packet ready");
                }
                else if(p.type == LORA_PKT_ACK){
                    pkt_size = sizeof(lora_packet_ack);
                    Serial.println("ACK packet ready");
                }
                else if(p.type == LORA_PKT_DATA){
                    pkt_size = sizeof(lora_packet_data);
                    Serial.println("Data packet ready");
                }
                else if(p.type == LORA_PKT_PING){

                }
                else if(p.type == LORA_PKT_COMM){

                }

                // Transmit the packet.
                if(p.type != LORA_PKT_EMPTY){
                    this->transmit_func_callback((uint8_t*)&p, pkt_size);
                    r = this->genPktTimeout(6);
                    Serial.printf("Next transmission in %1.1fs\n--------------------------------\n", (float)r / 1000);
                    vTaskDelay(r / portTICK_PERIOD_MS);
                    if(!this->has_packets())
                        return lora_packet();
                    if(p.confirmed || p.type == LORA_PKT_ANNOUNCE || p.type == LORA_PKT_ACK){
                        Serial.printf("Erasing ID %s\n", p.id);
                        this->lora_packets.erase(this->lora_packets.begin() + i);
                    }
                    else if(!p.confirmed){
                        
                    }
                }
            }
        }
    }
    return p;
}

bool lora_pkt_history::add(char * pkt_id){
    char s[7] = {'\0'};
    strcpy(s, pkt_id);
    if(this->history.size() > 20)
        this->history.erase(this->history.begin());
    try{
        this->history.push_back(s);
        return true;
    }catch(std::exception &e){
        Serial.printf("lora_pkt_history::add error - %s\n", e.what());
        return false;
    }
}

bool lora_pkt_history::exists(char * pkt_id){
    for(uint8_t i = 0; i < this->history.size(); i++){
        if(strcmp(this->history[i].c_str(), pkt_id) == 0)
            return true;
    }
    return false;
}
