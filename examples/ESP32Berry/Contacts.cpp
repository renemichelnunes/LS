#include "Contacts.hpp"
#include <iterator>
#include <vector>
#include <algorithm>
#include <iostream>

Contact::Contact(const string &name, const string &lora_address){
    this->name = name;
    this->lora_address = lora_address;
}

Contact::Contact(){

}

string Contact::getName()const{
    return this->name;
}

string Contact::getLoraAddress()const{
    return this->lora_address;
}

bool Contact::operator==(const Contact &other)const{
    return (name == other.name || lora_address == other.lora_address);
}

void Contact::setName(string name){
    this->name = name;
}

void Contact::setLAddr(string laddr){
    this->lora_address = laddr;
}

static bool cmp_name(const Contact &c1, const Contact &c2){
    return c1.getName() < c2.getName();
}

bool Contact_list::add(Contact c){
    try{
        this->contact_list.push_back(c);
        std::sort(this->contact_list.begin(), this->contact_list.end(), cmp_name);
    }
    catch(exception ex){
        return false;
    }
    return true;
}
Contact_list::Contact_list(){
    
}

Contact_list::~Contact_list(){
    this->contact_list.clear();
}

bool Contact_list::del(Contact c){
    std::vector<Contact>::iterator it = std::find(this->contact_list.begin(), this->contact_list.end(), c);
    if(it != this->contact_list.end()){
        this->contact_list.erase(it);
    }
    else
        return false;
    return true;
}

bool Contact_list::find(Contact &c){
    std::vector<Contact>::iterator it = std::find(this->contact_list.begin(), this->contact_list.end(), c);
    if(it != this->contact_list.end()){
        c = this->contact_list[std::distance(this->contact_list.begin(), it)];
        return true;
    }
    else
        return false;
}

void Contact_list::print(){
    if(this->contact_list.size() == 0)
        return;
    for(uint32_t i = 0; i < this->contact_list.size(); i++){
        cout << this->contact_list[i].getName() << endl;
        cout << this->contact_list[i].getLoraAddress() << endl;
        cout << "---------------------------------------------" << endl;
    }
}
