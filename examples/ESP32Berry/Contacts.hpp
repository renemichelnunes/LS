#pragma once
#include <Arduino.h>

class Contact{
    public:
        String name;
        String lora_address;
};

class Contacts{
    private:
        /* data */
    public:
        Contacts(/* args */);
        ~Contacts();
};
