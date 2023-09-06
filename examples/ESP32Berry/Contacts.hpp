#pragma once
#include <string>
#include <vector>

using namespace std;

class Contact{
    public:
        Contact(const string &name, const string &lora_address);
        Contact();
        bool operator==(const Contact &other)const;
        string getName()const;
        string getLoraAddress()const;
        void setName(string name);
        void setLAddr(string laddr);
    private:
        string name;
        string lora_address;
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
