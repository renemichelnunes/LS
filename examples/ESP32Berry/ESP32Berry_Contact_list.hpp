#pragma once
#include "ESP32Berry_Config.hpp"
#include "ESP32Berry_AppBase.hpp"
#include "Contacts.hpp"
#include <RadioLib.h>
#include "lora_radio.hpp"

class AppContactList : public AppBase {
private:
  lv_style_t msgStyle;
  lv_obj_t *addBtn;
  lv_obj_t *list;
  lv_obj_t * configbtn;
  int display_width;
  void draw_ui();
  
  TaskHandle_t lvgl_task_handle;

public:
  SemaphoreHandle_t bin_sem;
  AppContactList(Display *display, System *system, Network *network, const char *title);
  ~AppContactList();
  Contact_list contact_list;
  void refresh_contact_list();
  void add_contact();
  void tg_event_handler(lv_event_t *e);
  lv_obj_t * getList();
  void close_app();
  void lv_port_sem_take(void);
  void lv_port_sem_give(void);
};