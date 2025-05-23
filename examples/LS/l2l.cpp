#include <l2l.hpp>
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
        r = random() % (seconds*10);
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

lora_outgoing_packets::lora_outgoing_packets(int16_t (*transmit_func_callback)(uint8_t *, size_t), int16_t (*finish_transmit_func_callback)()){
    this->finish_transmit_func_callback = finish_transmit_func_callback;
    this->lora_packets.clear();
    this->transmit_func_callback = transmit_func_callback;
}

/// @brief Creates a string with a sequence of 6 chars between letters and numbers randomly, the contact's id.
/// @param size 
/// @return std::string
std::string generate_ID(uint8_t size){
    //char * s = (char*)calloc(size + 1, sizeof(char));
    bool wifi_was_off = false;
    // We need the wifi on to get RNG on esp_random, otherwise we'll get false random numbers
    if(WiFi.getMode() == WIFI_OFF){
        WiFi.mode(WIFI_STA);
        wifi_was_off = true;
    }
    char s[size + 1] = {'\0'};
    static const char alphanum[] = "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < size; ++i) {
        s[i] = alphanum[esp_random() % (sizeof(alphanum) - 1)];
    }
    if(wifi_was_off)
        WiFi.mode(WIFI_OFF);
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
                        Serial.printf("ACK packet to %s sender %s ready\n", ack->status, ack->sender);
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
                        memcpy(data->data, p.data, p.data_size);
                        data->data_size = p.data_size;
                        data->crc = calculate_data_crc(p.data, 208);
                        data->type = LORA_PKT_DATA;
                        data->app_id = p.app_id;
                        data->hops = p.hops;
                        Serial.printf("Data packet ID %s ready\n", data->id);
                    }
                    else{
                        Serial.println("Data packet not ready");
                    }
                }
                else if(p.type == LORA_PKT_DATA_SMALL){
                    pkt_size = sizeof(lora_packet_data_small);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_data_small * data = (lora_packet_data_small*)packet;
                        strcpy(data->id, p.id);
                        strcpy(data->sender, p.sender);
                        strcpy(data->destiny, p.destiny);
                        memcpy(data->data, p.data, p.data_size);
                        data->data_size = p.data_size;
                        data->crc = calculate_data_crc(p.data, 64);
                        data->type = LORA_PKT_DATA_SMALL;
                        data->app_id = p.app_id;
                        data->hops = p.hops;
                        Serial.printf("Small data packet ID %s ready\n", data->id);
                    }
                    else{
                        Serial.println("Small data packet not ready");
                    }
                }
                else if(p.type == LORA_PKT_DATA_16){
                    pkt_size = sizeof(lora_packet_data_16);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_data_16 * data = (lora_packet_data_16*)packet;
                        strcpy(data->id, p.id);
                        strcpy(data->sender, p.sender);
                        strcpy(data->destiny, p.destiny);
                        memcpy(data->data, p.data, p.data_size);
                        data->data_size = p.data_size;
                        data->crc = calculate_data_crc(p.data, 16);
                        data->type = LORA_PKT_DATA_16;
                        data->app_id = p.app_id;
                        data->hops = p.hops;
                        Serial.printf("lora_packet_data_16 data packet ID %s ready\n", data->id);
                    }
                    else{
                        Serial.println("lora_packet_data_16 data packet not ready");
                    }
                }
                else if(p.type == LORA_PKT_DATA_32){
                    pkt_size = sizeof(lora_packet_data_32);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_data_32 * data = (lora_packet_data_32*)packet;
                        strcpy(data->id, p.id);
                        strcpy(data->sender, p.sender);
                        strcpy(data->destiny, p.destiny);
                        memcpy(data->data, p.data, p.data_size);
                        data->data_size = p.data_size;
                        data->crc = calculate_data_crc(p.data, 32);
                        data->type = LORA_PKT_DATA_32;
                        data->app_id = p.app_id;
                        data->hops = p.hops;
                        Serial.printf("lora_packet_data_32 data packet ID %s ready\n", data->id);
                    }
                    else{
                        Serial.println("lora_packet_data_32 data packet not ready");
                    }
                }
                else if(p.type == LORA_PKT_DATA_48){
                    pkt_size = sizeof(lora_packet_data_48);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_data_48 * data = (lora_packet_data_48*)packet;
                        strcpy(data->id, p.id);
                        strcpy(data->sender, p.sender);
                        strcpy(data->destiny, p.destiny);
                        memcpy(data->data, p.data, p.data_size);
                        data->data_size = p.data_size;
                        data->crc = calculate_data_crc(p.data, 48);
                        data->type = LORA_PKT_DATA_48;
                        data->app_id = p.app_id;
                        data->hops = p.hops;
                        Serial.printf("lora_packet_data_48 data packet ID %s ready\n", data->id);
                    }
                    else{
                        Serial.println("lora_packet_data_48 data packet not ready");
                    }
                }
                else if(p.type == LORA_PKT_DATA_64){
                    pkt_size = sizeof(lora_packet_data_64);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_data_64 * data = (lora_packet_data_64*)packet;
                        strcpy(data->id, p.id);
                        strcpy(data->sender, p.sender);
                        strcpy(data->destiny, p.destiny);
                        memcpy(data->data, p.data, p.data_size);
                        data->data_size = p.data_size;
                        data->crc = calculate_data_crc(p.data, 64);
                        data->type = LORA_PKT_DATA_64;
                        data->app_id = p.app_id;
                        data->hops = p.hops;
                        Serial.printf("lora_packet_data_64 data packet ID %s ready\n", data->id);
                    }
                    else{
                        Serial.println("lora_packet_data_64 data packet not ready");
                    }
                }
                else if(p.type == LORA_PKT_DATA_80){
                    pkt_size = sizeof(lora_packet_data_80);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_data_80 * data = (lora_packet_data_80*)packet;
                        strcpy(data->id, p.id);
                        strcpy(data->sender, p.sender);
                        strcpy(data->destiny, p.destiny);
                        memcpy(data->data, p.data, p.data_size);
                        data->data_size = p.data_size;
                        data->crc = calculate_data_crc(p.data, 80);
                        data->type = LORA_PKT_DATA_80;
                        data->app_id = p.app_id;
                        data->hops = p.hops;
                        Serial.printf("lora_packet_data_80 data packet ID %s ready\n", data->id);
                    }
                    else{
                        Serial.println("lora_packet_data_80 data packet not ready");
                    }
                }
                else if(p.type == LORA_PKT_DATA_96){
                    pkt_size = sizeof(lora_packet_data_96);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_data_96 * data = (lora_packet_data_96*)packet;
                        strcpy(data->id, p.id);
                        strcpy(data->sender, p.sender);
                        strcpy(data->destiny, p.destiny);
                        memcpy(data->data, p.data, p.data_size);
                        data->data_size = p.data_size;
                        data->crc = calculate_data_crc(p.data, 96);
                        data->type = LORA_PKT_DATA_96;
                        data->app_id = p.app_id;
                        data->hops = p.hops;
                        Serial.printf("lora_packet_data_96 data packet ID %s ready\n", data->id);
                    }
                    else{
                        Serial.println("lora_packet_data_96 data packet not ready");
                    }
                }
                else if(p.type == LORA_PKT_DATA_112){
                    pkt_size = sizeof(lora_packet_data_112);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_data_112 * data = (lora_packet_data_112*)packet;
                        strcpy(data->id, p.id);
                        strcpy(data->sender, p.sender);
                        strcpy(data->destiny, p.destiny);
                        memcpy(data->data, p.data, p.data_size);
                        data->data_size = p.data_size;
                        data->crc = calculate_data_crc(p.data, 112);
                        data->type = LORA_PKT_DATA_112;
                        data->app_id = p.app_id;
                        data->hops = p.hops;
                        Serial.printf("lora_packet_data_112 data packet ID %s ready\n", data->id);
                    }
                    else{
                        Serial.println("lora_packet_data_112 data packet not ready");
                    }
                }
                else if(p.type == LORA_PKT_DATA_128){
                    pkt_size = sizeof(lora_packet_data_128);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_data_128 * data = (lora_packet_data_128*)packet;
                        strcpy(data->id, p.id);
                        strcpy(data->sender, p.sender);
                        strcpy(data->destiny, p.destiny);
                        memcpy(data->data, p.data, p.data_size);
                        data->data_size = p.data_size;
                        data->crc = calculate_data_crc(p.data, 128);
                        data->type = LORA_PKT_DATA_128;
                        data->app_id = p.app_id;
                        data->hops = p.hops;
                        Serial.printf("lora_packet_data_128 data packet ID %s ready\n", data->id);
                    }
                    else{
                        Serial.println("lora_packet_data_128 data packet not ready");
                    }
                }
                else if(p.type == LORA_PKT_DATA_144){
                    pkt_size = sizeof(lora_packet_data_144);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_data_144 * data = (lora_packet_data_144*)packet;
                        strcpy(data->id, p.id);
                        strcpy(data->sender, p.sender);
                        strcpy(data->destiny, p.destiny);
                        memcpy(data->data, p.data, p.data_size);
                        data->data_size = p.data_size;
                        data->crc = calculate_data_crc(p.data, 144);
                        data->type = LORA_PKT_DATA_144;
                        data->app_id = p.app_id;
                        data->hops = p.hops;
                        Serial.printf("lora_packet_data_144 data packet ID %s ready\n", data->id);
                    }
                    else{
                        Serial.println("lora_packet_data_144 data packet not ready");
                    }
                }
                else if(p.type == LORA_PKT_DATA_160){
                    pkt_size = sizeof(lora_packet_data_160);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_data_160 * data = (lora_packet_data_160*)packet;
                        strcpy(data->id, p.id);
                        strcpy(data->sender, p.sender);
                        strcpy(data->destiny, p.destiny);
                        memcpy(data->data, p.data, p.data_size);
                        data->data_size = p.data_size;
                        data->crc = calculate_data_crc(p.data, 160);
                        data->type = LORA_PKT_DATA_160;
                        data->app_id = p.app_id;
                        data->hops = p.hops;
                        Serial.printf("lora_packet_data_160 data packet ID %s ready\n", data->id);
                    }
                    else{
                        Serial.println("lora_packet_data_160 data packet not ready");
                    }
                }
                else if(p.type == LORA_PKT_DATA_176){
                    pkt_size = sizeof(lora_packet_data_176);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_data_176 * data = (lora_packet_data_176*)packet;
                        strcpy(data->id, p.id);
                        strcpy(data->sender, p.sender);
                        strcpy(data->destiny, p.destiny);
                        memcpy(data->data, p.data, p.data_size);
                        data->data_size = p.data_size;
                        data->crc = calculate_data_crc(p.data, 176);
                        data->type = LORA_PKT_DATA_176;
                        data->app_id = p.app_id;
                        data->hops = p.hops;
                        Serial.printf("lora_packet_data_176 data packet ID %s ready\n", data->id);
                    }
                    else{
                        Serial.println("lora_packet_data_176 data packet not ready");
                    }
                }
                else if(p.type == LORA_PKT_DATA_192){
                    pkt_size = sizeof(lora_packet_data_192);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_data_192 * data = (lora_packet_data_192*)packet;
                        strcpy(data->id, p.id);
                        strcpy(data->sender, p.sender);
                        strcpy(data->destiny, p.destiny);
                        memcpy(data->data, p.data, p.data_size);
                        data->data_size = p.data_size;
                        data->crc = calculate_data_crc(p.data, 192);
                        data->type = LORA_PKT_DATA_192;
                        data->app_id = p.app_id;
                        data->hops = p.hops;
                        Serial.printf("lora_packet_data_192 data packet ID %s ready\n", data->id);
                    }
                    else{
                        Serial.println("lora_packet_data_192 data packet not ready");
                    }
                }
                else if(p.type == LORA_PKT_DATA_208){
                    pkt_size = sizeof(lora_packet_data_208);
                    packet = calloc(1, pkt_size);
                    if(packet){
                        lora_packet_data_208 * data = (lora_packet_data_208*)packet;
                        strcpy(data->id, p.id);
                        strcpy(data->sender, p.sender);
                        strcpy(data->destiny, p.destiny);
                        memcpy(data->data, p.data, p.data_size);
                        data->data_size = p.data_size;
                        data->crc = calculate_data_crc(p.data, 208);
                        data->type = LORA_PKT_DATA_208;
                        data->app_id = p.app_id;
                        data->hops = p.hops;
                        Serial.printf("lora_packet_data_208 data packet ID %s ready\n", data->id);
                    }
                    else{
                        Serial.println("lora_packet_data_208 data packet not ready");
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
                    r = this->time_on_air*2 + this->genPktTimeout(6);
                    //Serial.printf("Tempo de espera %lu\n", r);
                    if(r < 5000)
                        r += 5000;
                    //Serial.printf("TX time %1.3fs\n--------------------------------\n", (floanational
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
        //Serial.println(F("History queue full, erasing the oldest"));
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
