#include "notification.hpp"
#include "Arduino.h"

notification::notification()
{
}

notification::~notification()
{
}
/// @brief Add a notification struct to the list.
/// @param msg 
/// @param symbol 
void notification::add(const char * msg, const char * symbol){
    char n[300] = {'\0'};
    char b[13] = {'\0'};
    notification_struct notification;
    // This is used to check if the message fits in 299 bytes, the last byte is a string finalizer.
    if(sizeof(msg) > 299)
        memcpy(n, msg, 299);
    strcpy(n, msg);
    strcpy(notification.msg, msg);
    // Same check fro the symbol.
    if(sizeof(symbol) > 13)
        memcpy(b, symbol, 13);
    strcpy(b, symbol);
    // Copy the message and symbol to the notification struct.
    strcpy(notification.msg, n);
    strcpy(notification.symbol, symbol);
    // Add to the list.
    this->list.push_back(notification);
}
/// @brief Get the message and symbol of the last notification before delete from the list.
/// @param msg 
/// @param symbol 
void notification::pop(char * msg, char * symbol){
    if(this->list.size() > 0){
        strcpy(msg, this->list[this->list.size() - 1].msg);
        strcpy(symbol, this->list[this->list.size() - 1].symbol);
        this->list.pop_back();
    }
}
/// @brief Return the size of the list.
/// @return uint32_t
uint32_t notification::size(){
    return this->list.size();
}