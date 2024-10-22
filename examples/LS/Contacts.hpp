#pragma once
#include <string>
#include <vector>
#include <Arduino.h>

using namespace std;

struct ContactMessage{
    char messageID[7] = {'\0'};
    char dateTime[30] = {'\0'};
    bool ack = false;
    bool me = false;
    char message[208] = {'\0'};
    float rssi = 0;
    float snr = 0;
};

/// @brief Class that represents a LoRa contact.
class Contact{
    public:
        Contact( String name,  String id);
        Contact();
        bool operator==( Contact other);
        String getName();
        String getID();
        String getKey();
        float getRSSI();
        float getSNR();
        void setName(String name);
        void setID(String id);
        void setKey(String key);
        void setRSSI(float rssi);
        void setSNR(float snr);
        bool inrange = false;
        uint32_t timeout = 0;
        // Routines to handle the messages
        bool addMessage(ContactMessage cm);
        ContactMessage * getMessageByID(const char * id);
        vector<ContactMessage> getMessagesList();
        bool existsMessage(const char * id);
    private:
        String name;
        String id;
        String key;
        float rssi;
        float snr;
        vector<ContactMessage> messages;
        uint8_t max_messages = 20;
};
/// @brief Class that provides a list of contacts.
class Contact_list{
    private:
        vector<Contact> list;
        uint64_t check_period = (1 * 60 * 1000L);
    public:
        bool add(Contact c);
        bool del(Contact c);
        bool find(Contact &c);
        Contact getContact(uint32_t index);
        Contact * getContactByName(String name);
        Contact * getContactByID(const char *  id);
        vector<Contact> * getContactsList();
        vector<ContactMessage> getContactMessages(const char * id);
        uint32_t size();
        void check_inrange();
        void setCheckPeriod(uint8_t min);
        Contact_list();
        ~Contact_list();
};
