#include "ESP32Berry_Contact_list.hpp"
#include <exception>
#include <string>
#include <RadioLib.h>
#include <random>
#include <Preferences.h>

Preferences prefs;

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

static AppContactList *instance = NULL;

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
  if(code == LV_EVENT_SHORT_CLICKED){
    msgAndList * msg = (msgAndList *)lv_event_get_user_data(e);
    lv_obj_t * txtReply = msg->txtReply;
    String message = lv_textarea_get_text(txtReply);
    Serial.print("Message: ");
    Serial.println(message);
    if(!message.isEmpty()){
      lv_list_add_text(msg->list, "Me");
      lv_obj_t * btnList = lv_list_add_btn(msg->list, NULL, message.c_str());
      lv_obj_t* label = lv_obj_get_child(btnList, 0);
      lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
      lv_obj_scroll_to_view(btnList, LV_ANIM_OFF);
      //delay(2000);
      lv_list_add_text(msg->list, msg->c->getName().c_str());
      lv_obj_t * btnList2 = lv_list_add_btn(msg->list, NULL, "You're welcome! If you have any more questions or need further assistance, please don't hesitate to ask. Happy coding!");
      lv_obj_t* label2 = lv_obj_get_child(btnList2, 0);
      lv_label_set_long_mode(label2, LV_LABEL_LONG_WRAP);
      lv_obj_scroll_to_view(btnList2, LV_ANIM_OFF);
    }
    lv_textarea_set_text(txtReply, "");
  }
}

static void chat_window(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_SHORT_CLICKED){
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_user_data(e);
    String name = lv_list_get_btn_text(instance->getList(), btn);
    Contact * ch = instance->contact_list.getContactByName(name);
    Serial.print("Chat with ");
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
    lv_textarea_set_max_length(txtReply, 256);//Per LoRa packet

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
  lv_obj_t * txtLoraAddr = lv_obj_get_child(window, 1);
  String name, laddr;

  if(code == LV_EVENT_CLICKED){

    name = lv_textarea_get_text(txtName);
    laddr = lv_textarea_get_text(txtLoraAddr);
    cc->setName(name);
    cc->setLAddr(laddr);

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
    lv_textarea_set_placeholder_text(txtLoraAddr, "LoRa Address");
    lv_textarea_set_text(txtLoraAddr, ce->getLoraAddress().c_str());
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
    Serial.println("LoRa radio ready");
  else
    Serial.println("LoRa radio not ready");
}

static void add(lv_event_t* e){
  lv_obj_t* addBtn = lv_event_get_target(e);
  lv_obj_t* window = (lv_obj_t*)lv_event_get_user_data(e);
  String name, lora_addr;

  lv_obj_t* txtName = lv_obj_get_child(window, 0);
  Serial.print("Name: ");
  name = lv_textarea_get_text(txtName);
  Serial.println(name);
  lv_obj_t* txtLoRaAddr = lv_obj_get_child(window, 1);
  Serial.print("LoRa Address: ");
  lora_addr = lv_textarea_get_text(txtLoRaAddr);
  Serial.println(lora_addr);
  if(name != "" && lora_addr != ""){
    Contact c = Contact(name, lora_addr);
    if(!instance->contact_list.find(c)){
      if(instance->contact_list.add(c))
        Serial.println("Contact added");
      else
        Serial.println("Fail to add contact");
    }else
      Serial.println("Contact already exists");
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
      lv_textarea_set_placeholder_text(txtLoraAddr, "LoRa Address");
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

static void close_config(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_SHORT_CLICKED){
    prefs.begin("lora_settings", false);
    /*Get objects data*/
    lv_obj_t * window = (lv_obj_t *)lv_event_get_user_data(e);
    prefs.end();
    if(window != NULL)
      lv_obj_del(window);
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
  if(code == LV_EVENT_CLICKED){
    lv_obj_t * dd = (lv_obj_t *)lv_event_get_user_data(e);
    char strvalue[10] = {'\0'};
    lv_dropdown_get_selected_str(dd, strvalue, sizeof(strvalue));
    Serial.print("selection: ");
    Serial.println(strvalue);
  }
}

static void config_radio(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_SHORT_CLICKED){
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
    lv_obj_add_event_cb(btnClose, close_config, LV_EVENT_SHORT_CLICKED, window);

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
    
    /*Lora unique ID*/
    lv_obj_t * txtID = lv_textarea_create(winConfig);
    lv_obj_set_size(txtID, 100, 30);
    lv_obj_align(txtID, LV_ALIGN_TOP_LEFT, 18, 20);
    lv_textarea_set_placeholder_text(txtID, "Unique ID");
    lv_textarea_set_max_length(txtID, 6);

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
    lv_obj_set_size(ddCurrent, 100, 30);
    lv_obj_align(ddCurrent, LV_ALIGN_TOP_LEFT, 100, 80);
    lv_obj_add_event_cb(ddCurrent, ddGetCurrentLimit, LV_EVENT_CLICKED, ddCurrent);

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
    lv_obj_set_size(ddBW, 100, 30);
    lv_obj_align(ddBW, LV_ALIGN_TOP_LEFT, 100, 110);

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
    lv_obj_set_size(ddSF, 100, 30);
    lv_obj_align(ddSF, LV_ALIGN_TOP_LEFT, 100, 140);

    /*Coding rate*/
    lv_obj_t * lblCR = lv_label_create(winConfig);
    lv_label_set_text(lblCR, "Coding rate");
    lv_obj_align(lblCR, LV_ALIGN_TOP_LEFT, 0, 180);

    lv_obj_t * ddCR = lv_dropdown_create(winConfig);
    lv_dropdown_set_options(ddCR, "5\n"
                                        "6\n"
                                        "7\n"
                                        "8");
    lv_obj_set_size(ddCR, 100, 30);
    lv_obj_align(ddCR, LV_ALIGN_TOP_LEFT, 100, 170);

    /*Sync word*/
    lv_obj_t * txtSyncW = lv_textarea_create(winConfig);
    lv_obj_set_size(txtSyncW, 100, 30);
    lv_obj_align(txtSyncW, LV_ALIGN_TOP_LEFT, 100, 200);
    lv_textarea_set_placeholder_text(txtSyncW, "hex 5B");
    lv_textarea_set_max_length(txtSyncW, 2);

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
    lv_obj_set_size(ddPower, 100, 30);
    lv_obj_align(ddPower, LV_ALIGN_TOP_LEFT, 100, 230);

    /*Preamble symbols*/
    lv_obj_t * txtPreamble = lv_textarea_create(winConfig);
    lv_obj_set_size(txtPreamble, 100, 30);
    lv_obj_align(txtPreamble, LV_ALIGN_TOP_LEFT, 100, 260);
    lv_textarea_set_placeholder_text(txtPreamble, "0-65535");
    lv_textarea_set_max_length(txtPreamble, 5);

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
    lv_obj_set_size(ddFreq, 100, 30);
    lv_obj_align(ddFreq, LV_ALIGN_TOP_LEFT, 100, 290);
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