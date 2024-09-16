#include "con_nets.hpp"

Wifi_connected_nets::Wifi_connected_nets(){

}

Wifi_connected_nets::~Wifi_connected_nets(){
    
}
/// @brief Find a network and return a wifi_info * structure pointer.
/// @param ssid 
/// @return wifi_info *
wifi_info * Wifi_connected_nets::find(char * ssid){
    if(list.size() > 0){
        for(uint32_t i = 0; i < list.size(); i++)
            if(strcmp(list[i].SSID, ssid) == 0)
                return &list[i];
    }
    return NULL;
}
/// @brief Add a wifi_info structure to the list.
/// @param wi 
/// @return bool
bool Wifi_connected_nets::add(wifi_info wi){
    wifi_info * w = find(wi.SSID);
    if(w == NULL){
        list.push_back(wi);
        return true;
    }
    return false;
}
/// @brief Save the wifi history to a file in SPIFFS.
/// @return 
bool Wifi_connected_nets::save(){
    if(!SPIFFS.begin(true)){
        Serial.println("failed mounting SPIFFS");
        return false;
    }

    fs::File file = SPIFFS.open("/wifi_list", FILE_WRITE);
    if(!file){
        Serial.println("couldn't open settings file");
        return false;
    }

    if(list.size() > 0){
        Serial.println("Saving networks...");
        for(uint32_t i = 0; i < list.size(); i++){
            file.println(list[i].SSID);
            file.println(list[i].auth_type);
            file.println(list[i].login);
            file.println(list[i].pass);
        }
        file.close();
        return true;
    }
    return false;
}
/// @brief Load the wifi history from SPIFFS.
/// @return bool
bool Wifi_connected_nets::load(){
    wifi_info wi;

    if(!SPIFFS.begin(true)){
        Serial.println("failed mounting SPIFFS");
        return false;
    }

    fs::File file = SPIFFS.open("/wifi_list", FILE_READ);
    if(!file){
        Serial.println("couldn't open settings file");
        return false;
    }

    std::vector<String> v;
    while(file.available()){
        v.push_back(file.readStringUntil('\n'));
    }

    file.close();

    for(uint32_t index = 0; index < v.size(); index ++){
        v[index].remove(v[index].length() - 1);
    }

    for(uint32_t i = 0; i < v.size(); i += 4){
        strcpy(wi.SSID, v[i].c_str());
        wi.auth_type = (wifi_auth_mode_t)v[i+1].toInt();
        strcpy(wi.login, v[i+2].c_str());
        strcpy(wi.pass, v[i+3].c_str());

        add(wi);
    }
    if(list.size() > 0)
        return true;
    else
        return false;
}
/// @brief Delete a network from the history.
/// @param ssid 
/// @return bool
bool Wifi_connected_nets::del(char * ssid){
    uint index = -1;

    for(uint i = 0; i < list.size(); i++)
        if(strcmp(list[i].SSID, ssid) == 0){
            index = i;
            break;
        }
    if(index != -1){
        list.erase(list.begin() + index);
        return true;
    }
    return false;
}