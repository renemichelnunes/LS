#pragma once
#include "Arduino.h"
#include <vector>

struct notification_struct{
    char msg[200] = {'\0'};
    char symbol[13] = {'\0'};
};

class notification
{
private:
    
public:
    std::vector <notification_struct> list;
    notification();
    ~notification();

    void add(char * msg, char * symbol);
    void pop(char * msg, char * symbol);
    uint32_t size();
};
