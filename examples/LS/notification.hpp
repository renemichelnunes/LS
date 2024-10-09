#pragma once
#include "Arduino.h"
#include <vector>
/// @brief Struct to hold a message and a symbol.
struct notification_struct{
    char msg[200] = {'\0'};
    char symbol[13] = {'\0'};
};
/// @brief Class to create a list of notification structs.
class notification
{
private:
    
public:
    std::vector <notification_struct> list;
    notification();
    ~notification();

    void add(const char * msg, const char * symbol);
    void pop(char * msg, char * symbol);
    uint32_t size();
};
