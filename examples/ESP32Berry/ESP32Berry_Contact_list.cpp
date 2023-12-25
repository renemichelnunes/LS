#include "ESP32Berry_Contact_list.hpp"
#include <exception>
#include <string>
#include <RadioLib.h>
#include <random>
#include <Preferences.h>
#include <iomanip>
#include <vector>
#include "lora_messages.hpp"
#include "SPIFFS.h"

static AppContactList *instance = NULL;

Preferences prefs;

static void saveContacts(){
  if(!SPIFFS.begin(true)){
    Serial.println("failed mounting SPIFFS");
    return;
  }

  File file = SPIFFS.open("/contacts", FILE_WRITE);
  if(!file){
    Serial.println("contacts file problem");
    return;
  }

  Contact c;

  for(uint32_t index = 0; index < instance->contact_list.size(); index++){
    c = instance->contact_list.getContact(index);
    file.println(c.getName());
    file.println(c.getID());
  }
  Serial.println("Contacts saved");
  file.close();
}

static void loadContacts(){
  if(!SPIFFS.begin(true)){
    Serial.println("failed mounting SPIFFS");
    return;
  }

  File file = SPIFFS.open("/contacts", FILE_READ);
  if(!file){
    Serial.println("contacts file problem");
    return;
  }
  
  vector<String> v;
  while(file.available()){
    v.push_back(file.readStringUntil('\n'));
  }

  file.close();

  for(uint32_t index = 0; index < v.size(); index ++){
    v[index].remove(v[index].length() - 1);
  }

  Contact c;
  for(uint32_t index = 0; index < v.size(); index += 2){
    c.setName(v[index]);
    c.setID(v[index + 1]);
    instance->contact_list.add(c);
  }
}

std::string generate_ID(){
  srand(time(NULL));
  static const char alphanum[] = "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  std::string ss;

  for (int i = 0; i < 6; ++i) {
    ss[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }
  return ss;
}

lv_obj_t * AppContactList::getList(){
  
  return list;
}

static void add_btn_event_cb(lv_event_t * e);

static void close_chat_window(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_CLICKED){
    lv_obj_t * window = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_del(window);
  }
}

struct msgAndList{
  lv_obj_t * list, *txtReply;
  Contact * c;
};
msgAndList * msg = new msgAndList;

static void sendMessage(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  int16_t err_code;
  lv_obj_t * btnList2;
  lv_obj_t* label2;

  if(code == LV_EVENT_SHORT_CLICKED){
    msgAndList * msg = (msgAndList *)lv_event_get_user_data(e);
    lv_obj_t * txtReply = msg->txtReply;
    String message = lv_textarea_get_text(txtReply);
    Serial.print(F("Message: "));
    Serial.println(message);
    if(!message.isEmpty()){  
      lora_packet packet, dummy, dummy2;
      strcpy(packet.id, msg->c->getID().c_str());
      strcpy(packet.msg, message.c_str());
      strcpy(packet.status, "sent");
      instance->_display->lv_port_sem_take();
      instance->_display->radio->getRadio()->standby();
      instance->_display->radio->getRadio()->readData((uint8_t*)&dummy, sizeof(dummy));
      err_code = instance->_display->radio->getRadio()->startTransmit((uint8_t*)&packet, sizeof(packet));

      //instance->_display->radio->getRadio()->startTransmit((uint8_t*)&dummy, sizeof(dummy));
      instance->_display->radio->getRadio()->startReceive();
      instance->_display->lv_port_sem_give();
      if(err_code != RADIOLIB_ERR_NONE){
        lv_list_add_text(msg->list, "fail to send");  
      }else{
        lv_list_add_text(msg->list, "Me");
        lv_obj_t * btnList = lv_list_add_btn(msg->list, NULL, message.c_str());
        lv_obj_t* label = lv_obj_get_child(btnList, 0);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_scroll_to_view(btnList, LV_ANIM_OFF);
        lv_textarea_set_text(txtReply, "");
        // Add answer to the contact messages
        packet.me = true;
        instance->_display->lim.addMessage(packet);
        Serial.println("respose added to the list of messages");
      }
    }
  }
}

static void load_messages(char * id){
  vector<lora_packet> caller_msg;
  caller_msg = instance->_display->lim.getMessages(id);
  if(caller_msg.size() > 0){
    Serial.print(caller_msg.size());
    Serial.println(" messages");
    Serial.println(caller_msg[0].id);
    for(int i = 0; i < caller_msg.size(); i++){
      if(caller_msg[i].me)
        Serial.print("me: ");
      Serial.println(caller_msg[i].msg);
    }
  }
}

static void chat_window(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_SHORT_CLICKED){
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_user_data(e);
    String name = lv_list_get_btn_text(instance->getList(), btn);
    Contact * ch = instance->contact_list.getContactByName(name);
    Serial.print(F("Chat with "));
    Serial.println(ch->getName());
    
    lv_obj_t * window = lv_obj_create(lv_scr_act());
    lv_obj_set_size(window, 320, 240);
    lv_obj_clear_flag(window, LV_OBJ_FLAG_SCROLLABLE);
    /*Contact description at the top*/
    lv_obj_t * btnContact = lv_btn_create(window);
    lv_obj_set_size(btnContact, 240, 25);
    lv_obj_align(btnContact, LV_ALIGN_TOP_LEFT, -10, -10);

    lv_obj_t * lblContact = lv_label_create(btnContact);
    char text[30];
    sprintf(text, "Chat with %s", name);
    lv_label_set_text(lblContact, text);
    lv_obj_align(lblContact, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_long_mode(lblContact, LV_LABEL_LONG_SCROLL_CIRCULAR);

    /*Back button on the right side*/
    lv_obj_t * btnClose = lv_btn_create(window);
    lv_obj_set_size(btnClose, 40, 25);
    lv_obj_align(btnClose, LV_ALIGN_TOP_RIGHT, 10, -10);

    lv_obj_t * lblClose = lv_label_create(btnClose);
    lv_label_set_text(lblClose, "back");
    lv_obj_align(lblClose, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btnClose, close_chat_window, LV_EVENT_CLICKED, window);

    /*List of replies*/
    lv_obj_t * listReply = lv_list_create(window);
    lv_obj_set_size(listReply, 320, 140);
    lv_obj_align(listReply, LV_ALIGN_TOP_MID, 0, 20);

    /*Reply text area*/
    lv_obj_t * txtReply = lv_textarea_create(window);
    lv_obj_set_size(txtReply, 270, 60);
    lv_obj_align(txtReply, LV_ALIGN_TOP_LEFT, -15, 165);
    lv_obj_add_state(txtReply, LV_STATE_FOCUSED);
    lv_textarea_set_max_length(txtReply, 200);//Per LoRa packet

    /*Send button*/
    lv_obj_t * btnSend = lv_btn_create(window);
    lv_obj_set_size(btnSend, 40, 55);
    lv_obj_align(btnSend, LV_ALIGN_TOP_RIGHT, 10, 168);
    lv_obj_clear_flag(btnSend, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    lv_obj_t * lblBtnSend = lv_label_create(btnSend);
    lv_label_set_text(lblBtnSend, "Send");
    lv_obj_align(lblBtnSend, LV_ALIGN_CENTER, 0, 0);

    msg->list = listReply;
    msg->txtReply = txtReply;
    msg->c = ch;
    lv_obj_add_event_cb(btnSend, sendMessage, LV_EVENT_SHORT_CLICKED, msg);

    // Retrieve messages
    load_messages((char *)ch->getID().c_str());
  }
}

//Used in close_edit_window()
struct data{
  lv_obj_t * obj;
  Contact * c;
};
data * data_ = new data;

static void close_edit_window(lv_event_t* e){
  lv_event_code_t code = lv_event_get_code(e);
  data * data_ = (data*)lv_event_get_user_data(e);
  lv_obj_t * window = data_->obj;//The edit window
  Contact * cc = data_->c;//The contact data
  cc = instance->contact_list.getContactByName(cc->getName());
  lv_obj_t * txtName = lv_obj_get_child(window, 0);
  lv_obj_t * txtID = lv_obj_get_child(window, 1);
  String name, id;

  if(code == LV_EVENT_CLICKED){

    name = lv_textarea_get_text(txtName);
    id = lv_textarea_get_text(txtID);
    cc->setName(name);
    cc->setID(id);

    lv_obj_del(window);
    instance->refresh_contact_list();
  }
}

static void delContact(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_CLICKED){
    data * de = (data *)lv_event_get_user_data(e);
    Contact * cd = de->c;
    cd = instance->contact_list.getContactByName(cd->getName());
    lv_obj_t * window = de->obj;

    instance->contact_list.del(*cd);
    lv_obj_del(window);
    instance->refresh_contact_list();
  }
}

static void edit_contact(lv_event_t* e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code = LV_EVENT_LONG_PRESSED){
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_user_data(e);
    String name = lv_list_get_btn_text(instance->getList(), btn);
    Contact* ce = instance->contact_list.getContactByName(name);

    lv_obj_t* window = lv_obj_create(lv_scr_act());
    lv_obj_set_size(window, 320, 240);
    lv_obj_align(window, LV_ALIGN_CENTER, 0 , 0);
    
    lv_obj_t* txtName = lv_textarea_create(window);
    lv_obj_set_size(txtName, 250, 40);
    lv_textarea_set_placeholder_text(txtName, "Name");
    lv_textarea_set_text(txtName, ce->getName().c_str());
    lv_obj_align(txtName, LV_ALIGN_TOP_MID, 0, -10);

    lv_obj_t* txtLoraAddr = lv_textarea_create(window);
    lv_obj_set_size(txtLoraAddr, 250, 40);
    lv_textarea_set_placeholder_text(txtLoraAddr, "ID");
    lv_textarea_set_text(txtLoraAddr, ce->getID().c_str());
    lv_obj_align(txtLoraAddr, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t* btnAdd = lv_btn_create(window);
    lv_obj_set_size(btnAdd, 40, 25);
    lv_obj_align(btnAdd, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_t* lblBtnAdd = lv_label_create(btnAdd);
    lv_label_set_text(lblBtnAdd, "OK");
    lv_obj_align(lblBtnAdd, LV_ALIGN_CENTER, 0, 0);

    /*Let's pass the contact and the window together
    So we can edit directly the contact data and
    and close the edit window without additional steps*/
    data_->c = ce;
    data_->obj = window;

    lv_obj_add_event_cb(btnAdd, close_edit_window, LV_EVENT_CLICKED, data_);

    lv_obj_t * btnDel = lv_btn_create(window);
    lv_obj_set_size(btnDel, 40, 25);
    lv_obj_align(btnDel, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    lv_obj_t * lblBtnDel = lv_label_create(btnDel);
    lv_label_set_text(lblBtnDel, "Del");
    lv_obj_align(lblBtnDel, LV_ALIGN_CENTER, 0, 0);

    lv_obj_add_event_cb(btnDel, delContact, LV_EVENT_CLICKED, data_);
  }
}


Contact c;
void AppContactList::refresh_contact_list(){
  try{
    lv_obj_clean(list);

    // The floating button
    lv_obj_t* addBtn = lv_btn_create(list);
    lv_obj_set_size(addBtn, 50, 50);
    lv_obj_add_flag(addBtn, LV_OBJ_FLAG_FLOATING);
    lv_obj_align(addBtn, LV_ALIGN_BOTTOM_RIGHT, 0, -lv_obj_get_style_pad_right(list, LV_PART_MAIN));
    lv_obj_add_event_cb(addBtn, add_btn_event_cb, LV_EVENT_ALL, list);
    lv_obj_set_style_radius(addBtn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_img_src(addBtn, LV_SYMBOL_PLUS, 0);
    lv_obj_set_style_text_font(addBtn, lv_theme_get_font_large(addBtn), 0);

    /*Adding events to each contact*/
    for(uint32_t i = 0; i < contact_list.size(); i++){
      c = contact_list.getContact(i);
      lv_obj_t* btn = lv_list_add_btn(this->list, LV_SYMBOL_CALL, c.getName().c_str());
      /*Edit contact info*/
      lv_obj_add_event_cb(btn, edit_contact, LV_EVENT_LONG_PRESSED, btn);
      /*Open chat window*/
      lv_obj_add_event_cb(btn, chat_window, LV_EVENT_SHORT_CLICKED, btn);
      lv_obj_move_foreground(addBtn);
      lv_obj_scroll_to_view(btn, LV_ANIM_ON);
    }
    saveContacts();
  }catch(exception e){
    Serial.println(e.what());
  }
}



AppContactList::AppContactList(Display* display, System* system, Network* network, 
const char* title) : AppBase(display, system, network, title){
  instance = this;
  display_width = display->get_display_width();
  contact_list = Contact_list();
  this->draw_ui();
  if(instance->_display->isLoRaOn())
    Serial.println(F("LoRa radio ready"));
  else
    Serial.println(F("LoRa radio not ready"));
}

static void add(lv_event_t* e){
  lv_obj_t* addBtn = lv_event_get_target(e);
  lv_obj_t* window = (lv_obj_t*)lv_event_get_user_data(e);
  String name, lora_addr;

  lv_obj_t* txtName = lv_obj_get_child(window, 0);
  Serial.print(F("Name: "));
  name = lv_textarea_get_text(txtName);
  Serial.println(name);
  lv_obj_t* txtLoRaAddr = lv_obj_get_child(window, 1);
  Serial.print(F("ID: "));
  lora_addr = lv_textarea_get_text(txtLoRaAddr);
  Serial.println(lora_addr);
  if(name != "" && lora_addr != ""){
    Contact c = Contact(name, lora_addr);
    if(!instance->contact_list.find(c)){
      if(instance->contact_list.add(c))
        Serial.println(F("Contact added"));
      else
        Serial.println(F("Fail to add contact"));
    }else
      Serial.println(F("Contact already exists"));
  }
  lv_obj_del(window);
  instance->refresh_contact_list();
}

static void add_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * float_btn = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
      lv_obj_t* window = lv_obj_create(lv_scr_act());
      lv_obj_set_size(window, 320, 240);
      lv_obj_align(window, LV_ALIGN_CENTER, 0 , 0);
      
      lv_obj_t* txtName = lv_textarea_create(window);
      lv_obj_set_size(txtName, 250, 40);
      lv_textarea_set_placeholder_text(txtName, "Name");
      lv_obj_align(txtName, LV_ALIGN_TOP_MID, 0, -10);
      lv_obj_add_flag(txtName, LV_STATE_FOCUSED);

      lv_obj_t* txtLoraAddr = lv_textarea_create(window);
      lv_obj_set_size(txtLoraAddr, 250, 40);
      lv_textarea_set_placeholder_text(txtLoraAddr, "ID");
      lv_obj_align(txtLoraAddr, LV_ALIGN_TOP_MID, 0, 30);

      lv_obj_t* btnAdd = lv_btn_create(window);
      lv_obj_set_size(btnAdd, 40, 25);
      lv_obj_align(btnAdd, LV_ALIGN_BOTTOM_LEFT, 0, 0);

      lv_obj_t* lblBtnAdd = lv_label_create(btnAdd);
      lv_label_set_text(lblBtnAdd, "OK");
      lv_obj_align(lblBtnAdd, LV_ALIGN_CENTER, 0, 0);
      
      lv_obj_t * list = (lv_obj_t*)lv_event_get_user_data(e);
      lv_obj_add_event_cb(btnAdd, add, LV_EVENT_CLICKED, window);

      lv_obj_t* last_btn = lv_obj_get_child(list, -1);
      lv_obj_move_foreground(float_btn);
      lv_obj_scroll_to_view(last_btn, LV_ANIM_ON);
    }
}

struct config_radio_objs{
  lv_obj_t * window;
  lv_obj_t * name;
  lv_obj_t * id;
  lv_obj_t * addr;
  lv_obj_t * sync_word;
  lv_obj_t * preamble;
  lv_obj_t * chkCRC;
  lv_obj_t * current_limit;
  lv_obj_t * bandwidth;
  lv_obj_t * spread_factor;
  lv_obj_t * coding_rate;
  lv_obj_t * tx_power;
  lv_obj_t * lora_freq;
}config_objs;

static void print_lora_settings(){
  if(prefs.begin("lora_settings", true)){
    String name, id;
    uint16_t addr, current_limit, spread_factor, coding_rate, bandwidth, tx_power, freq;
    uint16_t preamble;
    uint32_t sync_word;
    bool crc;

    name = prefs.getString("name");
    id = prefs.getString("id");
    addr = prefs.getUChar("address");
    current_limit = prefs.getUShort("current_limit");
    bandwidth = prefs.getUShort("bandwidth");
    spread_factor = prefs.getUShort("spread_factor");
    coding_rate = prefs.getUShort("coding_rate");
    sync_word = prefs.getULong("sync_word");
    tx_power = prefs.getUShort("tx_power");
    preamble = prefs.getUShort("preamble");
    freq = prefs.getUShort("lora_freq");
    crc = prefs.getBool("crc");
    char v[32];

    Serial.println(F("=========================LoRa Settings==================================="));
    Serial.print(F("Name: "));
    Serial.println(name);
    Serial.print(F("ID: "));
    Serial.println(id);
    Serial.print(F("Address: "));
    sprintf(v, "%x", addr);
    Serial.println(v);
    Serial.print(F("Current limit: "));
    Serial.println(instance->_display->radio->vcurrent_limit[current_limit]);
    Serial.print(F("Bandwidth: "));
    Serial.println(instance->_display->radio->vbandwidth[bandwidth]);
    Serial.print(F("Spread factor: "));
    Serial.println(instance->_display->radio->vspread_factor[spread_factor]);
    Serial.print(F("Coding rate: "));
    Serial.println(instance->_display->radio->vcoding_rate[coding_rate]);
    Serial.print(F("Sync word: "));
    sprintf(v, "%x", sync_word);
    Serial.println(v);
    Serial.print(F("TX power: "));
    Serial.println(instance->_display->radio->vtx_power[tx_power]);
    Serial.print(F("Preamble symbols: "));
    Serial.println(preamble);
    Serial.print(F("LoRa chip carrier: "));
    Serial.println(instance->_display->radio->vlora_freq[freq]);
    Serial.print(F("CRC check "));
    Serial.println(crc ? "true" : "false");
    Serial.println(F("========================================================================="));
    prefs.end();
  }
}

static void close_config(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_SHORT_CLICKED){
    /*Get objects data*/
    config_radio_objs * config = (config_radio_objs *)lv_event_get_user_data(e);
    if(prefs.begin("lora_settings", false)){
      String name, id, addr, sync_word, preamble;
      bool CRC;
      char * end;
      uint32_t n;
      char v[32];
      /*Text areas*/
      name = lv_textarea_get_text(config->name);
      id = lv_textarea_get_text(config->id);
      addr = lv_textarea_get_text(config->addr);
      sync_word = lv_textarea_get_text(config->sync_word);
      preamble = lv_textarea_get_text(config->preamble);
      CRC = lv_obj_get_state(config->chkCRC) & LV_STATE_CHECKED;

      /*Dropdown lists*/
      instance->_display->radio->settings.setCurrentLimit(lv_dropdown_get_selected(config->current_limit));
      instance->_display->radio->settings.setBandwidth(lv_dropdown_get_selected(config->bandwidth));
      instance->_display->radio->settings.setSpreadFactor(lv_dropdown_get_selected(config->spread_factor));
      instance->_display->radio->settings.setCodeRate(lv_dropdown_get_selected(config->coding_rate));
      instance->_display->radio->settings.setTXPower(lv_dropdown_get_selected(config->tx_power));
      instance->_display->radio->settings.setFreq(lv_dropdown_get_selected(config->lora_freq));

      instance->_display->radio->settings.setName(name);
      instance->_display->radio->settings.setId(id);
      n = strtoll(addr.c_str(), &end, 16);
      instance->_display->radio->settings.setAddr(n);
      n = strtoll(sync_word.c_str(), &end, 16);
      instance->_display->radio->settings.setSyncWord(n);
      n = strtoll(preamble.c_str(), &end, 10);
      instance->_display->radio->settings.setPreamble(n);
      instance->_display->radio->settings.setCRC(CRC);
      /*
      Serial.println(F("=========================LoRa Settings==================================="));
      Serial.print(F("Name: "));
      Serial.println(instance->_display->radio.settings.getName());
      Serial.print(F("ID: "));
      Serial.println(instance->_display->radio.settings.getID());
      Serial.print(F("Address: "));
      Serial.println(instance->_display->radio.settings.getAddr());
      Serial.print(F("Current limit: "));
      Serial.println(instance->_display->radio.settings.getCurrentLimit());
      Serial.print(F("Bandwidth: "));
      Serial.println(instance->_display->radio.settings.getBandwidth());
      Serial.print(F("Spread factor: "));
      Serial.println(instance->_display->radio.settings.getSpreadFactor());
      Serial.print(F("Coding rate: "));
      Serial.println(instance->_display->radio.settings.getCodingRate());
      Serial.print(F("Sync word: "));
      Serial.println(instance->_display->radio.settings.getSyncWord());
      Serial.print(F("TX power: "));
      Serial.println(instance->_display->radio.settings.getTXPower());
      Serial.print(F("Preamble symbols: "));
      Serial.println(instance->_display->radio.settings.getPreamble());
      Serial.print(F("LoRa chip carrier: "));
      Serial.println(instance->_display->radio.settings.getFreq());
      Serial.print(F("CRC check "));
      Serial.println(instance->_display->radio.settings.getCRC() ? "true" : "false");
      Serial.println(F("========================================================================="));
      */
      try{
        if(prefs.clear())
          Serial.println(F("Settings cleared"));
        else
          Serial.println(F("Settings could not be cleared"));
        prefs.putString("name", instance->_display->radio->settings.getName());
        prefs.putString("id", instance->_display->radio->settings.getID());
        prefs.putUChar("address", instance->_display->radio->settings.getAddr());
        prefs.putUShort("current_limit", instance->_display->radio->settings.getCurrentLimit());
        prefs.putUShort("bandwidth", instance->_display->radio->settings.getBandwidth());
        prefs.putUShort("spread_factor", instance->_display->radio->settings.getSpreadFactor());
        prefs.putUShort("coding_rate", instance->_display->radio->settings.getCodingRate());
        prefs.putULong("sync_word", instance->_display->radio->settings.getSyncWord());
        prefs.putUShort("tx_power", instance->_display->radio->settings.getTXPower());
        prefs.putUShort("preamble", instance->_display->radio->settings.getPreamble());
        prefs.putUShort("lora_freq", instance->_display->radio->settings.getFreq());
        prefs.putBool("crc", instance->_display->radio->settings.getCRC());
        Serial.println(F("Settings saved"));
      }catch(exception ex){
        Serial.print(F("failed to save settings - "));
        Serial.println(ex.what());
      }
      
      prefs.end();

      print_lora_settings();

      /*Apply the configuration on the lora chip*/
      //instance->_display->lora_apply_config();
    }
    if(config->window != NULL)
      lv_obj_del(config->window);
  }
}

static void genID(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_SHORT_CLICKED){
    lv_obj_t * txtID = (lv_obj_t*)lv_event_get_user_data(e);
    std::string id = generate_ID();
    lv_textarea_set_text(txtID, id.c_str());
  }
}

static void ddGetCurrentLimit(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_VALUE_CHANGED){
    lv_obj_t * dd = (lv_obj_t *)lv_event_get_user_data(e);
    uint16_t index = lv_dropdown_get_selected(dd);
    Serial.print(F("Current limit: "));
    Serial.println(instance->_display->radio->vcurrent_limit[index]);
    instance->_display->radio->settings.setCurrentLimit(index);
  }
}

static void ddGetBW(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_VALUE_CHANGED){
    lv_obj_t * dd = (lv_obj_t *)lv_event_get_user_data(e);
    uint16_t index = lv_dropdown_get_selected(dd);
    Serial.print(F("bandwidth: "));
    Serial.println(instance->_display->radio->vbandwidth[index]);
    instance->_display->radio->settings.setBandwidth(index);
  }
}

static void ddGetSF(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_VALUE_CHANGED){
    lv_obj_t * dd = (lv_obj_t *)lv_event_get_user_data(e);
    uint16_t index = lv_dropdown_get_selected(dd);
    Serial.print(F("Spread factor: "));
    Serial.println(instance->_display->radio->vspread_factor[index]);
    instance->_display->radio->settings.setSpreadFactor(index);
  }
}

static void ddGetCR(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_VALUE_CHANGED){
    lv_obj_t * dd = (lv_obj_t *)lv_event_get_user_data(e);
    uint16_t index = lv_dropdown_get_selected(dd);
    Serial.print(F("Coding rate: "));
    Serial.println(instance->_display->radio->vcoding_rate[index]);
    instance->_display->radio->settings.setCodeRate(index);
  }
}

static void ddGetPower(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_VALUE_CHANGED){
    lv_obj_t * dd = (lv_obj_t *)lv_event_get_user_data(e);
    uint16_t index = lv_dropdown_get_selected(dd);
    Serial.print(F("TX power: "));
    Serial.println(instance->_display->radio->vtx_power[index]);
    instance->_display->radio->settings.setTXPower(index);
  }
}

static void ddGetLoraFreq(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_VALUE_CHANGED){
    lv_obj_t * dd = (lv_obj_t *)lv_event_get_user_data(e);
    uint16_t index = lv_dropdown_get_selected(dd);
    Serial.print(F("LoRa carrier: "));
    Serial.println(instance->_display->radio->vlora_freq[index]);
    instance->_display->radio->settings.setFreq(index);
  }
}

static void loadPrefs(){
  if(prefs.begin("lora_settings", true)){
    String name, id;
    uint16_t addr, current_limit, spread_factor, coding_rate, bandwidth, tx_power, freq;
    uint16_t preamble;
    uint32_t sync_word;
    bool crc;

    name = prefs.getString("name");
    id = prefs.getString("id");
    addr = prefs.getUChar("address");
    current_limit = prefs.getUShort("current_limit");
    bandwidth = prefs.getUShort("bandwidth");
    spread_factor = prefs.getUShort("spread_factor");
    coding_rate = prefs.getUShort("coding_rate");
    sync_word = prefs.getULong("sync_word");
    tx_power = prefs.getUShort("tx_power");
    preamble = prefs.getUShort("preamble");
    freq = prefs.getUShort("lora_freq");
    crc = prefs.getBool("crc");
    prefs.end();

    instance->_display->radio->settings.setName(name);
    instance->_display->radio->settings.setId(id);
    instance->_display->radio->settings.setAddr(addr);
    instance->_display->radio->settings.setCurrentLimit(current_limit);
    instance->_display->radio->settings.setBandwidth(bandwidth);
    instance->_display->radio->settings.setSpreadFactor(spread_factor);
    instance->_display->radio->settings.setCodeRate(coding_rate);
    instance->_display->radio->settings.setSyncWord(sync_word);
    instance->_display->radio->settings.setTXPower(tx_power);
    instance->_display->radio->settings.setPreamble(preamble);
    instance->_display->radio->settings.setFreq(freq);
    instance->_display->radio->settings.setCRC(crc); 

    Serial.println(F("Lora settings loaded"));
  }else 
    Serial.println(F("first time settings"));
}

static void config_radio(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_SHORT_CLICKED){
    loadPrefs();
    char s[32];
    lv_obj_t * window = lv_obj_create(lv_scr_act());
    lv_obj_set_size(window, 320, 240);
    lv_obj_clear_flag(window, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * btnTitle = lv_btn_create(window);
    lv_obj_set_size(btnTitle, 120, 25);
    lv_obj_align(btnTitle, LV_ALIGN_TOP_LEFT, -15, -15);

    lv_obj_t * lblBtnTitle = lv_label_create(btnTitle);
    lv_label_set_text(lblBtnTitle, "LoRa settings");
    lv_obj_align(lblBtnTitle, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * btnClose = lv_btn_create(window);
    lv_obj_set_size(btnClose, 25, 25);
    lv_obj_align(btnClose, LV_ALIGN_TOP_RIGHT, +15, -15);
    //lv_obj_set_style_radius(btnClose, 0, 0);
    lv_obj_set_style_bg_img_src(btnClose, LV_SYMBOL_CLOSE, 0);
    lv_obj_add_event_cb(btnClose, close_config, LV_EVENT_SHORT_CLICKED, &config_objs);

    /*Lora configs*/
    lv_obj_t * winConfig = lv_obj_create(window);
    lv_obj_set_size(winConfig, 320, 220);
    lv_obj_align(winConfig, LV_ALIGN_TOP_MID, 0, 10);
    //lv_obj_set_style_bg_color(winConfig, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    /*Lora name*/
    lv_obj_t * txtName = lv_textarea_create(winConfig);
    lv_obj_set_size(txtName, 200, 30);
    lv_obj_align(txtName, LV_ALIGN_TOP_LEFT, 0, -10);
    lv_textarea_set_placeholder_text(txtName, "Name");
    lv_textarea_set_text(txtName, instance->_display->radio->settings.getName().c_str());
    
    /*Lora unique ID*/
    lv_obj_t * txtID = lv_textarea_create(winConfig);
    lv_obj_set_size(txtID, 100, 30);
    lv_obj_align(txtID, LV_ALIGN_TOP_LEFT, 18, 20);
    lv_textarea_set_placeholder_text(txtID, "Unique ID");
    lv_textarea_set_max_length(txtID, 6);
    lv_textarea_set_text(txtID, instance->_display->radio->settings.getID().c_str());

    lv_obj_t * lblID = lv_label_create(winConfig);
    lv_label_set_text(lblID, "ID");
    lv_obj_align(lblID, LV_ALIGN_TOP_LEFT, 0, 30);

    /*Generate ID button*/
    lv_obj_t * btnGenerate = lv_btn_create(winConfig);
    lv_obj_set_size(btnGenerate, 80, 20);
    lv_obj_align(btnGenerate, LV_ALIGN_TOP_LEFT, 120, 25);
    lv_obj_add_event_cb(btnGenerate, genID, LV_EVENT_SHORT_CLICKED, txtID);

    lv_obj_t * lblBtnGenerate = lv_label_create(btnGenerate);
    lv_label_set_text(lblBtnGenerate, "Generate");
    lv_obj_align(lblBtnGenerate, LV_ALIGN_CENTER, 0, 0);

    /*Node address*/
    lv_obj_t * txtAddr = lv_textarea_create(winConfig);
    lv_obj_set_size(txtAddr, 100, 30);
    lv_obj_align(txtAddr, LV_ALIGN_TOP_LEFT, 100, 50);
    lv_textarea_set_placeholder_text(txtAddr, "Address");
    lv_textarea_set_max_length(txtAddr, 3);
    if(instance->_display->radio->settings.getAddr() != 0)
      sprintf(s, "%x", instance->_display->radio->settings.getAddr());
    else
      strcpy(s, "");
    lv_textarea_set_text(txtAddr, s);

    lv_obj_t * lblAddr = lv_label_create(winConfig);
    lv_label_set_text(lblAddr, "Address");
    lv_obj_align(lblAddr, LV_ALIGN_TOP_LEFT, 0, 60);

    /*Current limiter*/
    lv_obj_t * lblCurrent= lv_label_create(winConfig);
    lv_label_set_text(lblCurrent, "Current limit");
    lv_obj_align(lblCurrent, LV_ALIGN_TOP_LEFT, 0, 90);

    lv_obj_t * ddCurrent = lv_dropdown_create(winConfig);
    lv_dropdown_set_options(ddCurrent, "45\n"
                                        "60\n"
                                        "80\n"
                                        "100\n"
                                        "120\n"
                                        "140\n"
                                        "160\n"
                                        "180\n"
                                        "200\n"
                                        "220\n"
                                        "240\n"
                                        "no limit");
    lv_dropdown_set_selected(ddCurrent, instance->_display->radio->settings.getCurrentLimit());
    lv_obj_set_size(ddCurrent, 100, 30);
    lv_obj_align(ddCurrent, LV_ALIGN_TOP_LEFT, 100, 80);
    lv_obj_add_event_cb(ddCurrent, ddGetCurrentLimit, LV_EVENT_VALUE_CHANGED, ddCurrent);
    

    /*Bandwidth*/
    lv_obj_t * lblBW= lv_label_create(winConfig);
    lv_label_set_text(lblBW, "Bandwidth");
    lv_obj_align(lblBW, LV_ALIGN_TOP_LEFT, 0, 120);

    lv_obj_t * ddBW = lv_dropdown_create(winConfig);
    lv_dropdown_set_options(ddBW, "7.8\n"
                                        "10.4\n"
                                        "15.6\n"
                                        "20.8\n"
                                        "31.25\n"
                                        "41.7\n"
                                        "62.5\n"
                                        "125.0\n"
                                        "250.0\n"
                                        "500.0");
    lv_dropdown_set_selected(ddBW, instance->_display->radio->settings.getBandwidth());
    lv_obj_set_size(ddBW, 100, 30);
    lv_obj_align(ddBW, LV_ALIGN_TOP_LEFT, 100, 110);
    lv_obj_add_event_cb(ddBW, ddGetBW, LV_EVENT_VALUE_CHANGED, ddBW);

    /*Spread factor*/
    lv_obj_t * lblSF = lv_label_create(winConfig);
    lv_label_set_text(lblSF, "Spread factor");
    lv_obj_align(lblSF, LV_ALIGN_TOP_LEFT, 0, 150);

    lv_obj_t * ddSF = lv_dropdown_create(winConfig);
    lv_dropdown_set_options(ddSF, "5\n"
                                        "6\n"
                                        "7\n"
                                        "8\n"
                                        "9\n"
                                        "10\n"
                                        "11\n"
                                        "12");
    lv_dropdown_set_selected(ddSF, instance->_display->radio->settings.getSpreadFactor());
    lv_obj_set_size(ddSF, 100, 30);
    lv_obj_align(ddSF, LV_ALIGN_TOP_LEFT, 100, 140);
    lv_obj_add_event_cb(ddSF, ddGetSF, LV_EVENT_VALUE_CHANGED, ddSF);

    /*Coding rate*/
    lv_obj_t * lblCR = lv_label_create(winConfig);
    lv_label_set_text(lblCR, "Coding rate");
    lv_obj_align(lblCR, LV_ALIGN_TOP_LEFT, 0, 180);

    lv_obj_t * ddCR = lv_dropdown_create(winConfig);
    lv_dropdown_set_options(ddCR, "5\n"
                                        "6\n"
                                        "7\n"
                                        "8");
    lv_dropdown_set_selected(ddCR, instance->_display->radio->settings.getCodingRate());
    lv_obj_set_size(ddCR, 100, 30);
    lv_obj_align(ddCR, LV_ALIGN_TOP_LEFT, 100, 170);
    lv_obj_add_event_cb(ddCR, ddGetCR, LV_EVENT_VALUE_CHANGED, ddCR);

    /*Sync word*/
    lv_obj_t * txtSyncW = lv_textarea_create(winConfig);
    lv_obj_set_size(txtSyncW, 100, 30);
    lv_obj_align(txtSyncW, LV_ALIGN_TOP_LEFT, 100, 200);
    lv_textarea_set_placeholder_text(txtSyncW, "hex 5B");
    lv_textarea_set_max_length(txtSyncW, 2);
    sprintf(s, "%x", instance->_display->radio->settings.getSyncWord());
    lv_textarea_set_text(txtSyncW, s);

    lv_obj_t * lblSyncW = lv_label_create(winConfig);
    lv_label_set_text(lblSyncW, "Sync word");
    lv_obj_align(lblSyncW, LV_ALIGN_TOP_LEFT, 0, 210);

    /*Power output in dbm*/
    lv_obj_t * lblPower = lv_label_create(winConfig);
    lv_label_set_text(lblPower, "TX power");
    lv_obj_align(lblPower, LV_ALIGN_TOP_LEFT, 0, 240);

    lv_obj_t * ddPower = lv_dropdown_create(winConfig);
    lv_dropdown_set_options(ddPower, "-17\n"
                                        "-10\n"
                                        "-5\n"
                                        "0\n"
                                        "5\n"
                                        "10\n"
                                        "15\n"
                                        "20\n"
                                        "22");
    lv_dropdown_set_selected(ddPower, instance->_display->radio->settings.getTXPower());
    lv_obj_set_size(ddPower, 100, 30);
    lv_obj_align(ddPower, LV_ALIGN_TOP_LEFT, 100, 230);
    lv_obj_add_event_cb(ddPower, ddGetPower, LV_EVENT_VALUE_CHANGED, ddPower);

    /*Preamble symbols*/
    lv_obj_t * txtPreamble = lv_textarea_create(winConfig);
    lv_obj_set_size(txtPreamble, 100, 30);
    lv_obj_align(txtPreamble, LV_ALIGN_TOP_LEFT, 100, 260);
    lv_textarea_set_placeholder_text(txtPreamble, "0-65535");
    lv_textarea_set_max_length(txtPreamble, 5);
    itoa(instance->_display->radio->settings.getPreamble(), s, 10);
    lv_textarea_set_text(txtPreamble, s);

    lv_obj_t * lblPreamble = lv_label_create(winConfig);
    lv_label_set_text(lblPreamble, "Preamble");
    lv_obj_align(lblPreamble, LV_ALIGN_TOP_LEFT, 0, 270);

    /*Lora chip frequency*/
    lv_obj_t * lblFreq = lv_label_create(winConfig);
    lv_label_set_text(lblFreq, "LoRa freq");
    lv_obj_align(lblFreq, LV_ALIGN_TOP_LEFT, 0, 300);

    lv_obj_t * ddFreq = lv_dropdown_create(winConfig);
    lv_dropdown_set_options(ddFreq, "433.0\n"
                                    "868.0\n"
                                    "915.0");
    lv_dropdown_set_selected(ddFreq, instance->_display->radio->settings.getFreq());
    lv_obj_set_size(ddFreq, 100, 30);
    lv_obj_align(ddFreq, LV_ALIGN_TOP_LEFT, 100, 290);
    lv_obj_add_event_cb(ddFreq, ddGetLoraFreq, LV_EVENT_VALUE_CHANGED, ddFreq);

    lv_obj_t * chkCRC = lv_checkbox_create(winConfig);
    lv_checkbox_set_text(chkCRC, "CRC check");
    lv_obj_align(chkCRC, LV_ALIGN_OUT_TOP_LEFT, 0, 320);
    if(instance->_display->radio->settings.getCRC())
      lv_obj_add_state(chkCRC, LV_STATE_CHECKED);

    /*Objects to obtain the text value in close_config()*/
    config_objs.window = window;
    config_objs.name = txtName;
    config_objs.id = txtID;
    config_objs.addr = txtAddr;
    config_objs.sync_word = txtSyncW;
    config_objs.preamble = txtPreamble;
    config_objs.chkCRC = chkCRC;
    config_objs.current_limit = ddCurrent;
    config_objs.bandwidth = ddBW;
    config_objs.spread_factor = ddSF;
    config_objs.coding_rate = ddCR;
    config_objs.tx_power = ddPower;
    config_objs.lora_freq = ddFreq;
  }
  
}

void AppContactList::draw_ui(){
  lv_style_init(&msgStyle);
  lv_style_set_bg_color(&msgStyle, lv_color_black());
  lv_style_set_pad_ver(&msgStyle, 8);
  lv_style_set_border_color(&msgStyle, lv_color_hex(0x989898));
  lv_style_set_border_width(&msgStyle, 2);
  lv_style_set_border_opa(&msgStyle, LV_OPA_50);
  lv_style_set_border_side(&msgStyle, LV_BORDER_SIDE_BOTTOM);
  // The contact list
  list = lv_list_create(ui_AppPanel);
  lv_obj_set_size(list, display_width, 210);
  lv_obj_align(list, LV_ALIGN_LEFT_MID, -15, 0);
  lv_obj_set_style_border_opa(list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_list_add_btn(list, LV_SYMBOL_WARNING, "Broadcast");
  // The floating button
  lv_obj_t* addBtn = lv_btn_create(list);
  lv_obj_set_size(addBtn, 30, 30);
  lv_obj_add_flag(addBtn, LV_OBJ_FLAG_FLOATING);
  lv_obj_align(addBtn, LV_ALIGN_BOTTOM_RIGHT, 0, -lv_obj_get_style_pad_right(list, LV_PART_MAIN));
  lv_obj_add_event_cb(addBtn, add_btn_event_cb, LV_EVENT_ALL, list);
  lv_obj_set_style_radius(addBtn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_img_src(addBtn, LV_SYMBOL_PLUS, 0);
  lv_obj_set_style_text_font(addBtn, lv_theme_get_font_large(addBtn), 0);

  /*Config button*/
  configbtn = lv_btn_create(_bodyScreen);
  lv_obj_set_size(configbtn, 25, 25);
  lv_obj_align(configbtn, LV_ALIGN_TOP_RIGHT, -50, -12);
  lv_obj_set_style_radius(configbtn, 0, 0);
  lv_obj_set_style_bg_img_src(configbtn, LV_SYMBOL_BARS, 0);
  lv_obj_add_event_cb(configbtn, config_radio, LV_EVENT_SHORT_CLICKED, NULL);

  try{
    loadContacts();
    this->refresh_contact_list();  
  }catch (exception &e){
    Serial.println(e.what());
  }

}

void AppContactList::tg_event_handler(lv_event_t* e){
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* obj = lv_event_get_target(e);

  if(code == LV_EVENT_CLICKED){
    if(obj == addBtn){
      this->add_contact();
    }
  }
}

void AppContactList::add_contact(){
  
}

void AppContactList::close_app(){
  _display->goback_main_screen();
  lv_obj_del(_bodyScreen);
  delete this;
};

AppContactList::~AppContactList(){
  
}
