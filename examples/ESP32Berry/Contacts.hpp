#pragma once
#include <string>
#include <vector>
#include <Arduino.h>

using namespace std;

class Contact{
    public:
        Contact( String name,  String id);
        Contact();
        bool operator==( Contact other);
        String getName();
        String getID();
        void setName(String name);
        void setID(String id);
    private:
        String name;
        String id;
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
