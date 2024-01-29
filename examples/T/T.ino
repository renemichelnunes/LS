#include <Arduino.h>
#include "utilities.h"
#include <Wire.h>
#include <TFT_eSPI.h>
#include <RadioLib.h>
#include <ui.hpp>
#include <Contacts.hpp>
#include "SPIFFS.h"
#include "lora_messages.hpp"
#include "notification.hpp"
#include <time.h>
#include "esp_wpa2.h"
#include "WiFi.h"
#include "esp32-hal.h"
#include "esp_adc_cal.h"
#include <vector>
#include "con_nets.hpp"
#include <Audio.h>
#include <driver/i2s.h>
#include "es7210.h"
#include "Cipher.h"
#include "esp_wifi.h"

LV_FONT_DECLARE(clocknum);
LV_FONT_DECLARE(ubuntu);
//LV_IMG_DECLARE(bg);
LV_IMG_DECLARE(bg2);
LV_IMG_DECLARE(icon_lora2);
LV_IMG_DECLARE(icon_mail);

vector <wifi_info> wifi_list;
Wifi_connected_nets wifi_connected_nets = Wifi_connected_nets();

#define TOUCH_MODULES_GT911
#include "TouchLib.h"
#include <lwip/apps/sntp.h>

#define RADIO_FREQ 915.0
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

Audio audio;
uint8_t touchAddress = GT911_SLAVE_ADDRESS1;
TouchLib *touch = NULL;
TFT_eSPI tft;
SemaphoreHandle_t xSemaphore = NULL;
bool touchDected = false;
bool kbDected = false;
bool hasRadio = false; 
bool wifi_connected = false;
volatile bool announcing = false;
volatile bool transmiting = false;
volatile bool processing = false;
volatile bool gotPacket = false;
lv_indev_t *touch_indev = NULL;
lv_indev_t *kb_indev = NULL;
TaskHandle_t task_recv_pkt = NULL, 
             task_check_new_msg = NULL,
             task_not = NULL,
             task_date_time = NULL,
             task_bat = NULL,
             task_wifi_auto = NULL,
             task_play = NULL,
             task_play_radio = NULL;

Contact_list contacts_list = Contact_list();
lora_incomming_messages messages_list = lora_incomming_messages();
notification notification_list = notification();

char user_name[50] = "";
char user_id[7] = "";
char connected_to[60] = "";

struct tm timeinfo;

uint32_t ui_primary_color = 0x5c81aa;
int vRef = 0;

void update_bat(void * param);
void datetime();
void wifi_auto_toggle();
char * wifi_auth_mode_to_str(wifi_auth_mode_t auth_mode);
uint last_wifi_con = -1;

Cipher * cipher = new Cipher();

static void loadSettings(){
    char color[7];
    if(!SPIFFS.begin(true)){
        Serial.println("failed mounting SPIFFS");
        return;
    }

    fs::File file = SPIFFS.open("/settings", FILE_READ);
    if(!file){
        Serial.println("couldn't open settings file");
        return;
    }
    try{
        vector<String> v;
        while(file.available()){
            v.push_back(file.readStringUntil('\n'));
        }

        file.close();

        if(v.size() == 0)
            return;
        for(uint32_t index = 0; index < v.size(); index++){
            v[index].remove(v[index].length() - 1);
        }
        if(v.size() > 2){
            strcpy(user_name, v[0].c_str());
            strcpy(user_id, v[1].c_str());
            v[2].toUpperCase();
            ui_primary_color = strtoul(v[2].c_str(), NULL, 16);
            lv_disp_t *dispp = lv_disp_get_default();
            lv_theme_t *theme = lv_theme_default_init(dispp, lv_color_hex(ui_primary_color), lv_palette_main(LV_PALETTE_RED), false, &lv_font_montserrat_14);
            lv_disp_set_theme(dispp, theme);
        }


        Serial.println("Settings loaded");
    }catch (exception &ex){
        Serial.println(ex.what());
    }
}

static void saveSettings(){
    if(!SPIFFS.begin(true)){
        Serial.println("failed mounting SPIFFS");
        return;
    }

    fs::File file = SPIFFS.open("/settings", FILE_WRITE);
    if(!file){
        Serial.println("couldn't open settings file");
        return;
    }

    file.println(lv_textarea_get_text(frm_settings_name));
    file.println(lv_textarea_get_text(frm_settings_id));
    file.println(lv_textarea_get_text(frm_settings_color));
    Serial.println("settings saved");
    file.close();
}

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

    Serial.println("Loading contacts...");
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
        lv_obj_t * lbl = lv_label_create(btn);
        lv_label_set_text(lbl, contacts_list.getContact(i).getID().c_str());
        lv_obj_t * obj_status = lv_obj_create(btn);
        lv_obj_set_size(obj_status, 20, 20);
        lv_obj_clear_flag(obj_status, LV_OBJ_FLAG_SCROLLABLE);
        if(contacts_list.getContact(i).inrange)
            lv_obj_set_style_bg_color(obj_status, lv_color_hex(0x00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
        else
            lv_obj_set_style_bg_color(obj_status, lv_color_hex(0xaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(obj_status, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
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

static void notify(void * param){
    char n[30] = {'\0'};
    while(true){
        update_wifi_icon();
        if(notification_list.size() > 0){
            //vTaskDelay(1000 / portTICK_PERIOD_MS);
            notification_list.pop(n);
            lv_task_handler();
            lv_obj_clear_flag(frm_not, LV_OBJ_FLAG_HIDDEN);
            lv_task_handler();
            lv_label_set_text(frm_not_lbl, n);
            lv_task_handler();
            lv_label_set_text(frm_home_title_lbl, n);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            lv_task_handler();
            lv_label_set_text(frm_not_lbl, "");
            lv_obj_add_flag(frm_not, LV_OBJ_FLAG_HIDDEN);
            lv_task_handler();
            strcpy(n, "");
            Serial.println("notified");
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
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
    lora_packet p;
    lora_packet_status c;
    lora_packet_status pong;
    Contact * contact = NULL;
    char message[200] = {'\0'}, pmsg [200] = {'\0'}, dec_msg[200] = {'\0'};
    
    while(announcing){
        Serial.print("announcing ");
        Serial.println(announcing?"true":"false");
        Serial.println("processReceivedPacket");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    while(transmiting){
        Serial.print("transmiting ");
        Serial.println(transmiting?"true":"false");
        Serial.println("processReceivedPacket");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    while(true){
        if(gotPacket){
            processing = true;
            if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
                Serial.println("Packet received");

                uint32_t size = radio.getPacketLength();
                Serial.print("Size ");
                Serial.println(size);
                if(size > 0 and size <= sizeof(lora_packet)){
                    radio.readData((uint8_t *)&p, size);
                    if(strcmp(p.id, user_id) != 0){
                        activity(lv_color_hex(0x00ff00));
                        gotPacket = false;
                        Serial.print("id ");
                        Serial.println(p.id);
                        Serial.print("msg ");
                        Serial.println(p.msg);
                        Serial.print("status ");
                        Serial.println(p.status);
                        Serial.print("me ");
                        Serial.println(p.me ? "true": "false");
                        Serial.print(F(" RSSI:"));
                        Serial.print(radio.getRSSI());
                        Serial.print(F(" dBm"));
                        Serial.print(F("  SNR:"));
                        Serial.print(radio.getSNR());
                        Serial.println(F(" dB"));

                        contact = contacts_list.getContactByID(p.id);
                    
                        if(contact != NULL){
                            Serial.println("Contact found");
                            if(strcmp(p.status, "send") == 0){
                                strftime(p.date_time, sizeof(p.date_time)," - %a, %b %d %Y %H:%M", &timeinfo);
                                strcpy(dec_msg, decrypt(p.id, p.msg).c_str());
                                strcpy(p.msg, dec_msg);
                                notify_snd();
                                messages_list.addMessage(p);
                                
                                strcpy(message, LV_SYMBOL_ENVELOPE);
                                strcat(message, " ");
                                strcat(message, contact->getName().c_str());
                                strcat(message, ": ");
                                if(sizeof(p.msg) > 199){
                                    memcpy(pmsg, dec_msg, 199);
                                    strcat(message, pmsg);
                                }
                                else
                                    strcat(message, p.msg);
                                message[30] = '.';
                                message[31] = '.';
                                message[32] = '.';
                                message[33] = '\0';
                                notification_list.add(message);

                                strcpy(c.id, user_id);
                                strcpy(c.status, "recv");
                                Serial.print("Sending confirmation...");
                                while(announcing){
                                    Serial.println("Awaiting announcement to finnish before send confirmation..");
                                    vTaskDelay(100 / portTICK_PERIOD_MS);
                                }
                                activity(lv_color_hex(0xff0000));
                                vTaskDelay(500 / portTICK_PERIOD_MS);
                                transmiting = true;
                                if(radio.transmit((uint8_t*)&c, sizeof(lora_packet_status)) == RADIOLIB_ERR_NONE)
                                    Serial.println("Confirmation sent");
                                else
                                    Serial.print("Confirmation not sent");
                                transmiting = false;
                            }

                            if(strcmp(p.status, "recv") == 0){
                                messages_list.addMessage(p);
                            }
                            
                            if(strcmp(p.status, "ping") == 0){
                                notification_list.add("ping");
                                Serial.print("Sending pong...");
                                strcpy(pong.id, user_id);
                                strcpy(pong.status, "pong");
                                while(announcing){
                                    Serial.println("Awaiting announcing to finnish before send confirmation..");
                                    vTaskDelay(100 / portTICK_PERIOD_MS);
                                }
                                activity(lv_color_hex(0xff0000));
                                if(radio.transmit((uint8_t*)&pong, sizeof(lora_packet_status)) == RADIOLIB_ERR_NONE)
                                    Serial.println("done");
                                else
                                    Serial.print("failed");
                            }
                            if(strcmp(p.status, "pong") == 0){
                                notify_snd();
                                Serial.println("pong");
                                strcpy(message, LV_SYMBOL_DOWNLOAD " ");
                                strcat(message, "pong ");
                                sprintf(pmsg, "RSSI %.2f", radio.getRSSI());
                                strcat(message, pmsg);
                                sprintf(pmsg, " S/N %.2f", radio.getSNR());
                                strcat(message, pmsg);
                                notification_list.add(message);
                            }
                            if(strcmp(p.status, "show") == 0){
                                contact->inrange = true;
                                //strcpy(message, LV_SYMBOL_CALL " ");
                                //strcat(message, contact->getName().c_str());
                                //strcat(message, " hi!");
                                //notification_list.add(message);
                                contact->timeout = millis();
                                Serial.println("Announcement packet");
                            }
                        }
                    }
                    else{
                        //lv_task_handler();
                        //notification_list.add(LV_SYMBOL_WARNING "Packet ignored");
                        //lv_task_handler();
                        Serial.println("Packet ignored");
                    }
                }else
                    Serial.println("Unknown or malformed packet");
                gotPacket = false;
                radio.startReceive();
                xSemaphoreGive(xSemaphore);
                processing = false;
            }
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

void onListen(){
    gotPacket = true;
}

void onTransmit(){
    transmiting = false;
}

bool normalMode(){
    if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
        radio.sleep(false);
        radio.reset();
        radio.begin();
        // set carrier frequency to 868.0 MHz
        if (radio.setFrequency(RADIO_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY) {
            Serial.println(F("Selected frequency is invalid for this module!"));
            return false;
        }

        // set bandwidth to 250 kHz
        if (radio.setBandwidth(250.0) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
            Serial.println(F("Selected bandwidth is invalid for this module!"));
            return false;
        }

        // set spreading factor to 10
        if (radio.setSpreadingFactor(10) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
            Serial.println(F("Selected spreading factor is invalid for this module!"));
            return false;
        }

        // set coding rate to 6
        if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE) {
            Serial.println(F("Selected coding rate is invalid for this module!"));
            return false;
        }

        // set LoRa sync word to 0xAB
        if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
            Serial.println(F("Unable to set sync word!"));
            return false;
        }

        // set output power to 10 dBm (accepted range is -17 - 22 dBm)
        if (radio.setOutputPower(17) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
            Serial.println(F("Selected output power is invalid for this module!"));
            return false;
        }

        // set over current protection limit to 140 mA (accepted range is 45 - 140 mA)
        // NOTE: set value to 0 to disable overcurrent protection
        if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
            Serial.println(F("Selected current limit is invalid for this module!"));
            return false;
        }

        // set LoRa preamble length to 15 symbols (accepted range is 0 - 65535)
        if (radio.setPreambleLength(15) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
            Serial.println(F("Selected preamble length is invalid for this module!"));
            return false;
        }

        // disable CRC
        if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
            Serial.println(F("Selected CRC is invalid for this module!"));
            return false;
        }
        xSemaphoreGive(xSemaphore);
    }
    return true;
}

bool DXMode()
{
    /*
    Distance - SF/BW
    10km - 7/500
    20km - 8/500
    30km - 9/500
    40km - 10/500
    60km - 11/500
    80km - 12/500
    100km - 11/250
    125km - 12/250
    */
    
    if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
        radio.sleep(false);
        radio.reset();
        radio.begin();

        // set carrier frequency to 868.0 MHz
        if (radio.setFrequency(RADIO_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY) {
            Serial.println(F("Selected frequency is invalid for this module!"));
            return false;
        }

        // set bandwidth to 250 kHz
        if (radio.setBandwidth(125.0) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
            Serial.println(F("Selected bandwidth is invalid for this module!"));
            return false;
        }

        // set spreading factor to 10
        if (radio.setSpreadingFactor(12) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
            Serial.println(F("Selected spreading factor is invalid for this module!"));
            return false;
        }

        // set coding rate to 6
        if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE) {
            Serial.println(F("Selected coding rate is invalid for this module!"));
            return false;
        }

        // set LoRa sync word to 0xAB
        if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
            Serial.println(F("Unable to set sync word!"));
            return false;
        }

        // set output power to 10 dBm (accepted range is -17 - 22 dBm)
        if (radio.setOutputPower(22) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
            Serial.println(F("Selected output power is invalid for this module!"));
            return false;
        }

        // set over current protection limit to 140 mA (accepted range is 45 - 140 mA)
        // NOTE: set value to 0 to disable overcurrent protection
        if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
            Serial.println(F("Selected current limit is invalid for this module!"));
            return false;
        }

        // set LoRa preamble length to 15 symbols (accepted range is 0 - 65535)
        if (radio.setPreambleLength(15) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
            Serial.println(F("Selected preamble length is invalid for this module!"));
            return false;
        }

        // disable CRC
        if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
            Serial.println(F("Selected CRC is invalid for this module!"));
            return false;
        }
        xSemaphoreGive(xSemaphore);
    }
    return true;
    
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
    int32_t code = 0;
    xTaskCreatePinnedToCore(processReceivedPacket, "proc_recv_pkt", 20000, NULL, 1, &task_recv_pkt, 1);
    radio.setPacketReceivedAction(onListen);
    //radio.setPacketSentAction(onTransmit);
    code = radio.startReceive();
    Serial.print("setup radio start receive code ");
    Serial.println(code);
    if(code == RADIOLIB_ERR_NONE){
        hasRadio = true;
    }
    //return true;

}

String encrypt(String msg){
    char k[19] = {'\0'};
    //Minimum cipher key must be 16 bytes long, or the default key will be used
    strcpy(k, user_id);
    strcat(k, user_id);
    strcat(k, user_id);
    cipher->setKey(k);
    return cipher->encryptString(msg);
}

String decrypt(char * contact_id, String msg){
    char k[19] = {'\0'};

    strcpy(k, contact_id);
    strcat(k, contact_id);
    strcat(k, contact_id);
    cipher->setKey(k);
    return cipher->decryptBuffer(msg);
}

void test(lv_event_t * e){
    lora_packet_status my_packet;
    //xTaskCreatePinnedToCore(song, "play_song", 10000, NULL, 2 | portPRIVILEGE_BIT, &task_play_radio, 0);
    strcpy(my_packet.id, user_id);
    strcpy(my_packet.status, "ping");
    if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
        if(hasRadio){
            while(transmiting){
                Serial.println("test");
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
            radio.startReceive();
            transmiting = true;
            int state = radio.startTransmit((uint8_t *)&my_packet, sizeof(lora_packet_status));
            transmiting = false;
            if(state != RADIOLIB_ERR_NONE){
                Serial.print("transmission failed ");
                Serial.println(state);
            }else{
                Serial.println("transmitted");
                lv_task_handler();
                notification_list.add(LV_SYMBOL_UPLOAD " ping sent");
                lv_task_handler();
            }
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

void hide_settings(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_settings != NULL){
            saveSettings();
            lv_obj_add_flag(frm_settings, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void show_settings(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    char year[5], month[3], day[3], hour[3], minute[3], color[7];

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_settings != NULL){
            loadSettings();
            itoa(timeinfo.tm_year + 1900, year, 10);
            itoa(timeinfo.tm_mon + 1, month, 10);
            itoa(timeinfo.tm_mday, day, 10);
            itoa(timeinfo.tm_hour, hour, 10);
            itoa(timeinfo.tm_min, minute, 10);
            lv_textarea_set_text(frm_settings_year, year);
            lv_textarea_set_text(frm_settings_month, month);
            lv_textarea_set_text(frm_settings_day, day);
            lv_textarea_set_text(frm_settings_hour, hour);
            lv_textarea_set_text(frm_settings_minute, minute);
            lv_textarea_set_text(frm_settings_name, user_name);
            lv_textarea_set_text(frm_settings_id, user_id);
            sprintf(color, "%lX", ui_primary_color);
            lv_textarea_set_text(frm_settings_color, color);

            lv_obj_clear_flag(frm_settings, LV_OBJ_FLAG_HIDDEN);
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
            if(task_check_new_msg != NULL){
                vTaskDelete(task_check_new_msg);
                task_check_new_msg = NULL;
                Serial.println("task_check_new_msg finished");
            }
            if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
                lv_obj_clean(frm_chat_list);
                xSemaphoreGive(xSemaphore);
            }
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
    lv_obj_t * lbl_id = lv_obj_get_child(btn, 2);
    const char * name = lv_label_get_text(lbl);
    const char * id = lv_label_get_text(lbl_id);

    actual_contact = contacts_list.getContactByID(id);

    char title[31] = "Chat with ";
    strcat(title, name);
    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_chat != NULL){
            lv_obj_clear_flag(frm_chat, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(frm_chat_btn_title_lbl, title);
            lv_group_focus_obj(frm_chat_text_ans);
            if(task_check_new_msg == NULL){
                xTaskCreatePinnedToCore(check_new_msg, "check_new_msg", 11000, NULL, 1, &task_check_new_msg, 1);
                Serial.println("task_check_new_msg running");
                Serial.print("actual contact is ");
                Serial.println(actual_contact->getName());
            }
        }
    }
}

void send_message(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    String enc_msg;
    char msg[200] = {'\0'};
    activity(lv_color_hex(0xff0000));
    if(code == LV_EVENT_SHORT_CLICKED){
        lora_packet pkt;
        strcpy(pkt.id, user_id);
        strcpy(pkt.status, "send");
        strftime(pkt.date_time, sizeof(pkt.date_time)," - %a, %b %d %Y %H:%M", &timeinfo);
        
        if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
            if(hasRadio){
                if(strcmp(lv_textarea_get_text(frm_chat_text_ans), "") != 0){
                    strcpy(msg, lv_textarea_get_text(frm_chat_text_ans));
                    enc_msg = encrypt(lv_textarea_get_text(frm_chat_text_ans));
                    strcpy(pkt.msg, enc_msg.c_str());
                    while(announcing){
                        Serial.println("announcing");
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                    }
                    while(gotPacket){
                        Serial.println("gotPacket");
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                    }
                    activity(lv_color_hex(0xff0000));
                    transmiting = true;
                    int state = radio.startTransmit((uint8_t *)&pkt, sizeof(lora_packet));
                    transmiting = false;

                    if(state != RADIOLIB_ERR_NONE){
                        Serial.print("transmission failed ");
                        Serial.println(state);
                    }else{
                        Serial.println("transmitted");
                        // add the message to the list of messages
                        pkt.me = true;
                        strcpy(pkt.id, actual_contact->getID().c_str());
                        strcpy(pkt.msg, msg);
                        Serial.print("Adding answer to ");
                        Serial.println(pkt.id);
                        Serial.println(pkt.msg);
                        messages_list.addMessage(pkt);
                        lv_textarea_set_text(frm_chat_text_ans, "");
                    }
                }
            }
            xSemaphoreGive(xSemaphore);
        }
    }
}

void copy_text(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * lbl = (lv_obj_t *)lv_event_get_user_data(e);
    if(code == LV_EVENT_LONG_PRESSED){
        lv_textarea_add_text(frm_chat_text_ans, lv_label_get_text(lbl));
        lv_task_handler();
    }
}

void check_new_msg(void * param){
    vector<lora_packet> caller_msg;
    uint32_t actual_count = 0;
    lv_obj_t * btn = NULL, * lbl = NULL;
    char date[30] = {'\0'};
    char name[100] = {'\0'};
    
    while(true){
        caller_msg = messages_list.getMessages(actual_contact->getID().c_str());
        actual_count = caller_msg.size();
        if(actual_count > msg_count){
            Serial.println("new messages");
            for(uint32_t i = msg_count; i < actual_count; i++){
                if(caller_msg[i].me){
                    strcpy(name, "Me");
                    strcat(name, caller_msg[i].date_time);
                    Serial.println(name);
                    lv_list_add_text(frm_chat_list, name);
                }else{
                    if(strcmp(caller_msg[i].status, "recv") == 0)
                        btn = lv_list_add_btn(frm_chat_list, LV_SYMBOL_OK, "");
                    else{
                        strcpy(name, actual_contact->getName().c_str());
                        strcat(name, caller_msg[i].date_time);
                        lv_list_add_text(frm_chat_list, name);
                        Serial.println(name);
                    }
                }
                Serial.println(caller_msg[i].msg);
                if(strcmp(caller_msg[i].status, "recv") != 0){
                    btn = lv_list_add_btn(frm_chat_list, NULL, caller_msg[i].msg);
                    lv_obj_add_event_cb(btn, copy_text, LV_EVENT_LONG_PRESSED, lv_obj_get_child(btn, 0));
                }
                lbl = lv_obj_get_child(btn, 0);
                if(strcmp(caller_msg[i].status, "recv") != 0)
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
                Serial.println("Contact already exists");
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

void generateID(lv_event_t * e){
    lv_textarea_set_text(frm_settings_id, generate_ID().c_str());
}

void DX(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_VALUE_CHANGED){
        if(lv_obj_has_state(frm_settings_switch_dx, LV_STATE_CHECKED)){
            if(DXMode()){
                Serial.println("DX mode on");
                notification_list.add(LV_SYMBOL_SETTINGS " DX mode");
            }else{
                notification_list.add(LV_SYMBOL_SETTINGS " DX mode failed");
                Serial.println("DX mode failed");
            }
        }else{
            if(normalMode()){
                notification_list.add(LV_SYMBOL_SETTINGS " Normal mode");
                Serial.println("DX mode off");
            }else{
                notification_list.add(LV_SYMBOL_SETTINGS " Normal mode failed");
                Serial.println("Normal mode failed");
            }
        }
    }
}

void update_time(void *timeStruct) {
    char hourMin[6];
    char date[12];
    while(true){
        if(getLocalTime((struct tm*)timeStruct)){
            
            strftime(hourMin, 6, "%H:%M %p", (struct tm *)timeStruct);
            lv_label_set_text(frm_home_time_lbl, hourMin);

            strftime(date, 12, "%a, %b %d", (struct tm *)timeStruct);
            lv_label_set_text(frm_home_date_lbl, date);
        }
        vTaskDelay(60000 / portTICK_RATE_MS);
        announce();
    }
    vTaskDelete(task_date_time);
}

void setDate(int yr, int month, int mday, int hr, int minute, int sec, int isDst){
    timeinfo.tm_year = yr - 1900;   // Set date
    timeinfo.tm_mon = month-1;
    timeinfo.tm_mday = mday;
    timeinfo.tm_hour = hr;      // Set time
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = sec;
    timeinfo.tm_isdst = isDst;  // 1 or 0
    time_t t = mktime(&timeinfo);
    Serial.printf("Setting time: %s", asctime(&timeinfo));
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, NULL);
    notification_list.add(LV_SYMBOL_SETTINGS " date & time updated");
}

void setDateTime(){
    char day[3] = {'\0'}, month[3] = {'\0'}, year[5] = {'\0'}, hour[3] = {'\0'}, minute[3] = {'\0'};

    strcpy(day, lv_textarea_get_text(frm_settings_day));
    strcpy(month, lv_textarea_get_text(frm_settings_month));
    strcpy(year, lv_textarea_get_text(frm_settings_year));
    strcpy(hour, lv_textarea_get_text(frm_settings_hour));
    strcpy(minute, lv_textarea_get_text(frm_settings_minute));

    if(strcmp(day, "") != 0 && strcmp(month, "") != 0 && strcmp(year, "") != 0 && strcmp(hour, "") != 0 && strcmp(minute, "") != 0){
        try{
            setDate(atoi(year), atoi(month), atoi(day), atoi(hour), atoi(minute), 0, 0);
        }
        catch(exception &ex){
            Serial.println(ex.what());
        }
    }
}

void applyDate(lv_event_t * e){
    char hourMin[6];
    char date[12];

    setDateTime();
    strftime(hourMin, 6, "%H:%M %p", (struct tm *)&timeinfo);
    lv_label_set_text(frm_home_time_lbl, hourMin);

    strftime(date, 12, "%a, %b %d", (struct tm *)&timeinfo);
    lv_label_set_text(frm_home_date_lbl, date);
}

void apply_color(lv_event_t * e){
    char color[7] = "";

    strcpy(color, lv_textarea_get_text(frm_settings_color));
    if(strcmp(color, "") == 0)
        return;
    ui_primary_color = strtoul(color, NULL, 16);
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_color_hex(ui_primary_color), lv_palette_main(LV_PALETTE_RED), false, &lv_font_montserrat_14);
    lv_disp_set_theme(dispp, theme);
}

void update_frm_contacts_status(uint16_t index, bool in_range){
    if(index > contacts_list.size() - 1 || index < 0)
        return;
    lv_obj_t * btn = lv_obj_get_child(frm_contacts_list, index);
    lv_obj_t * obj_status = lv_obj_get_child(btn, 3);
    lv_task_handler();
    if(in_range)
        lv_obj_set_style_bg_color(obj_status, lv_color_hex(0x00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
    else
        lv_obj_set_style_bg_color(obj_status, lv_color_hex(0xaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_task_handler();
}

void check_contacts_in_range(){
    activity(lv_color_hex(0xff66ff));
    contacts_list.check_inrange();
    for(uint32_t i = 0; i < contacts_list.size(); i++){
        update_frm_contacts_status(i, contacts_list.getList()[i].inrange);

        Serial.print(contacts_list.getList()[i].getName());
        Serial.println(contacts_list.getList()[i].inrange ? " is in range" : " is out of range");
    }
}

void initBat(){
    esp_adc_cal_characteristics_t adc_bat;
    esp_adc_cal_value_t type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_bat);

    if (type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        vRef = adc_bat.vref;
    } else {
        vRef = 1100;
    }
}

uint32_t read_bat(){
    uint16_t v = analogRead(BOARD_BAT_ADC);
    double vBat = ((double)v / 4095.0) * 2.0 * 3.3 * (vRef / 1000.0);
    if (vBat > 4.2) {
        vBat = 4.2;
    }
    uint32_t batPercent = map(vBat * 100, 320, 420, 0, 100);
    if (batPercent < 0) {
        batPercent = 0;
    }

    return batPercent;
}

char * get_battery_icon(uint32_t percentage) {
  if (percentage >= 90) {
    return LV_SYMBOL_BATTERY_FULL;
  } else if (percentage >= 65 && percentage < 90) {
    return LV_SYMBOL_BATTERY_3;
  } else if (percentage >= 40 && percentage < 65) {
    return LV_SYMBOL_BATTERY_2;
  } else if (percentage >= 15 && percentage < 40) {
    return LV_SYMBOL_BATTERY_1;
  } else {
    return LV_SYMBOL_BATTERY_EMPTY;
  }
}

void update_bat(void * param){
    char icon[12] = {'\0'};
    uint32_t p = read_bat();
    char pc[4] = {'\0'};
    char msg[30]= {'\0'};
    while(true){
        itoa(p, pc, 10);
        strcpy(msg, pc);
        strcat(msg, "% ");
        strcat(msg, get_battery_icon(p));
        lv_label_set_text(frm_home_bat_lbl, msg);
        check_contacts_in_range();
        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}

void hide_wifi(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_wifi != NULL){
            lv_obj_add_flag(frm_wifi, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void show_wifi(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_wifi != NULL){
            lv_obj_clear_flag(frm_wifi, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void wifi_apply(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    int i = (int)lv_event_get_user_data(e);
    uint32_t count = 0;
    char msg[100] = {'\0'};

    if(code == LV_EVENT_SHORT_CLICKED){
        if(wifi_list[i].auth_type == WIFI_AUTH_WPA2_PSK || 
           wifi_list[i].auth_type == WIFI_AUTH_WPA_WPA2_PSK || 
           wifi_list[i].auth_type == WIFI_AUTH_WEP){
            strcpy(wifi_list[i].pass, lv_textarea_get_text(frm_wifi_simple_ta_pass));
            if(strcmp(wifi_list[i].pass, "") == 0){
                Serial.println("Provide a password");
                return;
            }
            lv_label_set_text(frm_wifi_simple_title_lbl, "Connecting...");
            WiFi.disconnect(true);
            WiFi.mode(WIFI_STA);
            WiFi.begin(wifi_list[i].SSID, wifi_list[i].pass);
            while(!WiFi.isConnected()){
                Serial.print(".");
                count++;
                if(count == 30){
                    WiFi.disconnect();
                    break;
                }
                delay(1000);
            }
        }else if(wifi_list[i].auth_type == WIFI_AUTH_WPA2_ENTERPRISE){
            strcpy(wifi_list[i].pass, lv_textarea_get_text(frm_wifi_login_ta_pass));
            strcpy(wifi_list[i].login, lv_textarea_get_text(frm_wifi_login_ta_login));
            if(strcmp(wifi_list[i].pass, "") == 0 || strcmp(wifi_list[i].login, "") == 0){
                Serial.println("Provide a login and passwod");
                return;
            }
            lv_task_handler();
            lv_label_set_text(frm_wifi_login_title_lbl, "Connecting...");
            lv_task_handler();
            WiFi.disconnect(true);
            WiFi.mode(WIFI_STA);
            
            esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)wifi_list[i].login, strlen(wifi_list[i].login));
            esp_wifi_sta_wpa2_ent_set_username((uint8_t*)wifi_list[i].login, strlen(wifi_list[i].login));
            esp_wifi_sta_wpa2_ent_set_password((uint8_t*)wifi_list[i].pass, strlen(wifi_list[i].pass));
            esp_wifi_sta_wpa2_ent_enable();
            
            WiFi.begin(wifi_list[i].SSID, wifi_list[i].pass);

            while(!WiFi.isConnected()){
                Serial.print(".");
                count++;
                if(count == 30){
                    WiFi.disconnect();
                    break;
                }
                delay(1000);
            }
        }else if(wifi_list[i].auth_type == WIFI_AUTH_OPEN){
            WiFi.disconnect();
            WiFi.mode(WIFI_STA);
            WiFi.begin(wifi_list[i].SSID);
            while(!WiFi.isConnected()){
                Serial.print(".");
                count++;
                if(count == 30){
                    WiFi.disconnect();
                    break;
                }
                delay(1000);
            }
        }else
            Serial.println("auth type not implemented");
        
        if(WiFi.isConnected()){
            last_wifi_con = i;
            wifi_connected_nets.add(wifi_list[i]);
            wifi_connected_nets.save();
            Serial.println("wifi network saved");
            lv_obj_set_style_text_color(frm_settings_btn_wifi_lbl, lv_color_hex(0x00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
            Serial.println(WiFi.localIP().toString());
            strcat(connected_to, wifi_list[i].SSID);
            strcat(connected_to, " ");
            strcat(connected_to, WiFi.localIP().toString().c_str());
            lv_obj_add_flag(frm_wifi_login, LV_OBJ_FLAG_HIDDEN); 
            lv_obj_add_flag(frm_wifi_simple, LV_OBJ_FLAG_HIDDEN);
            datetime();
        }else{
            Serial.println("Connection failed");
            lv_label_set_text(frm_wifi_connected_to_lbl, "Auth failed");
        }
    }
}

void forget_net(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    uint i = (int)lv_event_get_user_data(e);

    if(code == LV_EVENT_LONG_PRESSED){
        if(wifi_connected_nets.del(wifi_list[i].SSID)){
            lv_label_set_text(frm_wifi_connected_to_lbl, "forgoten");
            wifi_connected_nets.save();
            WiFi.disconnect(true);
            lv_obj_set_style_text_color(frm_settings_btn_wifi_lbl, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

void wifi_select(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    uint i = (int)lv_event_get_user_data(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        Serial.print("Connecting to ");
        Serial.println(wifi_list[i].SSID);
        Serial.println(wifi_list[i].RSSI);
        Serial.println(wifi_list[i].auth_type);

        switch(wifi_list[i].auth_type){
            case WIFI_AUTH_WPA2_PSK:
                lv_obj_remove_event_cb(frm_wifi_simple_btn_connect, wifi_apply);
                lv_obj_add_event_cb(frm_wifi_simple_btn_connect, wifi_apply, LV_EVENT_SHORT_CLICKED, (void *)i);
                lv_obj_clear_flag(frm_wifi_simple, LV_OBJ_FLAG_HIDDEN);
                break;
            case WIFI_AUTH_WPA_WPA2_PSK:
                lv_obj_remove_event_cb(frm_wifi_simple_btn_connect, wifi_apply);
                lv_obj_add_event_cb(frm_wifi_simple_btn_connect, wifi_apply, LV_EVENT_SHORT_CLICKED, (void *)i);
                lv_obj_clear_flag(frm_wifi_simple, LV_OBJ_FLAG_HIDDEN);
                break;
            case WIFI_AUTH_WPA2_ENTERPRISE:
                lv_obj_remove_event_cb(frm_wifi_login_btn_connect, wifi_apply);
                lv_obj_add_event_cb(frm_wifi_login_btn_connect, wifi_apply, LV_EVENT_SHORT_CLICKED, (void *)i);
                lv_obj_clear_flag(frm_wifi_login, LV_OBJ_FLAG_HIDDEN);
                break;
            case WIFI_AUTH_OPEN:
                wifi_apply(e);
                break;
        }
    }if(code == LV_EVENT_LONG_PRESSED){
        forget_net(e);
    }
}

void wifi_scan_task(void * param){
    int n = 0;
    wifi_info wi;
    char ssid[100] = {'\0'};
    char rssi[5] = {'\0'};
    char ch[3] = {'\0'};
    lv_obj_t * btn = NULL;
    Serial.print("scanning...");
    lv_task_handler();
    lv_label_set_text(frm_wifi_connected_to_lbl, "Scanning...");
    lv_task_handler();
    //WiFi.disconnect(true);
    n = WiFi.scanNetworks();
    if(n > 0){
        lv_obj_clean(frm_wifi_list);
        wifi_list.clear();
        for(uint i = 0; i < n; i++){
            wi.auth_type = WiFi.encryptionType(i);
            wi.RSSI = WiFi.RSSI(i);
            wi.ch = WiFi.channel();
            strcpy(wi.SSID, WiFi.SSID(i).c_str());
            wifi_list.push_back(wi);
            strcpy(ssid, WiFi.SSID(i).c_str());
            strcat(ssid, " rssi:");
            itoa(WiFi.RSSI(i), rssi, 10);
            strcat(ssid, rssi);
            strcat(ssid, "\n");
            strcat(ssid, wifi_auth_mode_to_str(WiFi.encryptionType(i)));
            strcat(ssid, " ch:");
            itoa(WiFi.channel(i), ch, 10);
            strcat(ssid, ch);
            btn = lv_list_add_btn(frm_wifi_list, LV_SYMBOL_WIFI, ssid);
            lv_obj_add_event_cb(btn, wifi_select, LV_EVENT_SHORT_CLICKED, (void *)i);
            lv_obj_add_event_cb(btn, wifi_select, LV_EVENT_LONG_PRESSED, (void *)i);

        }
    }
    Serial.println("done");
    lv_label_set_text(frm_wifi_connected_to_lbl, "Scan complete");
    vTaskDelete(NULL);
}

void wifi_scan(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        xTaskCreatePinnedToCore(wifi_scan_task, "wifi_scan_task", 10000, NULL, 1, NULL, 1);
    }
}

void update_wifi_icon(){
    if(WiFi.isConnected()){
        lv_label_set_text(frm_home_wifi_lbl, LV_SYMBOL_WIFI);
        lv_label_set_text(frm_wifi_connected_to_lbl, connected_to);
    }
    else{
        lv_label_set_text(frm_home_wifi_lbl, "");
        lv_label_set_text(frm_wifi_connected_to_lbl, "");
    }
}

void show_pass(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    switch(code){
        case LV_EVENT_PRESSED:
            lv_textarea_set_password_mode(frm_wifi_simple_ta_pass, false);
            break;
        case LV_EVENT_RELEASED:
            lv_textarea_set_password_mode(frm_wifi_simple_ta_pass, true);
            break;
    }
}

void show_login_pass(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    switch(code){
        case LV_EVENT_PRESSED:
            lv_textarea_set_password_mode(frm_wifi_login_ta_pass, false);
            break;
        case LV_EVENT_RELEASED:
            lv_textarea_set_password_mode(frm_wifi_login_ta_pass, true);
            break;
    }
}

void show_simple(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_wifi_simple != NULL){
            lv_obj_clear_flag(frm_wifi_simple, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void hide_simple(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_wifi_simple != NULL){
            lv_obj_add_flag(frm_wifi_simple, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void show_wifi_login(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_wifi_login != NULL){
            lv_obj_clear_flag(frm_wifi_login, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void hide_wifi_login(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_wifi_login != NULL){
            lv_obj_add_flag(frm_wifi_login, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void wifi_toggle(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    uint c = 0;

    if(code == LV_EVENT_SHORT_CLICKED){
        if(wifi_connected_nets.list.size() == 0)
            return;
        if(!WiFi.isConnected()){
            wifi_auto_toggle();
        }
        else{
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            lv_obj_set_style_text_color(frm_settings_btn_wifi_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            Serial.println("wifi off");
        }
    }
}

bool setupSD()
{
    digitalWrite(BOARD_SDCARD_CS, HIGH);
    digitalWrite(RADIO_CS_PIN, HIGH);
    digitalWrite(BOARD_TFT_CS, HIGH);

    if (SD.begin(BOARD_SDCARD_CS, SPI, 800000U)) {
        uint8_t cardType = SD.cardType();
        if (cardType == CARD_NONE) {
            Serial.println("No SD_MMC card attached");
            return false;
        } else {
            Serial.print("SD_MMC Card Type: ");
            if (cardType == CARD_MMC) {
                Serial.println("MMC");
            } else if (cardType == CARD_SD) {
                Serial.println("SDSC");
            } else if (cardType == CARD_SDHC) {
                Serial.println("SDHC");
            } else {
                Serial.println("UNKNOWN");
            }
            uint32_t cardSize = SD.cardSize() / (1024 * 1024);
            uint32_t cardTotal = SD.totalBytes() / (1024 * 1024);
            uint32_t cardUsed = SD.usedBytes() / (1024 * 1024);
            Serial.printf("SD Card Size: %lu MB\n", cardSize);
            Serial.printf("Total space: %lu MB\n",  cardTotal);
            Serial.printf("Used space: %lu MB\n",   cardUsed);
            return true;
        }
    }
    return false;
}

bool setupCoder() {
    uint32_t ret_val = ESP_OK;

    Wire.beginTransmission(ES7210_ADDR);
    uint8_t error = Wire.endTransmission();
    if (error != 0) {
        Serial.println("ES7210 address not found");
        return false;
    }

    audio_hal_codec_config_t cfg = {
        .adc_input = AUDIO_HAL_ADC_INPUT_ALL,
        .codec_mode = AUDIO_HAL_CODEC_MODE_ENCODE,
        .i2s_iface = {
        .mode = AUDIO_HAL_MODE_SLAVE,
        .fmt = AUDIO_HAL_I2S_NORMAL,
        .samples = AUDIO_HAL_16K_SAMPLES,
        .bits = AUDIO_HAL_BIT_LENGTH_16BITS,
        },
    };

    ret_val |= es7210_adc_init(&Wire, &cfg);
    ret_val |= es7210_adc_config_i2s(cfg.codec_mode, &cfg.i2s_iface);
    ret_val |= es7210_adc_set_gain(
        (es7210_input_mics_t)(ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2),
        (es7210_gain_value_t)GAIN_0DB);
    ret_val |= es7210_adc_set_gain(
        (es7210_input_mics_t)(ES7210_INPUT_MIC3 | ES7210_INPUT_MIC4),
        (es7210_gain_value_t)GAIN_37_5DB);
    ret_val |= es7210_adc_ctrl_state(cfg.codec_mode, AUDIO_HAL_CTRL_START);
    return ret_val == ESP_OK;
}

void song(void * param) {
    //BaseType_t xHigherPriorityTaskWoken;
    if(WiFi.isConnected()){
        //if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
            digitalWrite(BOARD_POWERON, HIGH);
            audio.setPinout(BOARD_I2S_BCK, BOARD_I2S_WS, BOARD_I2S_DOUT);
            audio.setVolume(10);
            audio.connecttohost("0n-80s.radionetz.de:8000/0n-70s.mp3");
            //audio.connecttospeech("Notificação enviada com sucesso.", "pt");
            while (audio.isRunning()) {
                audio.loop();
            }
            audio.stopSong();
        //    xSemaphoreGive(xSemaphore);
        //}
    }
    vTaskDelete(task_play_radio);
}

void taskplaySong(void *p) {
    //if (xSemaphoreTake(xSemaphore, 10000 / portTICK_PERIOD_MS) == pdTRUE) {
        if (SD.exists("/comp_up.mp3")) {
            const char *path = "comp_up.mp3";
            digitalWrite(BOARD_POWERON, HIGH);
            audio.setPinout(BOARD_I2S_BCK, BOARD_I2S_WS, BOARD_I2S_DOUT);
            audio.setVolume(21);
            audio.connecttoFS(SD, path);
            Serial.printf("play %s\r\n", path);
            while (audio.isRunning()) {
                audio.loop();
            }
            audio.stopSong();
        }
        //xSemaphoreGive(xSemaphore);
    //}
    vTaskDelete(task_play);
}

void notify_snd(){
    xTaskCreatePinnedToCore(taskplaySong, "play_not_snd", 10000, NULL, 2, &task_play, 0);
}

/*
static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    if (slider == ui_brightnessSlider)
    {
        brightness = (int)lv_slider_get_value(slider);
    }

    if (slider == ui_musicVolume)
    {
        int vol = (int)lv_slider_get_value(slider);
        setVolume(vol);
    }
}*/

void activity(lv_color_t color){
    lv_obj_set_style_bg_color(frm_home_activity_led, color, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui(){
    //style**************************************************************
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_color_hex(ui_primary_color), lv_palette_main(LV_PALETTE_RED), false, &lv_font_montserrat_14);
    lv_disp_set_theme(dispp, theme);

    // Home screen**************************************************************
    frm_home = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_home, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(frm_home, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_color(frm_home, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(frm_home, 0, 0 ) ;
    lv_obj_set_style_border_width(frm_home, 0, 0);

    //background image
    lv_obj_set_style_bg_img_src(frm_home, &bg2, LV_PART_MAIN | LV_STATE_DEFAULT);

    // title bar
    /*frm_home_title = lv_btn_create(frm_home);
    lv_obj_set_size(frm_home_title, 310, 20);
    lv_obj_align(frm_home_title, LV_ALIGN_TOP_MID, 0, -10);*/

    frm_home_title_lbl = lv_label_create(frm_home);
    lv_obj_set_style_text_color(frm_home_title_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(frm_home_title_lbl, LV_ALIGN_TOP_LEFT, -5, -10);
    lv_obj_set_size(frm_home_title_lbl, 200, 30);
    lv_label_set_long_mode(frm_home_title_lbl, LV_LABEL_LONG_SCROLL);

    // battery icon
    frm_home_bat_lbl = lv_label_create(frm_home);
    lv_label_set_text(frm_home_bat_lbl, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(frm_home_bat_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(frm_home_bat_lbl, LV_ALIGN_TOP_RIGHT, 5, -10);

    //Wifi icon
    frm_home_wifi_lbl = lv_label_create(frm_home);
    lv_label_set_text(frm_home_wifi_lbl, "");
    lv_obj_set_style_text_color(frm_home_wifi_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(frm_home_wifi_lbl, LV_ALIGN_TOP_RIGHT, -55, -10);

    //date time background
    frm_home_frm_date_time = lv_obj_create(frm_home);
    lv_obj_set_size(frm_home_frm_date_time, 90, 55);
    lv_obj_align(frm_home_frm_date_time, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_clear_flag(frm_home_frm_date_time, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(frm_home_frm_date_time, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(frm_home_frm_date_time, 64, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(frm_home_frm_date_time, lv_color_hex(0x151515), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(frm_home_frm_date_time, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    //date label
    frm_home_date_lbl = lv_label_create(frm_home_frm_date_time);
    lv_obj_set_style_text_color(frm_home_date_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(frm_home_date_lbl, "-");
    lv_obj_align(frm_home_date_lbl, LV_ALIGN_TOP_MID, 0, -10);

    //time label
    frm_home_time_lbl = lv_label_create(frm_home_frm_date_time);
    lv_obj_set_style_text_font(frm_home_time_lbl, &clocknum, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(frm_home_time_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(frm_home_time_lbl, "00:00");
    lv_obj_align(frm_home_time_lbl, LV_ALIGN_TOP_MID, 0, 10);

    // Contacts button
    frm_home_btn_contacts = lv_btn_create(frm_home);
    lv_obj_set_size(frm_home_btn_contacts, 50, 35);
    lv_obj_set_align(frm_home_btn_contacts, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_add_event_cb(frm_home_btn_contacts, show_contacts_form, LV_EVENT_SHORT_CLICKED, NULL);
    /*
    frm_home_btn_contacts_lbl = lv_label_create(frm_home_btn_contacts);
    lv_label_set_text(frm_home_btn_contacts_lbl, LV_SYMBOL_CALL);
    lv_obj_align(frm_home_btn_contacts_lbl, LV_ALIGN_CENTER, 0, 0);
    
    */
    //lora icon
    frm_home_contacts_img = lv_img_create(frm_home_btn_contacts);
    lv_img_set_src(frm_home_contacts_img, &icon_lora2);
    lv_obj_set_align(frm_home_contacts_img, LV_ALIGN_CENTER);
    
    // Settings button
    frm_home_btn_settings = lv_btn_create(frm_home);
    lv_obj_set_size(frm_home_btn_settings, 20, 20);
    lv_obj_align(frm_home_btn_settings, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(frm_home_btn_settings, show_settings, LV_EVENT_SHORT_CLICKED, NULL);

    frm_home_btn_settings_lbl = lv_label_create(frm_home_btn_settings);
    lv_label_set_text(frm_home_btn_settings_lbl, LV_SYMBOL_SETTINGS);
    lv_obj_set_align(frm_home_btn_settings_lbl, LV_ALIGN_CENTER);

    // Test button
    btn_test = lv_btn_create(frm_home);
    lv_obj_set_size(btn_test, 50, 20);
    lv_obj_set_align(btn_test, LV_ALIGN_BOTTOM_RIGHT);
    
    lbl_btn_test = lv_label_create(btn_test);
    lv_obj_set_style_text_font(lbl_btn_test, &ubuntu, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(lbl_btn_test, "Ping");
    lv_obj_align(lbl_btn_test, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_test, test, LV_EVENT_SHORT_CLICKED, NULL);

    // Activity led 
    frm_home_activity_led = lv_obj_create(frm_home);
    lv_obj_set_size(frm_home_activity_led, 10, 15);
    lv_obj_align(frm_home_activity_led, LV_ALIGN_TOP_RIGHT, -75, -10);
    lv_obj_clear_flag(frm_home_activity_led, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(frm_home_activity_led, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(frm_home_activity_led, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT);

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

    //group the objects, the answer area will be almost on focus every time we send a msg
    frm_chat_group = lv_group_create();
    lv_group_add_obj(frm_chat_group, frm_chat_btn_back);
    lv_group_add_obj(frm_chat_group, frm_chat_btn_send);
    lv_group_add_obj(frm_chat_group, frm_chat_btn_back_lbl);
    lv_group_add_obj(frm_chat_group, frm_chat_btn_send_lbl);
    lv_group_add_obj(frm_chat_group, frm_chat_btn_title);
    lv_group_add_obj(frm_chat_group, frm_chat_btn_title_lbl);
    lv_group_add_obj(frm_chat_group, frm_chat_list);

    lv_obj_add_flag(frm_chat, LV_OBJ_FLAG_HIDDEN);

    // Settings form**************************************************************
    frm_settings = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_settings, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(frm_settings, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    frm_settings_btn_title = lv_btn_create(frm_settings);
    lv_obj_set_size(frm_settings_btn_title, 200, 20);
    lv_obj_align(frm_settings_btn_title, LV_ALIGN_TOP_LEFT, -15, -15);

    frm_settings_btn_title_lbl = lv_label_create(frm_settings_btn_title);
    lv_label_set_text(frm_settings_btn_title_lbl, "Settings");
    lv_obj_set_align(frm_settings_btn_title_lbl, LV_ALIGN_LEFT_MID);

    // back button
    frm_settings_btn_back = lv_btn_create(frm_settings);
    lv_obj_set_size(frm_settings_btn_back, 50, 20);
    lv_obj_align(frm_settings_btn_back, LV_ALIGN_TOP_RIGHT, 15, -15);
    lv_obj_add_event_cb(frm_settings_btn_back, hide_settings, LV_EVENT_SHORT_CLICKED, NULL);

    frm_settings_btn_back_lbl = lv_label_create(frm_settings_btn_back);
    lv_label_set_text(frm_settings_btn_back_lbl, "Back");
    lv_obj_set_align(frm_settings_btn_back_lbl, LV_ALIGN_CENTER);

    //base form
    frm_settings_dialog = lv_obj_create(frm_settings);
    lv_obj_set_size(frm_settings_dialog, LV_HOR_RES, 220);
    lv_obj_align(frm_settings_dialog, LV_ALIGN_TOP_MID, 0, 10);

    // Name
    frm_settings_name = lv_textarea_create(frm_settings_dialog);
    lv_textarea_set_one_line(frm_settings_name, true);
    lv_obj_set_size(frm_settings_name, 280, 30);
    lv_textarea_set_placeholder_text(frm_settings_name, "Name");
    lv_obj_align(frm_settings_name, LV_ALIGN_OUT_TOP_LEFT, 0, -10);

    // ID
    frm_settings_id = lv_textarea_create(frm_settings_dialog);
    lv_textarea_set_one_line(frm_settings_id, true);
    lv_obj_set_size(frm_settings_id, 90, 30);
    lv_textarea_set_placeholder_text(frm_settings_id, "ID");
    lv_obj_align(frm_settings_id, LV_ALIGN_TOP_LEFT, 0, 20);

    //Generate button
    frm_settings_btn_generate = lv_btn_create(frm_settings_dialog);
    lv_obj_set_size(frm_settings_btn_generate, 80, 20);
    lv_obj_align(frm_settings_btn_generate, LV_ALIGN_TOP_LEFT, 100, 25);
    lv_textarea_set_max_length(frm_settings_id, 6);
    lv_obj_add_event_cb(frm_settings_btn_generate, generateID, LV_EVENT_SHORT_CLICKED, NULL);

    frm_settings_btn_generate_lbl = lv_label_create(frm_settings_btn_generate);
    lv_label_set_text(frm_settings_btn_generate_lbl, "Generate");
    lv_obj_set_align(frm_settings_btn_generate_lbl, LV_ALIGN_CENTER);

    // dx switch
    frm_settings_switch_dx = lv_switch_create(frm_settings_dialog);
    lv_obj_align(frm_settings_switch_dx, LV_ALIGN_OUT_TOP_LEFT, 30, 55);
    lv_obj_add_event_cb(frm_settings_switch_dx, DX, LV_EVENT_VALUE_CHANGED, NULL);

    frm_settings_switch_dx_lbl = lv_label_create(frm_settings_dialog);
    lv_label_set_text(frm_settings_switch_dx_lbl, "DX");
    lv_obj_align(frm_settings_switch_dx_lbl, LV_ALIGN_TOP_LEFT, 0, 60);

    //wifi toggle button
    frm_settings_btn_wifi = lv_btn_create(frm_settings_dialog);
    lv_obj_set_size(frm_settings_btn_wifi, 30, 30);
    lv_obj_align(frm_settings_btn_wifi, LV_ALIGN_TOP_LEFT, 90, 55);
    lv_obj_add_event_cb(frm_settings_btn_wifi, wifi_toggle, LV_EVENT_SHORT_CLICKED, NULL);

    frm_settings_btn_wifi_lbl = lv_label_create(frm_settings_btn_wifi);
    lv_label_set_text(frm_settings_btn_wifi_lbl, LV_SYMBOL_WIFI);
    lv_obj_set_align(frm_settings_btn_wifi_lbl, LV_ALIGN_CENTER);

    // wifi config button
    frm_settings_btn_wifi_conf = lv_btn_create(frm_settings_dialog);
    lv_obj_set_size(frm_settings_btn_wifi_conf, 100, 30);
    lv_obj_align(frm_settings_btn_wifi_conf, LV_ALIGN_TOP_LEFT, 130, 55);
    lv_obj_add_event_cb(frm_settings_btn_wifi_conf, show_wifi, LV_EVENT_SHORT_CLICKED, NULL);

    frm_settings_btn_wifi_conf_lbl = lv_label_create(frm_settings_btn_wifi_conf);
    lv_label_set_text(frm_settings_btn_wifi_conf_lbl, "Configure");
    lv_obj_set_align(frm_settings_btn_wifi_conf_lbl, LV_ALIGN_CENTER);

    //date label
    frm_settings_date_lbl = lv_label_create(frm_settings_dialog);
    lv_label_set_text(frm_settings_date_lbl, "Date");
    lv_obj_align(frm_settings_date_lbl, LV_ALIGN_TOP_LEFT, 0, 100);

    //day
    frm_settings_day = lv_textarea_create(frm_settings_dialog);
    lv_obj_set_size(frm_settings_day, 60, 30);
    lv_obj_align(frm_settings_day, LV_ALIGN_TOP_LEFT, 40, 95);
    lv_textarea_set_accepted_chars(frm_settings_day, "1234567890");
    lv_textarea_set_max_length(frm_settings_day, 2);
    lv_textarea_set_placeholder_text(frm_settings_day, "dd");

    //month
    frm_settings_month = lv_textarea_create(frm_settings_dialog);
    lv_obj_set_size(frm_settings_month, 60, 30);
    lv_obj_align(frm_settings_month, LV_ALIGN_TOP_LEFT, 100, 95);
    lv_textarea_set_accepted_chars(frm_settings_month, "1234567890");
    lv_textarea_set_max_length(frm_settings_month, 2);
    lv_textarea_set_placeholder_text(frm_settings_month, "mm");

    //year
    frm_settings_year = lv_textarea_create(frm_settings_dialog);
    lv_obj_set_size(frm_settings_year, 60, 30);
    lv_obj_align(frm_settings_year, LV_ALIGN_TOP_LEFT, 160, 95);
    lv_textarea_set_accepted_chars(frm_settings_year, "1234567890");
    lv_textarea_set_max_length(frm_settings_year, 4);
    lv_textarea_set_placeholder_text(frm_settings_year, "yyyy");

    //time label
    frm_settings_time_lbl = lv_label_create(frm_settings_dialog);
    lv_label_set_text(frm_settings_time_lbl, "Time");
    lv_obj_align(frm_settings_time_lbl, LV_ALIGN_TOP_LEFT, 0, 135);

    //hour
    frm_settings_hour = lv_textarea_create(frm_settings_dialog);
    lv_obj_set_size(frm_settings_hour, 60, 30);
    lv_obj_align(frm_settings_hour, LV_ALIGN_TOP_LEFT, 40, 130);
    lv_textarea_set_accepted_chars(frm_settings_hour, "1234567890");
    lv_textarea_set_max_length(frm_settings_hour, 2);
    lv_textarea_set_placeholder_text(frm_settings_hour, "hh");

    //minute
    frm_settings_minute = lv_textarea_create(frm_settings_dialog);
    lv_obj_set_size(frm_settings_minute, 60, 30);
    lv_obj_align(frm_settings_minute, LV_ALIGN_TOP_LEFT, 100, 130);
    lv_textarea_set_accepted_chars(frm_settings_minute, "1234567890");
    lv_textarea_set_max_length(frm_settings_minute, 2);
    lv_textarea_set_placeholder_text(frm_settings_minute, "mm");

    // setDate button
    frm_settings_btn_setDate = lv_btn_create(frm_settings_dialog);
    lv_obj_set_size(frm_settings_btn_setDate, 50, 20);
    lv_obj_align(frm_settings_btn_setDate, LV_ALIGN_TOP_LEFT, 170, 135);
    lv_obj_add_event_cb(frm_settings_btn_setDate, applyDate, LV_EVENT_SHORT_CLICKED, NULL);

    //setDate label
    frm_settings_btn_setDate_lbl = lv_label_create(frm_settings_btn_setDate);
    lv_label_set_text(frm_settings_btn_setDate_lbl, "Set");
    lv_obj_set_align(frm_settings_btn_setDate_lbl, LV_ALIGN_CENTER);

    // color label
    frm_settings_btn_color_lbl = lv_label_create(frm_settings_dialog);
    lv_label_set_text(frm_settings_btn_color_lbl, "UI color");
    lv_obj_align(frm_settings_btn_color_lbl, LV_ALIGN_TOP_LEFT, 0, 170);
    
    //color 
    frm_settings_color = lv_textarea_create(frm_settings_dialog);
    lv_obj_set_size(frm_settings_color , 100, 30);
    lv_obj_align(frm_settings_color, LV_ALIGN_TOP_LEFT, 60, 165);
    lv_textarea_set_max_length(frm_settings_color, 6);
    lv_textarea_set_accepted_chars(frm_settings_color, "abcdefABCDEF1234567890");

    //apply color button
    frm_settings_btn_applycolor = lv_btn_create(frm_settings_dialog);
    lv_obj_set_size(frm_settings_btn_applycolor, 50, 20);
    lv_obj_align(frm_settings_btn_applycolor, LV_ALIGN_TOP_LEFT, 170, 170);
    lv_obj_add_event_cb(frm_settings_btn_applycolor, apply_color, LV_EVENT_SHORT_CLICKED, NULL);

    // apply color label
    frm_settings_btn_applycolor_lbl = lv_label_create(frm_settings_btn_applycolor);
    lv_label_set_text(frm_settings_btn_applycolor_lbl, "Apply");
    lv_obj_set_align(frm_settings_btn_applycolor_lbl, LV_ALIGN_CENTER);

    lv_obj_add_flag(frm_settings, LV_OBJ_FLAG_HIDDEN);
    
    // notification form************************************************************
    frm_not = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_not, 300, 25);
    lv_obj_align(frm_not, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_border_color(frm_not, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(frm_not, 0, 0 ) ;
    lv_obj_set_style_border_width(frm_not, 0, 0);

    //Label
    frm_not_lbl = lv_label_create(frm_not);
    lv_obj_align(frm_not_lbl, LV_ALIGN_TOP_LEFT, -10, -10);
    lv_label_set_long_mode(frm_not_lbl, LV_LABEL_LONG_SCROLL);

    lv_obj_add_flag(frm_not, LV_OBJ_FLAG_HIDDEN);

    /*form wifi********************************************************************/
    frm_wifi = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_wifi, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(frm_wifi, LV_OBJ_FLAG_SCROLLABLE);

    /*title*/
    frm_wifi_btn_title = lv_btn_create(frm_wifi);
    lv_obj_set_size(frm_wifi_btn_title, 200, 20);
    lv_obj_align(frm_wifi_btn_title, LV_ALIGN_TOP_LEFT, -15, -15);

    frm_wifi_btn_title_lbl = lv_label_create(frm_wifi_btn_title);
    lv_label_set_text(frm_wifi_btn_title_lbl, "WiFi Networks");
    lv_obj_set_align(frm_wifi_btn_title_lbl, LV_ALIGN_LEFT_MID);

    /*back button*/
    frm_wifi_btn_back = lv_btn_create(frm_wifi);
    lv_obj_set_size(frm_wifi_btn_back, 50, 20);
    lv_obj_align(frm_wifi_btn_back, LV_ALIGN_TOP_RIGHT, 15, -15);
    lv_obj_add_event_cb(frm_wifi_btn_back, hide_wifi, LV_EVENT_SHORT_CLICKED, NULL);

    frm_wifi_btn_back_lbl = lv_label_create(frm_wifi_btn_back);
    lv_label_set_text(frm_wifi_btn_back_lbl, "Back");
    lv_obj_set_align(frm_wifi_btn_back_lbl, LV_ALIGN_CENTER);

    /*scan button*/
    frm_wifi_btn_scan = lv_btn_create(frm_wifi);
    lv_obj_set_size(frm_wifi_btn_scan, 50, 20);
    lv_obj_align(frm_wifi_btn_scan, LV_ALIGN_TOP_RIGHT, -45, -15);
    lv_obj_add_event_cb(frm_wifi_btn_scan, wifi_scan, LV_EVENT_SHORT_CLICKED, NULL);

    frm_wifi_btn_scan_lbl = lv_label_create(frm_wifi_btn_scan);
    lv_label_set_text(frm_wifi_btn_scan_lbl, "Scan");
    lv_obj_set_align(frm_wifi_btn_scan_lbl, LV_ALIGN_CENTER);

    /*list*/
    frm_wifi_list = lv_list_create(frm_wifi);
    lv_obj_set_size(frm_wifi_list, 310, 190);
    lv_obj_align(frm_wifi_list, LV_ALIGN_TOP_LEFT, -10, 30);

    /*Connected network label*/
    frm_wifi_connected_to_lbl = lv_label_create(frm_wifi);
    lv_obj_align(frm_wifi_connected_to_lbl, LV_ALIGN_TOP_LEFT, 0, 10);
    lv_label_set_long_mode(frm_wifi_connected_to_lbl, LV_LABEL_LONG_SCROLL);
    lv_label_set_text(frm_wifi_connected_to_lbl, "");

    lv_obj_add_flag(frm_wifi, LV_OBJ_FLAG_HIDDEN);
    
    /*Form wifi auth simple******************************************************************/
    frm_wifi_simple = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_wifi_simple, 230, 110);
    lv_obj_align(frm_wifi_simple, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(frm_wifi_simple, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    frm_wifi_simple_title_lbl = lv_label_create(frm_wifi_simple);
    lv_obj_align(frm_wifi_simple_title_lbl, LV_ALIGN_TOP_MID, -10, -10);
    lv_label_set_long_mode(frm_wifi_simple_title_lbl, LV_LABEL_LONG_SCROLL);
    lv_label_set_text(frm_wifi_simple_title_lbl, "Connect to network");

    // password
    frm_wifi_simple_ta_pass = lv_textarea_create(frm_wifi_simple);
    lv_obj_set_size(frm_wifi_simple_ta_pass, 180, 30);
    lv_obj_align(frm_wifi_simple_ta_pass, LV_ALIGN_OUT_TOP_MID, -10, 20);
    lv_textarea_set_placeholder_text(frm_wifi_simple_ta_pass, "password");
    lv_textarea_set_password_mode(frm_wifi_simple_ta_pass, true);

    // see button
    frm_wifi_simple_btn_see = lv_btn_create(frm_wifi_simple);
    lv_obj_set_size(frm_wifi_simple_btn_see, 30, 20);
    lv_obj_align(frm_wifi_simple_btn_see, LV_ALIGN_TOP_LEFT, 180, 25);
    lv_obj_add_event_cb(frm_wifi_simple_btn_see, show_pass, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(frm_wifi_simple_btn_see, show_pass, LV_EVENT_RELEASED, NULL);

    frm_wifi_simple_btn_see_lbl = lv_label_create(frm_wifi_simple_btn_see);
    lv_label_set_text(frm_wifi_simple_btn_see_lbl, LV_SYMBOL_EYE_OPEN);
    lv_obj_set_align(frm_wifi_simple_btn_see_lbl, LV_ALIGN_CENTER);

    //connect button
    frm_wifi_simple_btn_connect = lv_btn_create(frm_wifi_simple);
    lv_obj_set_size(frm_wifi_simple_btn_connect, 70, 20);
    lv_obj_align(frm_wifi_simple_btn_connect, LV_ALIGN_OUT_TOP_LEFT, -10, 60);

    frm_wifi_simple_btn_connect_lbl = lv_label_create(frm_wifi_simple_btn_connect);
    lv_label_set_text(frm_wifi_simple_btn_connect_lbl, "Connect");
    lv_obj_set_align(frm_wifi_simple_btn_connect_lbl, LV_ALIGN_CENTER);

    //back button
    frm_wifi_simple_btn_back = lv_btn_create(frm_wifi_simple);
    lv_obj_set_size(frm_wifi_simple_btn_back, 50, 20);
    lv_obj_align(frm_wifi_simple_btn_back, LV_ALIGN_TOP_RIGHT, 10, 60);
    lv_obj_add_event_cb(frm_wifi_simple_btn_back, hide_simple, LV_EVENT_SHORT_CLICKED, NULL);

    frm_wifi_simple_btn_back_lbl = lv_label_create(frm_wifi_simple_btn_back);
    lv_label_set_text(frm_wifi_simple_btn_back_lbl, "Back");
    lv_obj_set_align(frm_wifi_simple_btn_back_lbl, LV_ALIGN_CENTER);

    lv_obj_add_flag(frm_wifi_simple, LV_OBJ_FLAG_HIDDEN);

    /*form wifi auth with login**************************************************************/
    frm_wifi_login = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_wifi_login, 230, 140);
    lv_obj_align(frm_wifi_login, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(frm_wifi_login, LV_OBJ_FLAG_SCROLLABLE);

    //title
    frm_wifi_login_title_lbl = lv_label_create(frm_wifi_login);
    lv_obj_align(frm_wifi_login_title_lbl, LV_ALIGN_TOP_MID, -10, -10);
    lv_label_set_long_mode(frm_wifi_login_title_lbl, LV_LABEL_LONG_SCROLL);
    lv_label_set_text(frm_wifi_login_title_lbl, "Connect to network");

    // login
    frm_wifi_login_ta_login = lv_textarea_create(frm_wifi_login);
    lv_obj_set_size(frm_wifi_login_ta_login, 180, 30);
    lv_obj_align(frm_wifi_login_ta_login, LV_ALIGN_OUT_TOP_MID, -10, 20);
    lv_textarea_set_placeholder_text(frm_wifi_login_ta_login, "login");

    // password
    frm_wifi_login_ta_pass = lv_textarea_create(frm_wifi_login);
    lv_obj_set_size(frm_wifi_login_ta_pass, 180, 30);
    lv_obj_align(frm_wifi_login_ta_pass, LV_ALIGN_OUT_TOP_MID, -10, 50);
    lv_textarea_set_placeholder_text(frm_wifi_login_ta_pass, "password");
    lv_textarea_set_password_mode(frm_wifi_login_ta_pass, true);

    // see button
    frm_wifi_login_btn_see = lv_btn_create(frm_wifi_login);
    lv_obj_set_size(frm_wifi_login_btn_see, 30, 20);
    lv_obj_align(frm_wifi_login_btn_see, LV_ALIGN_TOP_LEFT, 180, 55);
    lv_obj_add_event_cb(frm_wifi_login_btn_see, show_login_pass, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(frm_wifi_login_btn_see, show_login_pass, LV_EVENT_RELEASED, NULL);

    frm_wifi_login_btn_see_lbl = lv_label_create(frm_wifi_login_btn_see);
    lv_label_set_text(frm_wifi_login_btn_see_lbl, LV_SYMBOL_EYE_OPEN);
    lv_obj_set_align(frm_wifi_login_btn_see_lbl, LV_ALIGN_CENTER);

    //connect button
    frm_wifi_login_btn_connect = lv_btn_create(frm_wifi_login);
    lv_obj_set_size(frm_wifi_login_btn_connect, 70, 20);
    lv_obj_align(frm_wifi_login_btn_connect, LV_ALIGN_OUT_TOP_LEFT, -10, 90);

    frm_wifi_login_btn_connect_lbl = lv_label_create(frm_wifi_login_btn_connect);
    lv_label_set_text(frm_wifi_login_btn_connect_lbl, "Connect");
    lv_obj_set_align(frm_wifi_login_btn_connect_lbl, LV_ALIGN_CENTER);

    //back button
    frm_wifi_login_btn_back = lv_btn_create(frm_wifi_login);
    lv_obj_set_size(frm_wifi_login_btn_back, 50, 20);
    lv_obj_align(frm_wifi_login_btn_back, LV_ALIGN_TOP_RIGHT, 10, 90);
    lv_obj_add_event_cb(frm_wifi_login_btn_back, hide_wifi_login, LV_EVENT_SHORT_CLICKED, NULL);

    frm_wifi_login_btn_back_lbl = lv_label_create(frm_wifi_login_btn_back);
    lv_label_set_text(frm_wifi_login_btn_back_lbl, "Back");
    lv_obj_set_align(frm_wifi_login_btn_back_lbl, LV_ALIGN_CENTER);

    lv_obj_add_flag(frm_wifi_login, LV_OBJ_FLAG_HIDDEN);
}

void datetime(){
    const char * timezone = "<-03>3";
    char hm[6] = {'\0'};
    char date[12] = {'\0'};

    if(WiFi.status() == WL_CONNECTED){
        Serial.print("RSSI ");
        Serial.println(WiFi.RSSI());
        Serial.println(WiFi.localIP());

        esp_netif_init();
        if(sntp_enabled())
            sntp_stop();
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, "pool.ntp.org");
        sntp_init();
        setenv("TZ", timezone, 1);
        tzset();
        if(!getLocalTime(&timeinfo)){
            Serial.println("Failed to obtain time info");
            return;
        }
        Serial.println("Got time from NTP");
        
        
        Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
        strftime(hm, 6, "%H:%M %p", &timeinfo);
        strftime(date, 12, "%a, %b %d", &timeinfo);
        lv_label_set_text(frm_home_time_lbl, hm);
        lv_label_set_text(frm_home_date_lbl, date);
    }else
        Serial.println("Connect to a WiFi network to update the time and date");
}

char * wifi_auth_mode_to_str(wifi_auth_mode_t auth_mode){
    switch(auth_mode){
        case WIFI_AUTH_OPEN:
            return "OPEN";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA PSK";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2 PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA WPA2 PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WPA2 ENTERPRISE";
        case WIFI_AUTH_WPA3_PSK:
            return "WPA3 PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WPA2 WPA3 PSK";
        case WIFI_AUTH_WAPI_PSK:
            return "WAPI PSK";
        case WIFI_AUTH_MAX:
            return "MAX";
    }
}

void wifi_auto_toggle(){
    int n = 0;
    int c = 0;
    wifi_info wi;
    vector<wifi_info>list;
    char a[50] = {'\0'};
    
    Serial.print("Searching for wifi connections...");
    lv_label_set_text(frm_home_title_lbl, LV_SYMBOL_WIFI " Searching for wifi connections...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B);
    n = WiFi.scanNetworks();
    if(n > 0)
        for(int i = 0; i < n; i++){
            strcpy(wi.SSID, WiFi.SSID(i).c_str());
            list.push_back(wi);
        }
    else{
        Serial.println("No wifi networks found");
        lv_label_set_text(frm_home_title_lbl, LV_SYMBOL_WIFI "No wifi networks found");
    }
    Serial.println("done");
    lv_label_set_text(frm_home_title_lbl, LV_SYMBOL_WIFI "done");
    if(wifi_connected_nets.list.size() > 0){
        for(uint32_t i = 0; i < wifi_connected_nets.list.size(); i++){
            for(uint32_t j = 0; j < list.size(); j++){
                if(strcmp(wifi_connected_nets.list[i].SSID, list[j].SSID) == 0){
                    Serial.print("Connecting to ");
                    Serial.print(wifi_connected_nets.list[i].SSID);
                    strcpy(a, LV_SYMBOL_WIFI);
                    strcat(a, " Connecting to ");
                    strcat(a, wifi_connected_nets.list[i].SSID);
                    lv_label_set_text(frm_home_title_lbl, a);
                    if(wifi_connected_nets.list[i].auth_type == WIFI_AUTH_WPA2_ENTERPRISE){
                        esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)wifi_connected_nets.list[i].login, strlen(wifi_connected_nets.list[i].login));
                        esp_wifi_sta_wpa2_ent_set_username((uint8_t*)wifi_connected_nets.list[i].login, strlen(wifi_connected_nets.list[i].login));
                        esp_wifi_sta_wpa2_ent_set_password((uint8_t*)wifi_connected_nets.list[i].pass, strlen(wifi_connected_nets.list[i].pass));
                        esp_wifi_sta_wpa2_ent_enable();
                        WiFi.disconnect(true);
                        WiFi.mode(WIFI_STA);
                        WiFi.begin(wifi_connected_nets.list[i].SSID, wifi_connected_nets.list[i].pass);
                        
                        while(!WiFi.isConnected()){
                            Serial.print(".");
                            c++;
                            if(c == 30){
                                WiFi.disconnect();
                                c = 0;
                                break;
                            }
                            delay(1000);
                        }

                        if(WiFi.isConnected()){
                            last_wifi_con = i;
                            break;
                        }
                    }else if(wifi_connected_nets.list[i].auth_type == WIFI_AUTH_WPA2_PSK ||
                             wifi_connected_nets.list[i].auth_type == WIFI_AUTH_WPA_WPA2_PSK ||
                             wifi_connected_nets.list[i].auth_type == WIFI_AUTH_WEP){
                        WiFi.disconnect(true);
                        WiFi.mode(WIFI_STA);
                        WiFi.begin(wifi_connected_nets.list[i].SSID, wifi_connected_nets.list[i].pass);

                        while(!WiFi.isConnected()){
                            Serial.print(".");
                            c++;
                            if(c == 30){
                                WiFi.disconnect();
                                c = 0;
                                break;
                            }
                            delay(1000);
                        }

                        if(WiFi.isConnected()){
                            last_wifi_con = i;
                            break;
                        }
                    }
                }
            }
        }
        if(WiFi.isConnected()){
            lv_label_set_text(frm_home_title_lbl, LV_SYMBOL_WIFI " connected");
            lv_obj_set_style_text_color(frm_settings_btn_wifi_lbl, lv_color_hex(0x00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
            strcpy(connected_to, wifi_connected_nets.list[last_wifi_con].SSID);
            strcat(connected_to, " ");
            strcat(connected_to, WiFi.localIP().toString().c_str());
            Serial.println(" connected");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            lv_label_set_text(frm_home_title_lbl, "");
            datetime();
        }else{
            Serial.println("disconnected");
            lv_label_set_text(frm_home_title_lbl, LV_SYMBOL_WIFI " disconnected");
            lv_obj_set_style_text_color(frm_settings_btn_wifi_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

void wifi_auto_connect(void * param){
    int n = 0;
    int c = 0;
    wifi_info wi;
    vector<wifi_info>list;
    char a[50] = {'\0'};
    bool noNet = false;
    
    if(wifi_connected_nets.list.size() != 0){
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        Serial.print("Searching for wifi connections...");
        lv_label_set_text(frm_home_title_lbl, LV_SYMBOL_WIFI " Searching for wifi connections...");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_STA);
        
        n = WiFi.scanNetworks();
        if(n > 0)
            for(int i = 0; i < n; i++){
                strcpy(wi.SSID, WiFi.SSID(i).c_str());
                list.push_back(wi);
            }
        else{
            Serial.println("No wifi networks found");
            lv_label_set_text(frm_home_title_lbl, LV_SYMBOL_WIFI "No wifi networks found");
        }
        Serial.println("done");
        lv_label_set_text(frm_home_title_lbl, LV_SYMBOL_WIFI "done");
        if(wifi_connected_nets.list.size() > 0){
            for(uint32_t i = 0; i < wifi_connected_nets.list.size(); i++){
                for(uint32_t j = 0; j < list.size(); j++){
                    if(strcmp(wifi_connected_nets.list[i].SSID, list[j].SSID) == 0){
                        Serial.print("Connecting to ");
                        Serial.print(wifi_connected_nets.list[i].SSID);
                        strcpy(a, LV_SYMBOL_WIFI);
                        strcat(a, " Connecting to ");
                        strcat(a, wifi_connected_nets.list[i].SSID);
                        lv_label_set_text(frm_home_title_lbl, a);

                        if(wifi_connected_nets.list[i].auth_type == WIFI_AUTH_WPA2_ENTERPRISE){
                            esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)wifi_connected_nets.list[i].login, strlen(wifi_connected_nets.list[i].login));
                            esp_wifi_sta_wpa2_ent_set_username((uint8_t*)wifi_connected_nets.list[i].login, strlen(wifi_connected_nets.list[i].login));
                            esp_wifi_sta_wpa2_ent_set_password((uint8_t*)wifi_connected_nets.list[i].pass, strlen(wifi_connected_nets.list[i].pass));
                            esp_wifi_sta_wpa2_ent_enable();

                            WiFi.disconnect(true);
                            WiFi.mode(WIFI_STA);
                            //esp_wifi_set_ps(WIFI_PS_NONE);
                            //esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B);
                            WiFi.begin(wifi_connected_nets.list[i].SSID, wifi_connected_nets.list[i].pass);
                            
                            while(!WiFi.isConnected()){
                                Serial.print(".");
                                c++;
                                if(c == 30){
                                    WiFi.disconnect();
                                    c = 0;
                                    break;
                                }
                                delay(1000);
                            }

                            if(WiFi.isConnected()){
                                last_wifi_con = i;
                                break;
                            }
                        }else if(wifi_connected_nets.list[i].auth_type == WIFI_AUTH_WPA2_PSK ||
                                wifi_connected_nets.list[i].auth_type == WIFI_AUTH_WPA_WPA2_PSK ||
                                wifi_connected_nets.list[i].auth_type == WIFI_AUTH_WEP){

                            WiFi.disconnect(true);
                            WiFi.mode(WIFI_STA);
                            //esp_wifi_set_ps(WIFI_PS_NONE);
                            //esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B);
                            WiFi.begin(wifi_connected_nets.list[i].SSID, wifi_connected_nets.list[i].pass);

                            while(!WiFi.isConnected()){
                                Serial.print(".");
                                c++;
                                if(c == 30){
                                    WiFi.disconnect();
                                    c = 0;
                                    break;
                                }
                                delay(1000);
                            }

                            if(WiFi.isConnected()){
                                last_wifi_con = i;
                                break;
                            }
                        }
                    }
                }
                if(i == wifi_connected_nets.list.size())
                    noNet = true;
            }

            if(WiFi.isConnected()){
                lv_label_set_text(frm_home_title_lbl, LV_SYMBOL_WIFI " connected");
                lv_obj_set_style_text_color(frm_settings_btn_wifi_lbl, lv_color_hex(0x00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                strcpy(connected_to, wifi_connected_nets.list[last_wifi_con].SSID);
                strcat(connected_to, " ");
                strcat(connected_to, WiFi.localIP().toString().c_str());
                Serial.println(" connected");
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                lv_label_set_text(frm_home_title_lbl, "");
                datetime();
            }else{
                Serial.println("disconnected");
                lv_label_set_text(frm_home_title_lbl, LV_SYMBOL_WIFI " disconnected");
                WiFi.disconnect();
                WiFi.mode(WIFI_OFF);
            }
        }
    }
    if(task_wifi_auto != NULL){
        vTaskDelete(task_wifi_auto);
        task_wifi_auto = NULL;
    }
}

bool announce(){
    lora_packet_status hi;

    activity(lv_color_hex(0xffff00));
    strcpy(hi.id, user_id);
    strcpy(hi.status, "show");
    announcing = true;
    
    while(transmiting){
        Serial.print("transmiting ");
        Serial.println(transmiting?"true":"false");
        Serial.println("announce");
        lv_task_handler();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    while(gotPacket){
        Serial.print("gotPacket ");
        Serial.println(gotPacket?"true":"false");
        Serial.println("announce");
        lv_task_handler();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    Serial.println("Hi!");
    
    if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
        if(radio.startTransmit((uint8_t *)&hi, sizeof(lora_packet_status)) == 0){
            xSemaphoreGive(xSemaphore);
            announcing = false;
            transmiting = false;
            return true;
        }
    }
    
    xSemaphoreGive(xSemaphore);
    announcing = false;
    return false;
}

void setup(){
    bool ret = false;
    Serial.begin(115200);
    //delay(3000);

    //time interval checking online contacts
    contacts_list.setCheckPeriod(1);
    //Load contacts
    loadContacts();

    //load connected wifi networks
    if(wifi_connected_nets.load())
        Serial.println("wifi networks loaded");
    else
        Serial.println("no wifi networks loaded");

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

    //SD card
    if(!setupSD())
        Serial.println("cannot configure SD card");
    else
        Serial.println("SD card detected");

    //Load settings
    loadSettings();
    
    // set brightness
    analogWrite(BOARD_BL_PIN, 100);
    
    if(wifi_connected)
        datetime();
    //date time task
    xTaskCreatePinnedToCore(update_time, "update_time", 11000, (struct tm*)&timeinfo, 2, &task_date_time, 1);

    // Initial date
    setDate(2024, 1, 1, 0, 0, 0, 0);
    
    // battery
    initBat();
    xTaskCreatePinnedToCore(update_bat, "task_bat", 11000, NULL, 2, &task_bat, 1);

    // Notification task
    xTaskCreatePinnedToCore(notify, "notify", 11000, NULL, 1, &task_not, 1);
    if(hasRadio)
        lv_label_set_text(frm_home_title_lbl, "LoRa radio ready");
    else
        lv_label_set_text(frm_home_title_lbl, "LoRa radio failed");

    if(wifi_connected_nets.list.size() > 0)
        xTaskCreatePinnedToCore(wifi_auto_connect, "wifi_auto", 10000, NULL, 2, &task_wifi_auto, 0);

    if(announce())
        Serial.println("Hi everyone!");
    else
        Serial.println("Announcement failed");
}

void loop(){
    lv_task_handler();
    delay(5);
}
