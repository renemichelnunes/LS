#pragma once
#include "Arduino.h"
#include <vector>

class notification
{
private:
    
public:
    std::vector <char *> list;
    notification();
    ~notification();

    void add(char * msg);
    void pop(char * msg);
    uint32_t size();
};
