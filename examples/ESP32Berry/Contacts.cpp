#include "Contacts.hpp"
#include <iterator>
#include <vector>
#include <algorithm>
#include <iostream>

Contact::Contact(String name, String lora_address){
    this->name = name;
    this->lora_address = lora_address;
}

Contact::Contact(){

}

String Contact::getName(){
    return this->name;
}

String Contact::getLoraAddress(){
    return this->lora_address;
}

bool Contact::operator==( Contact other){
    return (name == other.name);
}

void Contact::setName(String name){
    this->name = name;
}

void Contact::setLAddr(String laddr){
    this->lora_address = laddr;
}

static bool cmp_name( Contact &c1,  Contact &c2){
    return c1.getName() < c2.getName();
}

bool Contact_list::add(Contact c){
    try{
        this->list.push_back(c);
        std::sort(this->list.begin(), this->list.end(), cmp_name);
    }
    catch(exception ex){
        return false;
    }
    return true;
}
Contact_list::Contact_list(){
    
}

Contact_list::~Contact_list(){
    this->list.clear();
}

uint32_t Contact_list::size(){
    return sizeof(this->list.size());
}

Contact  Contact_list::getContact(uint32_t index){
    return list.at(index);
}

bool Contact_list::del(Contact c){
    std::vector<Contact>::iterator it = std::find(this->list.begin(), this->list.end(), c);
    if(it != this->list.end()){
        this->list.erase(it);
    }
    else
        return false;
    return true;
}

bool Contact_list::find(Contact &c){
    std::vector<Contact>::iterator it = std::find(this->list.begin(), this->list.end(), c);
    if(it != this->list.end()){
        c = this->list[std::distance(this->list.begin(), it)];
        return true;
    }
    else
        return false;
}

