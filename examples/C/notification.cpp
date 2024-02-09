#include "notification.hpp"
#include "Arduino.h"

notification::notification()
{
}

notification::~notification()
{
}

void notification::add(char * msg, char * symbol){
    char n[300] = {'\0'};
    char b[13] = {'\0'};
    notification_struct notification;
    
    if(sizeof(msg) > 299)
        memcpy(n, msg, 299);
    strcpy(n, msg);
    strcpy(notification.msg, msg);
    if(sizeof(symbol) > 13)
        memcpy(b, symbol, 13);
    strcpy(b, symbol);
    strcpy(notification.msg, n);
    strcpy(notification.symbol, symbol);
    this->list.push_back(notification);
}

void notification::pop(char * msg, char * symbol){
    if(this->list.size() > 0){
        strcpy(msg, this->list[this->list.size() - 1].msg);
        strcpy(symbol, this->list[this->list.size() - 1].symbol);
        this->list.pop_back();
    }
}

uint32_t notification::size(){
    return this->list.size();
}