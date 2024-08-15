#pragma once
#include <string>
#include <vector>
#include <Arduino.h>

using namespace std;

struct ContactMessages{
    char messageID[7] = {'\0'};
    char senderID[7] = {'\0'};
    char dateTime[30] = {'\0'};
    char message[160] = {'\0'};
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
        void setName(String name);
        void setID(String id);
        void setKey(String key);
        bool inrange = false;
        uint32_t timeout = 0;
        // Routines to handle the messages
        bool addMessage(ContactMessages cm);
        ContactMessages * getMessageByID(char * id);
        bool existsMessage(char * id);
    private:
        String name;
        String id;
        String key;
        vector<ContactMessages> messages;
};
/// @brief Class that provides a list of contacts.
class Contact_list{
    private:
        vector <Contact> list;
        uint64_t check_period = (1 * 60 * 1000L);
    public:
        
        bool add(Contact c);
        bool del(Contact c);
        bool find(Contact &c);
        Contact getContact(uint32_t index);
        Contact * getContactByName(String name);
        Contact * getContactByID(String id);
        vector <Contact> getList();
        uint32_t size();
        void check_inrange();
        void setCheckPeriod(uint8_t min);
        Contact_list();
        ~Contact_list();
};
