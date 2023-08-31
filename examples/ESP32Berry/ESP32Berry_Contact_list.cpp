#include "ESP32Berry_Contact_list.hpp"

static AppContactList *instance = NULL;

AppContactList::AppContactList(Display* display, System* system, Network* network, 
const char* title) : AppBase(display, system, network, title){
  instance = this;
  display_width = display->get_display_width();
  this->draw_ui();
}

void AppContactList::draw_ui(){
  lv_style_init(&msgStyle);
  lv_style_set_bg_color(&msgStyle, lv_color_white());
  lv_style_set_pad_ver(&msgStyle, 8);
  lv_style_set_border_color(&msgStyle, lv_color_hex(0x989898));
  lv_style_set_border_width(&msgStyle, 2);
  lv_style_set_border_opa(&msgStyle, LV_OPA_50);
  lv_style_set_border_side(&msgStyle, LV_BORDER_SIDE_BOTTOM);

  ContactList = lv_list_create(ui_AppPanel);
  lv_obj_set_size(ContactList, display_width, 160);
  lv_obj_align_to(ContactList, ui_AppPanel, LV_ALIGN_OUT_TOP_MID, 0, -2);
  lv_obj_set_style_border_opa(ContactList, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(ContactList, 0, LV_PART_MAIN | LV_STATE_DEFAULT);


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