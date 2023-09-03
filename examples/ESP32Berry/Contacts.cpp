#include "Contacts.hpp"
#include <iterator>
#include <vector>
#include <algorithm>

Contact::Contact(const String &name, const String &lora_address){
    this->name = name;
    this->lora_address = lora_address;
}

Contact::Contact(){

}

String Contact::getName()const{
    return this->name;
}

String Contact::getLoraAddress()const{
    return this->lora_address;
}

bool Contact::operator==(const Contact &other)const{
    return (name == other.name && lora_address == other.lora_address);
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