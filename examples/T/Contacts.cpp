#include "Contacts.hpp"
#include <iterator>
#include <vector>
#include <algorithm>
#include <iostream>

Contact::Contact(String name, String id){
    this->name = name;
    this->id = id;
}

Contact::Contact(){
    
}

String Contact::getName(){
    return this->name;
}

String Contact::getID(){
    return this->id;
}

bool Contact::operator==( Contact other){
    return (name == other.name);
}

void Contact::setName(String name){
    this->name = name;
}

void Contact::setID(String id){
    this->id = id;
}

static bool cmp_name( Contact &c1,  Contact &c2){
    return c1.getName() < c2.getName();
}

bool Contact_list::add(Contact c){
    try{
        if(this->find(c))
            return false;
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
    return this->list.size();
}

Contact  Contact_list::getContact(uint32_t index){
    try{
        return list.at(index);
    }
    catch(exception e){
        Serial.println(e.what());
        return Contact();
    }
}

Contact * Contact_list::getContactByName(String name){
    Contact c = Contact(name, "");
    std::vector<Contact>::iterator it = std::find(this->list.begin(), this->list.end(), c);
    if(it != this->list.end()){
        return &this->list[std::distance(this->list.begin(), it)];
    }
    return NULL;
}

Contact *Contact_list::getContactByID(String id)
{
    if(list.size() > 0){
        for(uint32_t i = 0; i < list.size(); i++)
            if(strcmp(list[i].getID().c_str(), id.c_str()) == 0)
                return &this->list[i];
    }
    return NULL;
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
        c = this->getContact(std::distance(this->list.begin(), it));
        return true;
    }
    else
        return false;
}

vector <Contact> Contact_list::getList(){
    return list;
}

void Contact_list::check_inrange(){
    if(this->list.size() > 0){
        for(uint32_t index = 0; index < this->list.size(); index++){
            if(millis() - this->list[index].timeout >= this->check_period){
                this->list[index].inrange = false;
                this->list[index].timeout += this->check_period;
            }
        }
    }
}

void Contact_list::setCheckPeriod(uint8_t min){
    if(min >= 1)
        this->check_period = min * 60 * 1000L;
}