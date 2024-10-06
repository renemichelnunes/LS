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

uint32_t lora_outgoing_packets::genPktTimeout(uint16_t t1, uint16_t t2){
    uint32_t r = 100;
    if(t1 < t2){
        r = rand() % 50;
        if(r < 10)
            r += 10;
        r *= 100;
    }
    return r;
}

void lora_outgoing_packets::add(lora_packet pkt){
    pkt.timeout = genPktTimeout(1, 5);
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
static std::string generate_ID(uint8_t size){
  srand(time(NULL));
  static const char alphanum[] = "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  std::string ss;

  for (int i = 0; i < size; ++i) {
    ss[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }
  return ss;
}

lora_packet lora_outgoing_packets::check_packets(){
    uint32_t r = 100;
    void * pkt = NULL;
    uint32_t pkt_size = 0;
    lora_packet p;

    if(this->has_packets()){
        // Calculate in miliseconds between 1 and 5 seconds
        r = genPktTimeout(1, 5);
        // Remove the confirmed packets and set a new timeout for the unconfirmed ones.
        if(this->has_packets()){
            for(int i = 0; i < this->lora_packets.size(); i++){
                // If a message gets an ack, delete it, if not, update his timeout ack.
                if(this->lora_packets[i].confirmed){
                    Serial.printf("processTransmitingPackets - %s confirmed", this->lora_packets[i].id);
                    this->lora_packets.erase(this->lora_packets.begin() + i);
                }else if(this->lora_packets[i].timeout > millis() && !this->lora_packets[i].confirmed){ // If time is up and not confirmed, renew the timeout (between 1 and 5 seconds, increments in hundreds of miliseconds)
                    this->lora_packets[i].timeout = millis() + r;
                }
            }
        }
        
        if(this->has_packets()){
            for(int i = 0; i < this->lora_packets.size(); i++){
                if(this->lora_packets[i].timeout < millis()){
                    // Creating a packet by type
                    if(lora_packets[i].type == LORA_PKT_ANNOUNCE){
                        pkt_size = sizeof(lora_packet_announce);
                        pkt = calloc(pkt_size, sizeof(char));
                        // Generating packet info
                        p = lora_packets[i];
                        lora_packets.erase(lora_packets.begin() + i);
                        strcpy(((struct lora_packet_announce *)pkt)->sender, p.sender);
                        ((struct lora_packet_announce *)pkt)->type = p.type;
                        strcpy(((struct lora_packet_announce *)pkt)->id, generate_ID(6).c_str());
                        Serial.println("Announcement packet ready");
                    }
                    else if(lora_packets[i].type == LORA_PKT_ACK){
                        pkt_size = sizeof(lora_packet_ack);
                        pkt = calloc(pkt_size, sizeof(char));
                        // Generating packet info
                        p = lora_packets[i];
                        lora_packets.erase(lora_packets.begin() + i);
                        strcpy(((struct lora_packet_ack *)pkt)->id, generate_ID(6).c_str());
                        strcpy(((struct lora_packet_ack *)pkt)->sender, p.sender);
                        strcpy(((struct lora_packet_ack *)pkt)->destiny, p.destiny);
                        ((struct lora_packet_ack *)pkt)->type = p.type;
                        // destiny message ID to ack
                        strcpy(((struct lora_packet_ack *)pkt)->status, p.status); 
                        Serial.println("ACK packet ready");
                    }
                    else if(lora_packets[i].type == LORA_PKT_DATA){
                        pkt_size = sizeof(lora_packet_data);
                        pkt = calloc(pkt_size, sizeof(char));
                        // Generating packet info
                        p = lora_packets[i];
                        lora_packets.erase(lora_packets.begin() + i);
                        ((struct lora_packet_data *)pkt)->type = p.type;
                        strcpy(((struct lora_packet_data *)pkt)->id, generate_ID(6).c_str());
                        strcpy(((struct lora_packet_data *)pkt)->sender, p.sender);
                        strcpy(((struct lora_packet_data *)pkt)->destiny, p.destiny);
                        ((struct lora_packet_data *)pkt)->hops = 10;
                        memcpy(((struct lora_packet_data *)pkt)->data, p.data, p.data_size);
                        ((struct lora_packet_data *)pkt)->data_size = p.data_size;
                        Serial.println("Data packet ready");
                    }
                    else if(lora_packets[i].type == LORA_PKT_PING){

                    }
                    else if(lora_packets[i].type == LORA_PKT_COMM){

                    }

                    // Transmit the packet.
                    if(pkt != NULL){
                        this->transmit_func_callback((uint8_t*)pkt, pkt_size);
                        if(pkt){
                            free(pkt);
                            pkt = NULL;
                        }
                        if(!this->has_packets())
                            return lora_packet();
                        if(p.confirmed)
                            lora_packets.erase(lora_packets.begin() + i);
                    }
                }
            }
        }
    }
    return p;
}

bool lora_pkt_history::add(char * pkt_id){
    if(this->history.size() > 20)
        this->history.erase(this->history.begin());
    try{
        this->history.push_back(pkt_id);
        return true;
    }catch(std::exception &e){
        Serial.printf("lora_pkt_history::add error - %s\n", e.what());
        return false;
    }
}

bool lora_pkt_history::exists(char * pkt_id){
    for(uint8_t i = 0; i < this->history.size(); i++)
        if(strcmp(this->history[i], pkt_id) == 0)
            return true;
    return false;
}
