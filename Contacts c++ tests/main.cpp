#include "Contacts.hpp"
#include "lora_messages.hpp"
#include <cstdio>
#include <ctime>
#include <iostream>

int main(int argc, char** argv){
    Contact_list cl;
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
}
