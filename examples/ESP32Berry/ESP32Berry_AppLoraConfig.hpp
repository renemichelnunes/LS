#pragma once
#include "ESP32Berry_Config.hpp"
#include "ESP32Berry_AppBase.hpp"
#include "lora_radio.hpp"

class AppLoraConfig{
private:
  lv_obj_t * window;
  lv_obj_t * list;
  void draw_ui();

public:
  AppLoraConfig();
  ~AppLoraConfig();
};