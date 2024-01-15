#include "notification.hpp"
#include "Arduino.h"

notification::notification()
{
}

notification::~notification()
{
}

void notification::add(char * msg){
    char n[300] = {'\0'};
    if(sizeof(msg) > 299)
        memcpy(n, n, 299);
    strcpy(n, msg);
    this->list.push_back(msg);
}

void notification::pop(char * msg){
    if(this->list.size() > 0){
        strcpy(msg, this->list[this->list.size() - 1]);
        this->list.pop_back();
    }
}

uint32_t notification::size(){
    return this->list.size();
}