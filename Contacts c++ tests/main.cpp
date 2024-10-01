#include "Contacts.hpp"
#include "lora_messages.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>

int16_t transmit(uint8_t * data, size_t len){
    printf("\ndata => %s\nlen => %lu\n", data, len);
    return 0;
}

void teste(int16_t (*transmit_func_callback)(uint8_t *, size_t)){
    transmit_func_callback((uint8_t*)"teste", 6);
}

int main(int argc, char** argv){
    Contact_list cl = Contact_list();
    Contact c, *c1;
    c.setName("René");
    c.setID("123456");

    cl.add(c);
    printf("%d\n", cl.find(c));
    ContactMessage cm;
    strcpy(cm.messageID, "000000");
    strcpy(cm.dateTime, "26/08/2024");
    strcpy(cm.message, "isto é um teste");
    cm.rssi = 12.5;
    cm.snr = 8.25;
    c1 = cl.getContactByID("123456");
    c1->addMessage(cm);
    c1 = NULL;
    c1 = cl.getContactByID("123456");
    if(c1 == NULL){
        printf("%s\n", "contato não encontrado");
        exit(11);
    }else
        printf("Contato encontrado - %s\n", (*c1).getName().c_str());
    vector<ContactMessage> * messages = c1->getMessagesList();
    for(int i = 0; i < messages->size(); i++){
        printf("vector messages = %s\n", (*messages)[i].message);
    }
    ContactMessage *cm1 = c1->getMessageByID("000000");
    printf("cm1 = %s\n", cm1->message);

    strcpy(cm1->message, "este teste funcionou");
    printf("cm1 altered = %s\n", cm1->message);

    messages = c1->getMessagesList();
    for(int i = 0; i < messages->size(); i++){
        printf("final = %s\n", (*messages)[i].message);
    }

    strcpy((*messages)[0].message, "asda adad a");
    messages = cl.getContactMessages("123456");
    for(int i = 0; i < messages->size(); i++){
        printf("final 2 = %s\n", (*messages)[i].message);
    }

    lora_incomming_packets lip;
    lora_packet lp;

    lp.type = 0x00;
    strcpy(lp.sender, "123456");
    strcpy(lp.destiny, "111111");
    strcpy(lp.status, "show");
    lp.hops = 2;
    printf("has packets %d\n", lip.has_packets());
    lip.add(lp);
    printf("has packets %d\n", lip.has_packets());
    lora_packet lp1 = lip.get();
    printf("Type %d\nsender %s\nDestiny %s\nStatus %s\n", lp1.type, lp1.sender, lp1.destiny, lp1.status);
    printf("has packets %d\n", lip.has_packets());

    struct tm datetime;
    time_t timestamp;

    datetime.tm_year = 2023 - 1900; // Number of years since 1900
    datetime.tm_mon = 12 - 1; // Number of months since January
    datetime.tm_mday = 17;
    datetime.tm_hour = 12;
    datetime.tm_min = 30;
    datetime.tm_sec = 1;
    // Daylight Savings must be specified
    // -1 uses the computer's timezone setting
    datetime.tm_isdst = -1;

    timestamp = mktime(&datetime);

    printf("%s\n", ctime(&timestamp));
    printf("%lu\n", timestamp);
    printf("%lu\n", sizeof(long));

    uint32_t pkt_size = sizeof(lora_packet_ack);
    void * pkt = malloc(pkt_size);
    memset(pkt, '\0', pkt_size);
    for(int i = 0; i < pkt_size; i++){
        printf("%d ", *((unsigned char*)(pkt+i)));
    }
    strcpy(((struct lora_packet_ack *)pkt)->id, "121212");
    strcpy(((struct lora_packet_ack *)pkt)->sender, "abcdef");
    strcpy(((struct lora_packet_ack *)pkt)->destiny, "aaaaaa");
    strcpy(((struct lora_packet_ack *)pkt)->status, "111222");
    ((struct lora_packet_ack *)pkt)->type = LORA_PKT_ACK;
    ((struct lora_packet_ack *)pkt)->hops = 10;
    printf("packet size - %d\nid - %s\nsender - %s\ndestiny - %s\nmessage id - %s\npacket type - %d\nhops - %d\n", pkt_size,((struct lora_packet_ack *)pkt)->id, ((struct lora_packet_ack *)pkt)->sender, ((struct lora_packet_ack *)pkt)->destiny, ((struct lora_packet_ack *)pkt)->status, ((struct lora_packet_ack *)pkt)->type, ((struct lora_packet_ack *)pkt)->hops);
    free(pkt);
    for(int i = 0; i < pkt_size; i++){
        printf("%d ", *((unsigned char*)(pkt+i)));
    }
    pkt = NULL;
    teste(transmit);
    lora_outgoing_packets lop = lora_outgoing_packets(transmit);
    printf("lora_packet_announce size => %lu\n", sizeof(lora_packet_announce));
    printf("lora_packet_ack size => %lu\n", sizeof(lora_packet_ack));
    printf("lora_packet_comm size => %lu\n", sizeof(lora_packet_comm));
    printf("lora_packet_ping size => %lu\n", sizeof(lora_packet_ping));
    printf("lora_packet_data size => %lu\n", sizeof(lora_packet_data));
    printf("lora_packet size => %lu\n", sizeof(lora_packet));

    lora_packet p, p2, p3;
    strcpy(p.id, "vadsdf");
    strcpy(p2.id, "abcdef");
    p2.type = LORA_PKT_DATA;
    p.type = LORA_PKT_ANNOUNCE;
	p.timeout = lop.genPktTimeout(1, 5);
	p2.timeout = lop.genPktTimeout(1, 5);
    lop.add(p);
    lop.add(p2);
    while(true){
        p3 = lop.check_packets();
        if(p3.type != LORA_PKT_EMPTY){
            printf("lop.has_packets() => %d\n", lop.has_packets());
            printf("sent packet id => %s\n\n", p3.id);
        }
    }
}
