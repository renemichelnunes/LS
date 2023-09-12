#include "ESP32Berry_Contact_list.hpp"
#include <exception>
#include <string>

static AppContactList *instance = NULL;

//static Contact_list contact_list = Contact_list();

lv_obj_t * AppContactList::getList(){
  return list;
}

static void add_btn_event_cb(lv_event_t * e);

static void close_chat_window(lv_event_t * e){

}

static void chat_window(lv_event_t * e){
  lv_event_code_t code = lv_event_get_code(e);
  if(code == LV_EVENT_CLICKED){
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_user_data(e);
    String name = lv_list_get_btn_text(instance->getList(), btn);
    Contact * ch = instance->contact_list.getContactByName(name);
    Serial.print("Chat with ");
    Serial.println();

    lv_obj_t * window = lv_obj_create(lv_scr_act());
    lv_obj_set_size(window, 300, 200);

    lv_obj_t * btnContact = lv_btn_create(window);
    lv_obj_set_size(btnContact, 300, 200);
    lv_obj_align(btnContact, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t * lblContact = lv_label_create(btnContact);
    char text[30];
    sprintf(text, "Chat with %s", name);
    lv_label_set_text(lblContact, text);
    lv_obj_align(lblContact, LV_ALIGN_CENTER, 0, 0);
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
    lv_obj_set_size(window, 300, 200);
    lv_obj_align(window, LV_ALIGN_CENTER, 0 , 10);
    
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
      lv_obj_add_event_cb(btn, chat_window, LV_EVENT_CLICKED, btn);
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
}

/*for testing only==================================================*/

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
      lv_obj_set_size(window, 300, 200);
      lv_obj_align(window, LV_ALIGN_CENTER, 0 , 10);
      
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
/*for testing only==================================================*/

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
  lv_obj_set_size(addBtn, 50, 50);
  lv_obj_add_flag(addBtn, LV_OBJ_FLAG_FLOATING);
  lv_obj_align(addBtn, LV_ALIGN_BOTTOM_RIGHT, 0, -lv_obj_get_style_pad_right(list, LV_PART_MAIN));
  lv_obj_add_event_cb(addBtn, add_btn_event_cb, LV_EVENT_ALL, list);
  lv_obj_set_style_radius(addBtn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_img_src(addBtn, LV_SYMBOL_PLUS, 0);
  lv_obj_set_style_text_font(addBtn, lv_theme_get_font_large(addBtn), 0);
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
}

AppContactList::~AppContactList(){

}