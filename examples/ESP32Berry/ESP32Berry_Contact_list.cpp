#include "ESP32Berry_Contact_list.hpp"

static AppContactList *instance = NULL;

AppContactList::AppContactList(Display* display, System* system, Network* network, 
const char* title) : AppBase(display, system, network, title){
  instance = this;
  display_width = display->get_display_width();
  this->draw_ui();
  this->contact_list = Contact_list();
}

/*for testing only==================================================*/
static uint32_t btn_cnt = 1;

static void add(lv_event_t* e){
  lv_obj_t* addBtn = lv_event_get_target(e);
  lv_obj_t* window = (lv_obj_t*)lv_event_get_user_data(e);
  string name, lora_addr;


  lv_obj_t* txtName = lv_obj_get_child(window, 0);
  Serial.print("Name: ");
  name = lv_textarea_get_text(txtName);
  Serial.println(name.c_str());
  lv_obj_t* txtLoRaAddr = lv_obj_get_child(window, 1);
  Serial.print("LoRa Address: ");
  lora_addr = lv_textarea_get_text(txtLoRaAddr);
  Serial.println(lora_addr.c_str());
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
}

static void add_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * float_btn = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
      lv_obj_t* window = lv_obj_create(lv_scr_act());
      lv_obj_set_size(window, 200, 200);
      lv_obj_align(window, LV_ALIGN_CENTER, 0 , 0);
      
      lv_obj_t* txtName = lv_textarea_create(window);
      lv_obj_set_size(txtName, 160, 40);
      lv_textarea_set_placeholder_text(txtName, "Name");
      lv_obj_align(txtName, LV_ALIGN_TOP_MID, 0, 0);

      lv_obj_t* txtLoraAddr = lv_textarea_create(window);
      lv_obj_set_size(txtLoraAddr, 160, 40);
      lv_textarea_set_placeholder_text(txtLoraAddr, "LoRa Addr");
      lv_obj_align(txtLoraAddr, LV_ALIGN_TOP_MID, 0, 40);

      lv_obj_t* btnAdd = lv_btn_create(window);
      lv_obj_set_size(btnAdd, 40, 40);
      lv_obj_align(btnAdd, LV_ALIGN_BOTTOM_LEFT, 0, 0);

      lv_obj_t* lblBtnAdd = lv_label_create(btnAdd);
      lv_label_set_text(lblBtnAdd, "OK");
      lv_obj_align(lblBtnAdd, LV_ALIGN_CENTER, 0, 0);
      
      lv_obj_t * list = (lv_obj_t*)lv_event_get_user_data(e);
      lv_obj_add_event_cb(btnAdd, add, LV_EVENT_CLICKED, window);
      char buf[32];
      lv_snprintf(buf, sizeof(buf), "Contact %d", (int)btn_cnt);
      lv_obj_t * list_btn = lv_list_add_btn(list, LV_SYMBOL_CALL, buf);
      btn_cnt++;

      lv_obj_move_foreground(float_btn);
      lv_obj_scroll_to_view(list_btn, LV_ANIM_ON);
      
    }
}
/*for testing only==================================================*/

void AppContactList::draw_ui(){
  Serial.println("Contacts");
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
  //lv_list_add_text(list, "Broadcast");
  // The floating button
  lv_obj_t* addBtn = lv_btn_create(list);
  lv_obj_set_size(addBtn, 50, 50);
  lv_obj_add_flag(addBtn, LV_OBJ_FLAG_FLOATING);
  lv_obj_align(addBtn, LV_ALIGN_BOTTOM_RIGHT, 0, -lv_obj_get_style_pad_right(list, LV_PART_MAIN));
  lv_obj_add_event_cb(addBtn, add_btn_event_cb, LV_EVENT_ALL, list);
  lv_obj_set_style_radius(addBtn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_img_src(addBtn, LV_SYMBOL_PLUS, 0);
  lv_obj_set_style_text_font(addBtn, lv_theme_get_font_large(addBtn), 0);
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