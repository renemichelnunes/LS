#pragma once
#include <Arduino.h>
using namespace std;
#include <vector>

class Contact{
    public:
        Contact(const String &name, const String &lora_address);
        Contact();
        bool operator==(const Contact &other)const;
        String getName()const;
        String getLoraAddress()const;
    private:
        String name;
        String lora_address;
};

class Contact_list{
    private:
        vector <Contact> contact_list;
    public:
        bool add(Contact c);
        bool del(Contact c);
        bool find(Contact &c);
        Contact_list();
        ~Contact_list();
};
