#include <Arduino.h>
#include "utilities.h"
#include <Wire.h>
#include <TFT_eSPI.h>
#include <RadioLib.h>
#include <ui.hpp>
#include <Contacts.hpp>
#include "SPIFFS.h"
#include "lora_messages.hpp"

struct lora_packet2{
    char id[7] = "aaaa";
    bool me = false;
    char msg[200] = "testando";
    char status[7] = "sent";
}my_packet;


#define TOUCH_MODULES_GT911
#include "TouchLib.h"

#define RADIO_FREQ 915.0
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

uint8_t touchAddress = GT911_SLAVE_ADDRESS1;
TouchLib *touch = NULL;
TFT_eSPI tft;
SemaphoreHandle_t xSemaphore = NULL;
bool touchDected = false;
bool kbDected = false;
bool hasRadio = false; 
volatile bool gotPacket = false;
lv_indev_t *touch_indev = NULL;
lv_indev_t *kb_indev = NULL;
TaskHandle_t thproc_recv_pkt = NULL, 
             check_new_msg_task = NULL;

Contact_list contacts_list = Contact_list();
lora_incomming_messages messages_list = lora_incomming_messages();

static void loadContacts(){
  if(!SPIFFS.begin(true)){
    Serial.println("failed mounting SPIFFS");
    return;
  }

  fs::File file = SPIFFS.open("/contacts", FILE_READ);
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
    Serial.println(c.getID());
    Serial.println(c.getName());
    contacts_list.add(c);
  }
  Serial.print(contacts_list.size());
  Serial.println(" contacts found");
}

static void saveContacts(){
  if(!SPIFFS.begin(true)){
    Serial.println("failed mounting SPIFFS");
    return;
  }

  fs::File file = SPIFFS.open("/contacts", FILE_WRITE);
  if(!file){
    Serial.println("contacts file problem");
    return;
  }

  Contact c;

  for(uint32_t index = 0; index < contacts_list.size(); index++){
    c = contacts_list.getContact(index);
    file.println(c.getName());
    file.println(c.getID());
  }
  Serial.print(contacts_list.size());
  Serial.println(" contacts saved");
  file.close();
}

static void refresh_contact_list(){
    lv_obj_clean(frm_contacts_list);
    lv_obj_t * btn;
    for(uint32_t i = 0; i < contacts_list.size(); i++){
        btn = lv_list_add_btn(frm_contacts_list, LV_SYMBOL_CALL, contacts_list.getContact(i).getName().c_str());
        lv_obj_add_event_cb(btn, show_edit_contacts, LV_EVENT_LONG_PRESSED, btn);
        lv_obj_add_event_cb(btn, show_chat, LV_EVENT_SHORT_CLICKED, btn);
    }
    saveContacts();
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

static void disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );
    if ( xSemaphoreTake( xSemaphore, portMAX_DELAY ) == pdTRUE ) {
        tft.startWrite();
        tft.setAddrWindow( area->x1, area->y1, w, h );
        tft.pushColors( ( uint16_t * )&color_p->full, w * h, false );
        tft.endWrite();
        lv_disp_flush_ready( disp );
        xSemaphoreGive( xSemaphore );
    }
}

static bool getTouch(int16_t &x, int16_t &y)
{
    uint8_t rotation = tft.getRotation();
    if (!touch->read()) {
        return false;
    }
    TP_Point t = touch->getPoint(0);
    switch (rotation) {
    case 1:
        x = t.y;
        y = tft.height() - t.x;
        break;
    case 2:
        x = tft.width() - t.x;
        y = tft.height() - t.y;
        break;
    case 3:
        x = tft.width() - t.y;
        y = t.x;
        break;
    case 0:
    default:
        x = t.x;
        y = t.y;
    }
    //Serial.printf("R:%d X:%d Y:%d\n", rotation, x, y);
    return true;
}

static void touchpad_read( lv_indev_drv_t *indev_driver, lv_indev_data_t *data )
{
    data->state = getTouch(data->point.x, data->point.y) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}

static uint32_t keypad_get_key(void)
{
    char key_ch = 0;
    Wire.requestFrom(0x55, 1);
    while (Wire.available() > 0) {
        key_ch = Wire.read();
        
    }
    return key_ch;
}

static void keypad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    static uint32_t last_key = 0;
    uint32_t act_key ;
    act_key = keypad_get_key();
    if (act_key != 0) {
        data->state = LV_INDEV_STATE_PR;
        //Serial.printf("Key pressed : 0x%x\n", act_key);
        last_key = act_key;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
    data->key = last_key;
}

void setupLvgl()
{
    static lv_disp_draw_buf_t draw_buf;

#ifndef BOARD_HAS_PSRAM
#define LVGL_BUFFER_SIZE    ( TFT_HEIGHT * 100 )
    static lv_color_t buf[ LVGL_BUFFER_SIZE ];
#else
#define LVGL_BUFFER_SIZE    (TFT_WIDTH * TFT_HEIGHT * sizeof(lv_color_t))
    static lv_color_t *buf = (lv_color_t *)ps_malloc(LVGL_BUFFER_SIZE);
    if (!buf) {
        Serial.println("menory alloc failed!");
        delay(5000);
        assert(buf);
    }
#endif


    String LVGL_Arduino = "LVGL ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.println( LVGL_Arduino );

    lv_init();

    lv_group_set_default(lv_group_create());

    lv_disp_draw_buf_init( &draw_buf, buf, NULL, LVGL_BUFFER_SIZE );

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );

    /*Change the following line to your display resolution*/
    disp_drv.hor_res = TFT_HEIGHT;
    disp_drv.ver_res = TFT_WIDTH;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
#ifdef BOARD_HAS_PSRAM
    disp_drv.full_refresh = 1;
#endif
    lv_disp_drv_register( &disp_drv );

    /*Initialize the  input device driver*/

    /*Register a touchscreen input device*/
    if (touchDected) {
        static lv_indev_drv_t indev_touchpad;
        lv_indev_drv_init( &indev_touchpad );
        indev_touchpad.type = LV_INDEV_TYPE_POINTER;
        indev_touchpad.read_cb = touchpad_read;
        touch_indev = lv_indev_drv_register( &indev_touchpad );
    }

    if (kbDected) {
        Serial.println("Keyboard registered!!");
        /*Register a keypad input device*/
        static lv_indev_drv_t indev_keypad;
        lv_indev_drv_init(&indev_keypad);
        indev_keypad.type = LV_INDEV_TYPE_KEYPAD;
        indev_keypad.read_cb = keypad_read;
        kb_indev = lv_indev_drv_register(&indev_keypad);
        lv_indev_set_group(kb_indev, lv_group_get_default());
    }

}

void scanDevices(TwoWire *w)
{
    uint8_t err, addr;
    int nDevices = 0;
    uint32_t start = 0;
    for (addr = 1; addr < 127; addr++) {
        start = millis();
        w->beginTransmission(addr); delay(2);
        err = w->endTransmission();
        if (err == 0) {
            nDevices++;
            Serial.print("I2C device found at address 0x");
            if (addr < 16) {
                Serial.print("0");
            }
            Serial.print(addr, HEX);
            Serial.println(" !");

            if (addr == GT911_SLAVE_ADDRESS2) {
                touchAddress = GT911_SLAVE_ADDRESS2;
                Serial.println("Find GT911 Drv Slave address: 0x14");
            } else if (addr == GT911_SLAVE_ADDRESS1) {
                touchAddress = GT911_SLAVE_ADDRESS1;
                Serial.println("Find GT911 Drv Slave address: 0x5D");
            }
        } else if (err == 4) {
            Serial.print("Unknow error at address 0x");
            if (addr < 16) {
                Serial.print("0");
            }
            Serial.println(addr, HEX);
        }
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
}

bool checkKb()
{
    Wire.requestFrom(0x55, 1);
    return Wire.read() != -1;
}

void processReceivedPacket(void * param){
    lora_packet2 p;
    while(true){
        if(gotPacket){
            if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
                radio.standby();
                Serial.println("packet received");
                uint32_t size = radio.getPacketLength();
                Serial.print("size ");
                Serial.println(size);
                if(size > 0){
                    radio.readData((uint8_t *)&p, size);
                    Serial.print("id ");
                    Serial.println(p.id);
                    Serial.print("msg ");
                    Serial.println(p.msg);
                    Serial.print("status ");
                    Serial.println(p.status);
                    Serial.print("me ");
                    Serial.println(p.me ? "true": "false");
                }
                gotPacket = false;
                radio.startReceive();
                xSemaphoreGive(xSemaphore);
            }
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void listen(){
    gotPacket = true;
}

void setupRadio(lv_event_t * e)
{
    digitalWrite(BOARD_SDCARD_CS, HIGH);
    digitalWrite(RADIO_CS_PIN, HIGH);
    digitalWrite(BOARD_TFT_CS, HIGH);
    SPI.end();
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); //SD

    int state = radio.begin(RADIO_FREQ);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Start Radio success!");
    } else {
        Serial.print("Start Radio failed,code:");
        Serial.println(state);
        //return false;
    }

    hasRadio = true;

    // set carrier frequency to 868.0 MHz
    if (radio.setFrequency(RADIO_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Serial.println(F("Selected frequency is invalid for this module!"));
        //return false;
    }

    // set bandwidth to 250 kHz
    if (radio.setBandwidth(250.0) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        Serial.println(F("Selected bandwidth is invalid for this module!"));
        //return false;
    }

    // set spreading factor to 10
    if (radio.setSpreadingFactor(10) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        Serial.println(F("Selected spreading factor is invalid for this module!"));
        //return false;
    }

    // set coding rate to 6
    if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        Serial.println(F("Selected coding rate is invalid for this module!"));
        //return false;
    }

    // set LoRa sync word to 0xAB
    if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
        Serial.println(F("Unable to set sync word!"));
        //return false;
    }

    // set output power to 10 dBm (accepted range is -17 - 22 dBm)
    if (radio.setOutputPower(17) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("Selected output power is invalid for this module!"));
        //return false;
    }

    // set over current protection limit to 140 mA (accepted range is 45 - 140 mA)
    // NOTE: set value to 0 to disable overcurrent protection
    if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
        Serial.println(F("Selected current limit is invalid for this module!"));
        //return false;
    }

    // set LoRa preamble length to 15 symbols (accepted range is 0 - 65535)
    if (radio.setPreambleLength(15) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
        Serial.println(F("Selected preamble length is invalid for this module!"));
        //return false;
    }

    // disable CRC
    if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
        Serial.println(F("Selected CRC is invalid for this module!"));
        //return false;
    }

    xTaskCreatePinnedToCore(processReceivedPacket, "proc_recv_pkt", 10000, NULL, 1, &thproc_recv_pkt, 1);
    radio.setPacketReceivedAction(listen);
    radio.startReceive();
    //return true;

}

void test(lv_event_t * e){
    lora_packet2 dummy;
    if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
        if(hasRadio){
            int state = radio.startTransmit((uint8_t *)&my_packet, sizeof(lora_packet2));
            
            if(state != RADIOLIB_ERR_NONE){
                Serial.print("transmission failed ");
                Serial.println(state);
            }else
                Serial.println("transmitted");
            // clear the cache
            radio.startTransmit((uint8_t *)&dummy, sizeof(lora_packet2));
        }
        xSemaphoreGive(xSemaphore);
    }
}

void hide_contacts_frm(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_contacts != NULL){
            lv_obj_add_flag(frm_contacts, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void show_contacts_form(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_contacts != NULL){
            lv_obj_clear_flag(frm_contacts, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void hide_add_contacts_frm(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_add_contact != NULL){
            lv_obj_add_flag(frm_add_contact, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void show_add_contacts_frm(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_add_contact != NULL){
            lv_obj_clear_flag(frm_add_contact, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

Contact * actual_contact = NULL;
void hide_edit_contacts(lv_event_t * e){
    Contact c;
    const char * name, * id;
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_edit_contacts != NULL){
            name = lv_textarea_get_text(frm_edit_text_name);
            id = lv_textarea_get_text(frm_edit_text_ID);
            c.setName(lv_textarea_get_text(frm_edit_text_name));
            if(strcmp(name, "") != 0 && strcmp(id, "") != 0){
                actual_contact->setID(id);
                Serial.println("ID updated");
                if(strcmp(name, actual_contact->getName().c_str()) != 0)
                    if(!contacts_list.find(c))
                        actual_contact->setName(name);
                    else{
                        Serial.println("Name already exists");
                    }
                actual_contact = NULL;
                lv_obj_add_flag(frm_edit_contacts, LV_OBJ_FLAG_HIDDEN);
                Serial.println("Contact updated");
                refresh_contact_list();
            }else
                Serial.println("Name or ID empty");
        }
    }
}

void show_edit_contacts(lv_event_t * e){
    const char * name;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = (lv_obj_t * )lv_event_get_user_data(e);
    lv_obj_t * lbl = lv_obj_get_child(btn, 1);

    if(code == LV_EVENT_LONG_PRESSED){
        if(frm_edit_contacts != NULL){
            name = lv_label_get_text(lbl);
            actual_contact = contacts_list.getContactByName(name);
            lv_obj_clear_flag(frm_edit_contacts, LV_OBJ_FLAG_HIDDEN);
            lv_textarea_set_text(frm_edit_text_name, actual_contact->getName().c_str());
            lv_textarea_set_text(frm_edit_text_ID, actual_contact->getID().c_str());
        }
    }
}

uint32_t msg_count = 0;
void hide_chat(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_chat != NULL){
            if(check_new_msg_task != NULL){
                vTaskDelete(check_new_msg_task);
                check_new_msg_task = NULL;
                Serial.println("check_new_msg_task finished");
            }
            lv_obj_clean(frm_chat_list);
            msg_count = 0;
            lv_obj_add_flag(frm_chat, LV_OBJ_FLAG_HIDDEN);
            actual_contact = NULL;
        }
    }
}

void show_chat(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_t * lbl = lv_obj_get_child(btn, 1);
    const char * name = lv_label_get_text(lbl);
    actual_contact = contacts_list.getContactByName(name);

    char title[31] = "Chat with ";
    strcat(title, name);
    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_chat != NULL){
            lv_obj_clear_flag(frm_chat, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(frm_chat_btn_title_lbl, title);
            if(check_new_msg_task == NULL){
                xTaskCreatePinnedToCore(check_new_msg, "check_new_msg", 10000, NULL, 1, &check_new_msg_task, 1);
                Serial.println("check_new_msg_task running");
                Serial.print("actual contact is ");
                Serial.println(actual_contact->getName());
            }
        }
    }
}

void send_message(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lora_packet dummy;

    if(code == LV_EVENT_SHORT_CLICKED){
        lora_packet pkt;
        strcpy(pkt.id, actual_contact->getID().c_str());
        strcpy(pkt.status, "send");
        strcpy(pkt.msg, lv_textarea_get_text(frm_chat_text_ans));
        
        if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
            if(hasRadio){
                if(strcmp(lv_textarea_get_text(frm_chat_text_ans), "") != 0){
                    int state = radio.startTransmit((uint8_t *)&pkt, sizeof(lora_packet));
                
                    if(state != RADIOLIB_ERR_NONE){
                        Serial.print("transmission failed ");
                        Serial.println(state);
                    }else
                        Serial.println("transmitted");
                    // clear the cache
                    radio.startTransmit((uint8_t *)&dummy, sizeof(lora_packet));
                    // add the message to the list of messages
                    pkt.me = true;
                    Serial.println(pkt.id);
                    messages_list.addMessage(pkt);
                    lv_textarea_set_text(frm_chat_text_ans, "");
                }
            }
            xSemaphoreGive(xSemaphore);
        }
    }
}

void check_new_msg(void * param){
    vector<lora_packet> caller_msg;
    uint32_t actual_count = 0;
    lv_obj_t * btn = NULL, * lbl = NULL;
    
    while(true){
        caller_msg = messages_list.getMessages(actual_contact->getID().c_str());
        actual_count = caller_msg.size();
        if(actual_count > msg_count){
            Serial.println("new messages");
            for(uint32_t i = msg_count; i < actual_count; i++){
                if(caller_msg[i].me){
                    Serial.print("me: ");
                    lv_list_add_text(frm_chat_list, "Me");
                }else{
                    lv_list_add_text(frm_chat_list, actual_contact->getName().c_str());
                }
                Serial.println(caller_msg[i].msg);
                btn = lv_list_add_btn(frm_chat_list, NULL, caller_msg[i].msg);
                lbl = lv_obj_get_child(btn, 0);
                lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
                lv_obj_scroll_to_view(btn, LV_ANIM_OFF);
            }
            msg_count = actual_count;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void add_contact(lv_event_t * e){
    const char * name, * id;
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_SHORT_CLICKED){
        id = lv_textarea_get_text(frm_add_contact_textarea_id);
        name = lv_textarea_get_text(frm_add_contact_textarea_name);
        if(name != "" && id != ""){
            Contact c = Contact(name, id);
            if(!contacts_list.find(c)){
                if(contacts_list.add(c)){
                    Serial.println("Contact added");
                    refresh_contact_list();
                    lv_obj_add_flag(frm_add_contact, LV_OBJ_FLAG_HIDDEN);
                }
                else      
                    Serial.println("failed to add contact");
            }else
                Serial.println("Conatct already exists");
        }
    }
}

void del_contact(lv_event_t * e){
    const char * name;
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        name = lv_textarea_get_text(frm_edit_text_name);
        if(name != ""){
            Contact * c = contacts_list.getContactByName(name);
            if(c != NULL){
                if(contacts_list.del(*c)){
                    refresh_contact_list();
                    lv_obj_add_flag(frm_edit_contacts, LV_OBJ_FLAG_HIDDEN);
                    Serial.println("Contact deleted");
                }
            }else
                Serial.println("Contact not found");
        }
    }else
        Serial.println("Name is empty");
}

void ui(){
    //style
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_color_hex(0xE95622), lv_palette_main(LV_PALETTE_RED), false, &lv_font_montserrat_14);
    lv_disp_set_theme(dispp, theme);

    // Home screen**************************************************************
    init_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(init_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(init_screen, LV_OBJ_FLAG_SCROLLABLE);
    //lv_obj_set_style_bg_color(init_screen, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Contacts button
    btn_contacts = lv_btn_create(init_screen);
    lv_obj_set_size(btn_contacts, 100, 25);
    lv_obj_set_align(btn_contacts, LV_ALIGN_BOTTOM_LEFT);
    //lv_obj_set_pos(btn_contacts, 10, -10);
    lbl_btn_contacts = lv_label_create(btn_contacts);
    lv_label_set_text(lbl_btn_contacts, "Contacts");
    lv_obj_align(lbl_btn_contacts, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_contacts, show_contacts_form, LV_EVENT_SHORT_CLICKED, NULL);
    
    // Test button
    btn_test = lv_btn_create(init_screen);
    lv_obj_set_size(btn_test, 60, 25);
    lv_obj_set_align(btn_test, LV_ALIGN_BOTTOM_RIGHT);
    //lv_obj_set_pos(btn_contacts, 10, -10);
    lbl_btn_test = lv_label_create(btn_test);
    lv_label_set_text(lbl_btn_test, "Test");
    lv_obj_align(lbl_btn_test, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_test, test, LV_EVENT_SHORT_CLICKED, NULL);

    // Contacts form**************************************************************
    frm_contacts = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_contacts, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(frm_contacts, LV_OBJ_FLAG_SCROLLABLE);

    // Title button
    frm_contatcs_btn_title = lv_btn_create(frm_contacts);
    lv_obj_set_size(frm_contatcs_btn_title, 200, 20);
    lv_obj_align(frm_contatcs_btn_title, LV_ALIGN_TOP_LEFT, -15, -15);
    
    frm_contatcs_btn_title_lbl = lv_label_create(frm_contatcs_btn_title);
    lv_label_set_text(frm_contatcs_btn_title_lbl, "Contacts");
    lv_obj_set_align(frm_contatcs_btn_title_lbl, LV_ALIGN_LEFT_MID);

    // Close button
    frm_contacts_btn_back = lv_btn_create(frm_contacts);
    lv_obj_set_size(frm_contacts_btn_back, 50, 20);
    lv_obj_align(frm_contacts_btn_back, LV_ALIGN_TOP_RIGHT, 15, -15);
    //lv_obj_set_pos(btn_contacts, 10, -10);
    frm_contacts_btn_back_lbl = lv_label_create(frm_contacts_btn_back);
    lv_label_set_text(frm_contacts_btn_back_lbl, "Back");
    lv_obj_set_align(frm_contacts_btn_back_lbl, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(frm_contacts_btn_back, hide_contacts_frm, LV_EVENT_SHORT_CLICKED, NULL);

    // Contact list
    frm_contacts_list = lv_list_create(frm_contacts);
    lv_obj_set_size(frm_contacts_list, LV_HOR_RES, 220);
    lv_obj_align(frm_contacts_list, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_border_opa(frm_contacts_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(frm_contacts_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    refresh_contact_list();

    // Add contact button
    frm_contacts_btn_add = lv_btn_create(frm_contacts);
    lv_obj_set_size(frm_contacts_btn_add, 50, 20);
    lv_obj_align(frm_contacts_btn_add, LV_ALIGN_BOTTOM_RIGHT, 15, 15);
    
    frm_contacts_btn_add_lbl = lv_label_create(frm_contacts_btn_add);
    lv_label_set_text(frm_contacts_btn_add_lbl, "Add");
    lv_obj_set_align(frm_contacts_btn_add_lbl, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(frm_contacts_btn_add, show_add_contacts_frm, LV_EVENT_SHORT_CLICKED, NULL);

    // Hide contacts form
    lv_obj_add_flag(frm_contacts, LV_OBJ_FLAG_HIDDEN);

    // Add contacts form**************************************************************
    frm_add_contact = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_add_contact, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(frm_add_contact, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    frm_add_contacts_btn_title = lv_btn_create(frm_add_contact);
    lv_obj_set_size(frm_add_contacts_btn_title, 200, 20);
    lv_obj_align(frm_add_contacts_btn_title, LV_ALIGN_TOP_LEFT, -15, -15);

    frm_add_contacts_btn_title_lbl = lv_label_create(frm_add_contacts_btn_title);
    lv_label_set_text(frm_add_contacts_btn_title_lbl, "Add contact");
    lv_obj_set_align(frm_add_contacts_btn_title_lbl, LV_ALIGN_LEFT_MID);

    // id text input
    frm_add_contact_textarea_id = lv_textarea_create(frm_add_contact);
    lv_obj_set_size(frm_add_contact_textarea_id, 150, 30);
    lv_obj_align(frm_add_contact_textarea_id, LV_ALIGN_TOP_LEFT, 0, 25);
    lv_textarea_set_placeholder_text(frm_add_contact_textarea_id, "ID");

    // name text input
    frm_add_contact_textarea_name = lv_textarea_create(frm_add_contact);
    lv_obj_set_size(frm_add_contact_textarea_name, 240, 30);
    lv_obj_align(frm_add_contact_textarea_name, LV_ALIGN_TOP_LEFT, 0, 60);
    lv_textarea_set_placeholder_text(frm_add_contact_textarea_name, "Name");

    // add button
    frm_add_contacts_btn_add = lv_btn_create(frm_add_contact);
    lv_obj_set_size(frm_add_contacts_btn_add, 50, 20);
    lv_obj_align(frm_add_contacts_btn_add, LV_ALIGN_BOTTOM_RIGHT, 15, 15);

    frm_add_contacts_btn_add_lbl = lv_label_create(frm_add_contacts_btn_add);
    lv_label_set_text(frm_add_contacts_btn_add_lbl, "Add");
    lv_obj_set_align(frm_add_contacts_btn_add_lbl, LV_ALIGN_CENTER);

    lv_obj_add_event_cb(frm_add_contacts_btn_add, add_contact, LV_EVENT_SHORT_CLICKED, NULL);

    // Close button
    btn_frm_add_contatcs_close = lv_btn_create(frm_add_contact);
    lv_obj_set_size(btn_frm_add_contatcs_close, 50, 20);
    lv_obj_align(btn_frm_add_contatcs_close, LV_ALIGN_TOP_RIGHT, 15, -15);
    
    lbl_btn_frm_add_contacts_close = lv_label_create(btn_frm_add_contatcs_close);
    lv_label_set_text(lbl_btn_frm_add_contacts_close, "Back");
    lv_obj_align(lbl_btn_frm_add_contacts_close, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_frm_add_contatcs_close, hide_add_contacts_frm, LV_EVENT_SHORT_CLICKED, NULL);
    
    // hide add contact form
    lv_obj_add_flag(frm_add_contact, LV_OBJ_FLAG_HIDDEN);

    // Edit contacts form**************************************************************
    frm_edit_contacts = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_edit_contacts, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(frm_edit_contacts, LV_OBJ_FLAG_SCROLLABLE);

    //title
    frm_edit_btn_title = lv_btn_create(frm_edit_contacts);
    lv_obj_set_size(frm_edit_btn_title, 200, 20);
    lv_obj_align(frm_edit_btn_title, LV_ALIGN_TOP_LEFT, -15, -15);

    frm_edit_btn_title_lbl = lv_label_create(frm_edit_btn_title);
    lv_label_set_text(frm_edit_btn_title_lbl, "Edit contact");
    lv_obj_set_align(frm_edit_btn_title_lbl, LV_ALIGN_LEFT_MID);

    // back button
    frm_edit_btn_back = lv_btn_create(frm_edit_contacts);
    lv_obj_set_size(frm_edit_btn_back, 50, 20);
    lv_obj_align(frm_edit_btn_back, LV_ALIGN_TOP_RIGHT, 15, -15);

    frm_edit_btn_back_lbl = lv_label_create(frm_edit_btn_back);
    lv_label_set_text(frm_edit_btn_back_lbl, "Back");
    lv_obj_set_align(frm_edit_btn_back_lbl, LV_ALIGN_CENTER);

    lv_obj_add_event_cb(frm_edit_btn_back, hide_edit_contacts, LV_EVENT_SHORT_CLICKED, NULL);

    //Del button
    frm_edit_btn_del = lv_btn_create(frm_edit_contacts);
    lv_obj_set_size(frm_edit_btn_del, 50, 20);
    lv_obj_align(frm_edit_btn_del, LV_ALIGN_BOTTOM_RIGHT, 15, 15);
    lv_obj_add_event_cb(frm_edit_btn_del, del_contact, LV_EVENT_SHORT_CLICKED, NULL);

    frm_edit_btn_del_lbl = lv_label_create(frm_edit_btn_del);
    lv_label_set_text(frm_edit_btn_del_lbl, "Del");
    lv_obj_set_align(frm_edit_btn_del_lbl, LV_ALIGN_CENTER);

    // ID text input
    frm_edit_text_ID = lv_textarea_create(frm_edit_contacts);
    lv_obj_set_size(frm_edit_text_ID, 150, 30);
    lv_obj_align(frm_edit_text_ID, LV_ALIGN_TOP_LEFT, 0, 25);
    lv_textarea_set_placeholder_text(frm_edit_text_ID, "ID");

    // Name text input
    frm_edit_text_name = lv_textarea_create(frm_edit_contacts);
    lv_obj_set_size(frm_edit_text_name, 240, 30);
    lv_obj_align(frm_edit_text_name, LV_ALIGN_TOP_LEFT, 0, 60);
    lv_textarea_set_placeholder_text(frm_edit_text_name, "Name");

    lv_obj_add_flag(frm_edit_contacts, LV_OBJ_FLAG_HIDDEN);

    // Chat form**************************************************************
    frm_chat = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_chat, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(frm_chat, LV_OBJ_FLAG_SCROLLABLE);

    //title
    frm_chat_btn_title = lv_btn_create(frm_chat);
    lv_obj_set_size(frm_chat_btn_title, 200, 20);
    lv_obj_align(frm_chat_btn_title, LV_ALIGN_OUT_TOP_LEFT, -15, -15);

    frm_chat_btn_title_lbl = lv_label_create(frm_chat_btn_title);
    lv_label_set_text(frm_chat_btn_title_lbl, "chat with ");
    lv_obj_set_align(frm_chat_btn_title_lbl, LV_ALIGN_LEFT_MID);

    //back button
    frm_chat_btn_back = lv_btn_create(frm_chat);
    lv_obj_set_size(frm_chat_btn_back, 50, 20);
    lv_obj_align(frm_chat_btn_back, LV_ALIGN_TOP_RIGHT, 15, -15);

    frm_chat_btn_back_lbl = lv_label_create(frm_chat_btn_back);
    lv_label_set_text(frm_chat_btn_back_lbl, "Back");
    lv_obj_set_align(frm_chat_btn_back_lbl, LV_ALIGN_CENTER);

    lv_obj_add_event_cb(frm_chat_btn_back, hide_chat, LV_EVENT_SHORT_CLICKED, NULL);

    //list
    frm_chat_list = lv_list_create(frm_chat);
    lv_obj_set_size(frm_chat_list, 320, 170);
    lv_obj_align(frm_chat_list, LV_ALIGN_TOP_MID, 0, 5);

    //answer text input
    frm_chat_text_ans = lv_textarea_create(frm_chat);
    lv_obj_set_size(frm_chat_text_ans, 260, 50);
    lv_obj_align(frm_chat_text_ans, LV_ALIGN_BOTTOM_LEFT, -15, 15);
    lv_textarea_set_max_length(frm_chat_text_ans, 199);
    lv_textarea_set_placeholder_text(frm_chat_text_ans, "Answer");

    //send button
    frm_chat_btn_send = lv_btn_create(frm_chat);
    lv_obj_set_size(frm_chat_btn_send, 50, 50);
    lv_obj_align(frm_chat_btn_send, LV_ALIGN_BOTTOM_RIGHT, 15, 15);
    lv_obj_add_event_cb(frm_chat_btn_send, send_message, LV_EVENT_SHORT_CLICKED, NULL);

    frm_chat_btn_send_lbl = lv_label_create(frm_chat_btn_send);
    lv_label_set_text(frm_chat_btn_send_lbl, "Send");
    lv_obj_set_align(frm_chat_btn_send_lbl, LV_ALIGN_CENTER);

    lv_obj_add_flag(frm_chat, LV_OBJ_FLAG_HIDDEN);

    // Settings form**************************************************************
    
}

void setup(){
    bool ret = false;
    Serial.begin(115200);
    delay(2000);

    //Load contacts
    loadContacts();

    pinMode(BOARD_POWERON, OUTPUT);
    digitalWrite(BOARD_POWERON, HIGH);

    pinMode(BOARD_SDCARD_CS, OUTPUT);
    pinMode(RADIO_CS_PIN, OUTPUT);
    pinMode(BOARD_TFT_CS, OUTPUT);

    digitalWrite(BOARD_SDCARD_CS, HIGH);
    digitalWrite(RADIO_CS_PIN, HIGH);
    digitalWrite(BOARD_TFT_CS, HIGH);

    pinMode(BOARD_SPI_MISO, INPUT_PULLUP);
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);

    pinMode(BOARD_BOOT_PIN, INPUT_PULLUP);
    pinMode(BOARD_TBOX_G02, INPUT_PULLUP);
    pinMode(BOARD_TBOX_G01, INPUT_PULLUP);
    pinMode(BOARD_TBOX_G04, INPUT_PULLUP);
    pinMode(BOARD_TBOX_G03, INPUT_PULLUP);

    pinMode(BOARD_TOUCH_INT, OUTPUT);
    digitalWrite(BOARD_TOUCH_INT, HIGH);

    pinMode(BOARD_TOUCH_INT, INPUT); 
    delay(200);

    setupRadio(NULL);

    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    //scanDevices(&Wire);

    touch = new TouchLib(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, touchAddress);
    touch->init();
    Wire.beginTransmission(touchAddress);
    ret = Wire.endTransmission() == 0;
    touchDected = ret;
    if(touchDected)
        Serial.println("touch detected");
    else 
        Serial.println("touch not detected");

    tft.begin();
    tft.setRotation( 1 );
    tft.fillScreen(TFT_BLUE);

    xSemaphore = xSemaphoreCreateBinary();
    assert(xSemaphore);
    xSemaphoreGive( xSemaphore );

    Wire.beginTransmission(touchAddress);
    ret = Wire.endTransmission() == 0;
    touchDected = ret;

    kbDected = checkKb();

    setupLvgl();

    ui();
    // set brightness
    analogWrite(BOARD_BL_PIN, 100);

}

void loop(){
    lv_task_handler();
    delay(5);
}