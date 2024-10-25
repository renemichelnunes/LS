#include <lora_messages.hpp>
#include <stdio.h>
#include <cstring>

void lora_incomming_packets::add(lora_packet pkt){
    try{
        if(this->lora_packets.size() == this->max){
            Serial.println("Receiving queue full, erasing the oldest");
            this->lora_packets.erase(this->lora_packets.begin());
        }
        this->lora_packets.push_back(pkt);
        //Serial.printf("Receiving queue has %d elements\n", this->lora_packets.size());
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
        r = rand() % (seconds*10);
        if(r < 10)
            r += 10;
        r *= 100;
    }
    //Serial.printf("genPktTimeout %lu\n", r);
    return r;
}

void lora_outgoing_packets::setOnAirTime(uint32_t time)
{
    this->time_on_air = time;
}

bool lora_outgoing_packets::hasType(uint8_t lora_pkt_type)
{
    for(lora_packet pkt : this->lora_packets)
        if(pkt.type == LORA_PKT_ANNOUNCE)
            return true;
    return false;
}

void lora_outgoing_packets::add(lora_packet pkt){
    if(this->lora_packets.size() == this->max){
        Serial.println("Transmitting queue full, erasing the oldest");
        this->lora_packets.erase(this->lora_packets.begin());
    }
    this->lora_packets.push_back(pkt);
    //Serial.printf("Transmission queue has %d elements\n", this->lora_packets.size());
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
    static const char alphanum[] = "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < size; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    return s;
}

uint32_t calculate_data_crc(const void * data, size_t length){
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *byte_data = (const uint8_t *)data;

    for (size_t i = 0; i < length; i++) {
        crc ^= byte_data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320; // Polinômio reverso
            } else {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

lora_packet lora_outgoing_packets::check_packets(){
    uint32_t r = 100;
    uint32_t pkt_size = 0;
    lora_packet p;
    void * packet = NULL;

    if(this->has_packets()){
        for(int i = 0; i < this->lora_packets.size(); i++){
            p = lora_packets[i];
            if(p.timeout < millis()){
                // Creating a packet by type
                if(p.type == LORA_PKT_ANNOUNCE){
                    pkt_size = sizeof(lora_packet_announce);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_announce * ann = (lora_packet_announce *)packet;
                        strcpy(ann->id, p.id);
                        strcpy(ann->sender, p.sender);
                        //strcpy(ann->destiny, p.destiny);
                        ann->type = LORA_PKT_ANNOUNCE;
                        ann->hops = p.hops;
                        //Serial.println("Announcement packet ready");
                    }
                    else
                        Serial.println("Announcement packet not ready");
                }
                else if(p.type == LORA_PKT_ACK){
                    pkt_size = sizeof(lora_packet_ack);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_ack * ack = (lora_packet_ack *)packet;
                        strcpy(ack->id, p.id);
                        strcpy(ack->sender, p.sender);
                        strcpy(ack->destiny, p.destiny);
                        strcpy(ack->status, p.status);
                        ack->type = LORA_PKT_ACK;
                        ack->app_id = p.app_id;
                        ack->hops = p.hops;
                        Serial.println("ACK packet ready");
                    }
                    else{
                        Serial.println("ACK packet not ready");
                    }
                }
                else if(p.type == LORA_PKT_DATA){
                    pkt_size = sizeof(lora_packet_data);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_data * data = (lora_packet_data*)packet;
                        strcpy(data->id, p.id);
                        strcpy(data->sender, p.sender);
                        strcpy(data->destiny, p.destiny);
                        //strcpy(data->status, p.status);
                        memcpy(data->data, p.data, p.data_size);
                        data->data_size = p.data_size;
                        data->crc = calculate_data_crc(p.data, 208);
                        data->type = LORA_PKT_DATA;
                        data->app_id = p.app_id;
                        data->hops = p.hops;
                        Serial.println("Data packet ready");
                    }
                    else{
                        Serial.println("Data packet not ready");
                    }
                }
                else if(p.type == LORA_PKT_PING){

                }
                else if(p.type == LORA_PKT_COMM){

                }

                // Transmit the packet.
                if(p.type != LORA_PKT_EMPTY){
                    lora_packet_data * pp = (lora_packet_data*)packet;
                    if(pp->type == LORA_PKT_DATA){
                        //Serial.println("TRANSMITTING");
                        //Serial.printf("ID %s\nType %d\nAPP ID %d\nSender %s\nDestiny %s\nData size %d\nData %s\n\n", pp->id, pp->type, pp->app_id, pp->sender, pp->destiny, pp->data_size, pp->data);
                    }
                    else if(pp->type == LORA_PKT_ACK){
                        lora_packet_ack * pack = (lora_packet_ack*)packet;
                        //Serial.println("TRANSMITTING");
                        //Serial.printf("ID %s\nType %d\nAPP ID %d\nSender %s\nDestiny %s\nStatus %s\n\n", pack->id, pack->type, pack->app_id, pack->sender, pack->destiny, pack->status);
                    }
                    this->transmit_func_callback((uint8_t*)packet, pkt_size);
                    r = this->time_on_air + this->genPktTimeout(6);
                    Serial.printf("TX time %1.3fs\n--------------------------------\n", (float)this->time_on_air / 1000);
                    Serial.printf("Next transmission in %1.1fs\n--------------------------------\n", (float)r / 1000);
                    if(packet){
                        free(packet);
                        packet = NULL;
                    }
                    
                    vTaskDelay(r / portTICK_PERIOD_MS);
                    if(!this->has_packets())
                        return lora_packet();
                    if(p.confirmed || p.type == LORA_PKT_ANNOUNCE || p.type == LORA_PKT_ACK){
                        //Serial.printf("Erasing ID %s\n", p.id);
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
    if(this->history.size() == this->max){
        Serial.println("History queue full, erasing the oldest");
        this->history.erase(this->history.begin());
    }
    try{
        this->history.push_back(s);
        //Serial.printf("History queue has %d elements\n", this->history.size());
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
