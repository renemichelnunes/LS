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
        bool inrange = false;
        uint32_t timeout = 0;
    private:
        String name;
        String id;
};

class Contact_list{
    private:
        vector <Contact> list;
    public:
        uint64_t check_period = (5 * 60 * 1000L);
        bool add(Contact c);
        bool del(Contact c);
        bool find(Contact &c);
        Contact getContact(uint32_t index);
        Contact* getContactByName(String name);
        Contact * getContactByID(String id);
        vector <Contact> getList();
        uint32_t size();
        void check_inrange();
        Contact_list();
        ~Contact_list();
};
