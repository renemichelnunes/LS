#pragma once
#include <string>
#include <vector>
#include <Arduino.h>

using namespace std;

class Contact{
    public:
        Contact( String name,  String lora_address);
        Contact();
        bool operator==( Contact other);
        String getName();
        String getLoraAddress();
        void setName(String name);
        void setLAddr(String laddr);
    private:
        String name;
        String lora_address;
};

class Contact_list{
    private:
        vector <Contact> list;
    public:
        bool add(Contact c);
        bool del(Contact c);
        bool find(Contact &c);
        Contact getContact(uint32_t index);
        Contact* getContactByName(String name);
        uint32_t size();
        Contact_list();
        ~Contact_list();
};
