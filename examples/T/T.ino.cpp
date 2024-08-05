# 1 "/tmp/tmpyfjcao8s"
#include <Arduino.h>
# 1 "/home/rene/Documents/T-Deck-p/examples/T/T.ino"
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
#include "esp_wifi.h"
#include "mutex"
#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "index.h"
#include "style.h"
#include "script.h"

#include "ArduinoJson.hpp"

using namespace httpsserver;
using namespace ArduinoJson;


LV_FONT_DECLARE(clocknum);
LV_FONT_DECLARE(ubuntu);

LV_IMG_DECLARE(icon_brightness);
LV_IMG_DECLARE(icon_lora2);
LV_IMG_DECLARE(bg2);

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

static pthread_mutex_t lvgl_mutex = NULL;
static pthread_mutex_t messages_mutex = NULL;
static pthread_mutex_t send_json_mutex = NULL;
static pthread_mutex_t websocket_send = NULL;
static pthread_mutex_t sound_mutex = NULL;

bool touchDected = false;
bool kbDected = false;
bool hasRadio = false;
bool wifi_connected = false;
bool isDX = false;
volatile bool transmiting = false;
volatile bool processing = false;
volatile bool gotPacket = false;
volatile bool sendingJson = false;
volatile bool server_ready = false;
volatile bool parsing = false;

lv_indev_t *touch_indev = NULL;
lv_indev_t *kb_indev = NULL;

TaskHandle_t task_recv_pkt = NULL,
             task_check_new_msg = NULL,
             task_not = NULL,
             task_date_time = NULL,
             task_bat = NULL,
             task_wifi_auto = NULL,
             task_wifi_scan = NULL,
             task_play = NULL,
             task_play_radio = NULL;

uint32_t msg_count = 0;

Contact_list contacts_list = Contact_list();
lora_incomming_messages messages_list = lora_incomming_messages();
notification notification_list = notification();
vector<lora_packet> received_packets;
vector<lora_packet> transmiting_packets;
vector<lora_stats> received_stats;


Contact * actual_contact = NULL;

char user_name[50] = "";
char user_id[7] = "";
char user_key[17] = "";
char http_admin_pass[50] = "admin";
char connected_to[200] = "";
char ui_primary_color_hex_text[7] = {'\u0000'};
uint32_t ui_primary_color = 0x5c81aa;
uint8_t brightness = 1;

struct tm timeinfo;
char time_zone[10] = "<+03>3";

int vRef = 0;

void update_bat(void * param);

void datetime();


void wifi_auto_toggle();


const char * wifi_auth_mode_to_str(wifi_auth_mode_t auth_mode);

volatile int last_wifi_con = -1;

HTTPSServer * secureServer = NULL;

const uint8_t maxClients = 1;
#define BLOCK_SIZE 16
volatile bool wifi_got_ip = false;
static void loadSettings();
static void saveSettings();
static void loadContacts();
static void saveContacts();
static void refresh_contact_list();
static void notify(void * param);
static void disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p );
static bool getTouch(int16_t &x, int16_t &y);
static void touchpad_read( lv_indev_drv_t *indev_driver, lv_indev_data_t *data );
static uint32_t keypad_get_key(void);
static void keypad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
void setupLvgl();
void scanDevices(TwoWire *w);
bool checkKb();
void setTZ();
void handleFav(HTTPRequest * req, HTTPResponse * res);
void handleRoot(HTTPRequest * req, HTTPResponse * res);
void handleStyle(HTTPRequest * req, HTTPResponse * res);
void handleScript(HTTPRequest * req, HTTPResponse * res);
void handle404(HTTPRequest * req, HTTPResponse * res);
void sendJSON(string json);
bool containsNonPrintableChars(const char *str);
void printMessages(const char * id);
void sendContactMessages(const char * id);
void setBrightness2(uint8_t level);
string settingsJSON();
void zero_padding(unsigned char *plaintext, size_t length, unsigned char *padded_plaintext);
void encrypt_text(unsigned char *text, unsigned char *key, size_t text_length, unsigned char *ciphertext);
void decrypt_text(unsigned char *ciphertext, unsigned char *key, size_t cipher_length, unsigned char *decrypted_text);
void setupServer(void * param);
void shutdownServer(void *param);
void update_rssi_snr_graph(float rssi, float snr);
void collectPackets(void * param);
void processPackets(void * param);
void processTransmittingPackets(void * param);
void processReceivedStats(void * param);
void onListen();
void onTransmit();
bool normalMode();
bool DXMode();
void setupRadio(lv_event_t * e);
void ping(lv_event_t * e);
void hide_contacts_frm(lv_event_t * e);
void show_contacts_form(lv_event_t * e);
void hide_settings(lv_event_t * e);
void show_settings(lv_event_t * e);
void hide_add_contacts_frm(lv_event_t * e);
void show_add_contacts_frm(lv_event_t * e);
void hide_edit_contacts(lv_event_t * e);
void show_edit_contacts(lv_event_t * e);
void hide_chat(lv_event_t * e);
void show_chat(lv_event_t * e);
void send_message(lv_event_t * e);
void copy_text(lv_event_t * e);
void check_new_msg(void * param);
void add_contact(lv_event_t * e);
void del_contact(lv_event_t * e);
void generateID(lv_event_t * e);
void generateKEY(lv_event_t * e);
void DX(lv_event_t * e);
void update_time(void *timeStruct);
void setDate(int yr, int month, int mday, int hr, int minute, int sec, int isDst);
void setDateTime();
void applyDate(lv_event_t * e);
void apply_color(lv_event_t * e);
void update_frm_contacts_status(uint16_t index, bool in_range);
void sendContactsStatusJson(const char * id, bool status);
void check_contacts_in_range();
void initBat();
long mapv(long x, long in_min, long in_max, long out_min, long out_max);
uint32_t read_bat();
void hide_wifi(lv_event_t * e);
void show_wifi(lv_event_t * e);
void wifi_apply(lv_event_t * e);
void forget_net(lv_event_t * e);
void wifi_select(lv_event_t * e);
void wifi_scan_task(void * param);
void wifi_scan(lv_event_t * e);
void update_wifi_icon();
void show_pass(lv_event_t * e);
void show_login_pass(lv_event_t * e);
void show_simple(lv_event_t * e);
void hide_simple(lv_event_t * e);
void show_wifi_login(lv_event_t * e);
void hide_wifi_login(lv_event_t * e);
void wifi_toggle(lv_event_t * e);
bool setupSD();
bool setupCoder();
void song(void * param);
void taskplaySong(void *p);
void notify_snd();
void show_especial(lv_event_t * e);
void activity(lv_color_t color);
void setBrightness(lv_event_t * e);
void wifi_con_info();
void tz_event(lv_event_t * e);
void ui();
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
void WiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
void wifi_auto_connect(void * param);
void announce();
void task_beacon(void * param);
void setup_sound();
void setup();
void loop();
#line 141 "/home/rene/Documents/T-Deck-p/examples/T/T.ino"
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
        if(v.size() > 5){
            strcpy(user_name, v[0].c_str());
            strcpy(user_id, v[1].c_str());
            v[2].toUpperCase();
            strcpy(ui_primary_color_hex_text, v[2].c_str());
            lv_textarea_set_text(frm_settings_color, ui_primary_color_hex_text);
            apply_color(NULL);
            brightness = atoi(v[3].c_str());
            strcpy(user_key, v[4].c_str());
            strcpy(http_admin_pass, v[5].c_str());
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

    file.println(user_name);
    file.println(user_id);
    file.println(ui_primary_color_hex_text);
    file.println(brightness);
    file.println(user_key);
    file.println(http_admin_pass);
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
    for(uint32_t index = 0; index < v.size(); index += 3){
        c.setName(v[index]);
        c.setID(v[index + 1]);
        c.setKey(v[index + 2]);
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
    file.println(c.getKey());
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





        if(contacts_list.getContact(i).inrange){
            lv_obj_set_style_bg_color(obj_status, lv_color_hex(0x00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        else{
            lv_obj_set_style_bg_color(obj_status, lv_color_hex(0xaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
        }

        lv_obj_align(obj_status, LV_ALIGN_RIGHT_MID, 0, 0);

        lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);

        lv_obj_add_event_cb(btn, show_edit_contacts, LV_EVENT_LONG_PRESSED, btn);
        lv_obj_add_event_cb(btn, show_chat, LV_EVENT_SHORT_CLICKED, btn);

    }


    saveContacts();
}




std::string generate_ID(uint8_t size){
  srand(time(NULL));
  static const char alphanum[] = "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  std::string ss;

  for (int i = 0; i < size; ++i) {
    ss[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }
  return ss;
}




static void notify(void * param){
    char n[30] = {'\0'};
    char b[13] = {'\0'};
    while(true){


        update_wifi_icon();

        if(notification_list.size() > 0){

            notification_list.pop(n, b);

            lv_obj_clear_flag(frm_not, LV_OBJ_FLAG_HIDDEN);

            lv_label_set_text(frm_not_lbl, n);
            lv_label_set_text(frm_not_symbol_lbl, b);


            lv_label_set_text(frm_home_title_lbl, n);
            lv_label_set_text(frm_home_symbol_lbl, b);

            vTaskDelay(1000 / portTICK_PERIOD_MS);

            lv_label_set_text(frm_not_lbl, "");
            lv_label_set_text(frm_not_symbol_lbl, "");
            lv_obj_add_flag(frm_not, LV_OBJ_FLAG_HIDDEN);

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
#define LVGL_BUFFER_SIZE ( TFT_HEIGHT * 100 )
    static lv_color_t buf[ LVGL_BUFFER_SIZE ];
#else
#define LVGL_BUFFER_SIZE (TFT_WIDTH * TFT_HEIGHT * sizeof(lv_color_t))
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


    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );


    disp_drv.hor_res = TFT_HEIGHT;
    disp_drv.ver_res = TFT_WIDTH;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
#ifdef BOARD_HAS_PSRAM
    disp_drv.full_refresh = 1;
#endif
    lv_disp_drv_register( &disp_drv );




    if (touchDected) {
        static lv_indev_drv_t indev_touchpad;
        lv_indev_drv_init( &indev_touchpad );
        indev_touchpad.type = LV_INDEV_TYPE_POINTER;
        indev_touchpad.read_cb = touchpad_read;
        touch_indev = lv_indev_drv_register( &indev_touchpad );
    }

    if (kbDected) {
        Serial.println("Keyboard registered!!");

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



const char * tz_list(uint16_t index){
    const char * tzl[] = {
        "<-11>11",
        "<-10>10",
        "<-09>9",
        "<-08>8",
        "<-07>7",
        "<-06>6",
        "<-05>5",
        "<-03>3",
        "<-01>1",
        "<+00>0",
        "<+01>1",
        "<+03>3",
        "<+03>3",
        "<+04>4",
        "<+05>5",
        "<+08>8",
        "<+09>9",
        "<+10>10"
    };

    if(index >= 0 && index < sizeof(tzl))
        return tzl[index];
    return NULL;
}


void setTZ(){
    int8_t err = 0;
    if(sntp_enabled())
        sntp_stop();
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    err = setenv("TZ", time_zone, 1);
    if(err == 0){
        tzset();
        Serial.print(time_zone);
        Serial.println(" set");
    }
    else
        Serial.println("Failed to change the time zone");
}





std::string contacts_to_json(){
    JsonDocument doc;
    std::string json;


    doc["command"] = "contacts";
    for(uint32_t i = 0; i < contacts_list.size(); i++){

        doc["contacts"][i]["id"] = contacts_list.getList()[i].getID();
        doc["contacts"][i]["name"] = contacts_list.getList()[i].getName();
        doc["contacts"][i]["key"] = contacts_list.getList()[i].getKey();
        doc["contacts"][i]["status"] = contacts_list.getList()[i].inrange ? "on" : "off";
    }

    serializeJson(doc, json);
    return json;
}

#define HEADER_USERNAME "X-USERNAME"
#define HEADER_GROUP "X-GROUP"


void middlewareAuthentication(HTTPRequest * req, HTTPResponse * res, std::function<void()> next) {
    req->setHeader(HEADER_USERNAME, "");
    req->setHeader(HEADER_GROUP, "");

    std::string reqUsername = req->getBasicAuthUser();
    std::string reqPassword = req->getBasicAuthPassword();

    if (reqUsername.length() > 0 && reqPassword.length() > 0) {
        bool authValid = true;
        std::string group = "";
        if (reqUsername == "admin" && strcmp(reqPassword.c_str(), http_admin_pass) == 0) {
            group = "ADMIN";
        } else if (reqUsername == "user" && reqPassword == "user") {
            group = "USER";
        } else {
            authValid = false;
        }

        if (authValid) {
            req->setHeader(HEADER_USERNAME, reqUsername);
            req->setHeader(HEADER_GROUP, group);
            next();
        } else {
            res->setStatusCode(401);
            res->setStatusText("Unauthorized");
            res->setHeader("Content-Type", "text/plain");
            res->setHeader("WWW-Authenticate", "Basic realm=\"T-Deck privileged area\"");
            res->println("401. Unauthorized");
        }
    }else{
        next();
    }
}




void middlewareAuthorization(HTTPRequest * req, HTTPResponse * res, std::function<void()> next) {
    std::string username = req->getHeader(HEADER_USERNAME);

    if (username == "" && req->getRequestString().substr(0,9) == "/") {
        res->setStatusCode(401);
        res->setStatusText("Unauthorized");
        res->setHeader("Content-Type", "text/plain");
        res->setHeader("WWW-Authenticate", "Basic realm=\"T-Deck privileged area\"");
        res->println("401. Unauthorized");

    } else {
        next();
    }
}

void handleFav(HTTPRequest * req, HTTPResponse * res) {
    Serial.println("Sending favicon");
    res->setHeader("Content-Type", "image/vnd.microsoft.icon");

    Serial.println("favicon sent.");
}




void handleRoot(HTTPRequest * req, HTTPResponse * res) {
    server_ready = false;
    Serial.println("Sending main page");
    res->setHeader("Content-Type", "text/html");
    res->setStatusCode(200);
    res->setStatusText("OK");
    res->printStd(index_html);
    server_ready = true;
}



void handleStyle(HTTPRequest * req, HTTPResponse * res) {
    Serial.println("Sending style.css");
    res->setHeader("Content-Type", "text/css");
    res->setStatusCode(200);
    res->setStatusText("OK");
    res->println(style_css);
}



void handleScript(HTTPRequest * req, HTTPResponse * res) {
    Serial.println("Sending script.js");
    res->setHeader("Content-Type", "application/javascript");
    res->setStatusCode(200);
    res->setStatusText("OK");
    res->println(script_js);
}



void handle404(HTTPRequest * req, HTTPResponse * res) {
    req->discardRequestBody();
    res->setStatusCode(404);
    res->setStatusText("Not Found");
    res->setHeader("Content-Type", "text/html");
    res->println("<!DOCTYPE html>");
    res->println("<html>");
    res->println("<head><title>Not Found</title></head>");
    res->println("<body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p></body>");
    res->println("</html>");
}

class ChatHandler : public WebsocketHandler{
    public:
        static WebsocketHandler * create();
        void onMessage(WebsocketInputStreambuf * input);
        void onClose();
        void onError(std::string error);
};

void ChatHandler::onError(std::string error){
    Serial.printf("WebSocket error %s", error);
    onClose();
}


ChatHandler * activeClient;

WebsocketHandler * ChatHandler::create() {
    Serial.println("Creating new chat client!");
    ChatHandler * handler = new ChatHandler();
    if (activeClient == nullptr) {
        activeClient = handler;

    }

    return handler;
}


void ChatHandler::onClose() {
    if (activeClient != NULL) {
        Serial.println("closing websocket");
        activeClient = NULL;
    }
}


void sendJSON(string json){



    if(secureServer == NULL)
        return;
    if(!secureServer->isRunning())
        return;
    if(!server_ready)
        return;
    char ip[20] = {'\0'};

    if(!wifi_got_ip)
        return;
    if(json.length() > 0){
        if(activeClient != NULL){
            if(parsing){
                Serial.println("(parsing)WebSocket busy, wait...");
                delay(100);
            }
            sendingJson = true;
            int count = 0;
            if(!activeClient->closed())
                activeClient->send(json, WebsocketHandler::SEND_TYPE_TEXT);
            sendingJson = false;
        }
    }else
        Serial.println("JSON empty");
}




bool containsNonPrintableChars(const char *str) {
    while (*str) {
        if (*str < 32 || *str > 126) {
            return true;
        }
        str++;
    }
    return false;
}

void printMessages(const char * id){
    vector<lora_packet> msgs;

    pthread_mutex_lock(&messages_mutex);
        msgs = messages_list.getMessages(id);
    pthread_mutex_unlock(&messages_mutex);
    Serial.println("=================================");
    if(msgs.size() > 0){
        for(uint32_t i = 0; i < msgs.size(); i++)
            Serial.println(msgs[i].msg);
    }
    Serial.println("=================================");
}


void sendContactMessages(const char * id){
    vector<lora_packet>msgs;
    JsonDocument doc;
    string json;

    pthread_mutex_lock(&messages_mutex);
        msgs = messages_list.getMessages(id);
    pthread_mutex_unlock(&messages_mutex);

    doc["command"] = "msg_list";
    doc["id"] = id;


    if(msgs.size() > 0){
        for(uint32_t i = 0; i < msgs.size(); i++){
            doc["messages"][i]["me"] = msgs[i].me;
            doc["messages"][i]["msg_date"] = msgs[i].date_time;

            if(msgs[i].me){
                doc["messages"][i]["msg"] = msgs[i].msg;


            }
            else{
                doc["messages"][i]["msg"] = msgs[i].msg;


            }
        }
    }

    serializeJson(doc, json);

    while(sendingJson){
        Serial.println("waiting other json to finish...");
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    pthread_mutex_lock(&send_json_mutex);
    sendJSON(json.c_str());
    pthread_mutex_unlock(&send_json_mutex);
    Serial.println(json.c_str());
}


void setBrightness2(uint8_t level){

    brightness = level;

    analogWrite(BOARD_BL_PIN, mapv(brightness, 0, 9, 100, 255));

    lv_roller_set_selected(frm_settings_brightness_roller, brightness, LV_ANIM_OFF);

    saveSettings();
}


string settingsJSON(){
    JsonDocument doc;
    string json;

    doc["command"] = "settings";
    doc["name"] = user_name;
    doc["id"] = user_id;
    doc["key"] = user_key;
    doc["admin_pass"] = http_admin_pass;
    doc["dx"] = isDX;
    doc["color"] = ui_primary_color_hex_text;
    doc["brightness"] = brightness;
    serializeJson(doc, json);
    return json;
}





void zero_padding(unsigned char *plaintext, size_t length, unsigned char *padded_plaintext) {

    memcpy(padded_plaintext, plaintext, length);

    memset(padded_plaintext + length, 0, BLOCK_SIZE - (length % BLOCK_SIZE));
}






void encrypt_text(unsigned char *text, unsigned char *key, size_t text_length, unsigned char *ciphertext) {

    size_t padded_len = ((text_length + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;


    unsigned char padded_text[padded_len];


    zero_padding(text, text_length, padded_text);


    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);


    mbedtls_aes_setkey_enc(&aes, key, 128);


    for (size_t i = 0; i < padded_len; i += BLOCK_SIZE) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, &padded_text[i], &ciphertext[i]);
    }


    mbedtls_aes_free(&aes);
}






void decrypt_text(unsigned char *ciphertext, unsigned char *key, size_t cipher_length, unsigned char *decrypted_text) {

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);


    mbedtls_aes_setkey_dec(&aes, key, 128);


    for (size_t i = 0; i < cipher_length; i += BLOCK_SIZE) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, &ciphertext[i], &decrypted_text[i]);
    }


    mbedtls_aes_free(&aes);
}



void parseCommands(std::string jsonString){
    while(sendingJson){
        Serial.println("(sendingJSON)WebSocket busy, wait...");
        delay(100);
    }
    parsing = true;
    JsonDocument doc;


    deserializeJson(doc, jsonString);

    const char * command = doc["command"];
    if(strcmp(command, "send") == 0){


        const char * id = (const char*)doc["id"];
        const char * msg = (const char*)doc["msg"];

        if(strcmp(id, "111111") == 0 || actual_contact == NULL)
            return;
        uint8_t text_length = strlen(msg);
        uint8_t padded_len = ((text_length + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
        unsigned char ciphertext[padded_len];
        encrypt_text((unsigned char *)msg, (unsigned char *)user_key, text_length, ciphertext);
        Serial.print("Encrypted => ");
        for(uint8_t i = 0; i < padded_len; i++)
            Serial.printf("%02x ", ciphertext[i]);
        Serial.printf("\nPadded len %d\n", padded_len);
        unsigned char decrypted_text[padded_len + 1] = {'\0'};
        decrypt_text(ciphertext, (unsigned char*)user_key, padded_len, decrypted_text);
        Serial.printf("\nDecrypted => %s\n", decrypted_text);



        if(hasRadio){

            lora_packet pkt;
            strcpy(pkt.sender, user_id);
            strcpy(pkt.destiny, id);


            strcpy(pkt.status, "send");
            strftime(pkt.date_time, sizeof(pkt.date_time)," - %a, %b %d %Y %H:%M", &timeinfo);
            memcpy(pkt.msg, ciphertext, padded_len);
            pkt.msg_size = padded_len;
            transmiting_packets.push_back(pkt);
            pkt.me = true;

            strcpy(pkt.msg, msg);
            Serial.print("Adding answer to ");
            Serial.println(pkt.destiny);
            Serial.println(pkt.msg);

            pthread_mutex_lock(&messages_mutex);
            messages_list.addMessage(pkt);
            pthread_mutex_unlock(&messages_mutex);

        }else
            Serial.println("Radio not configured");
    }else if(strcmp(command, "contacts") == 0){

        while(sendingJson){
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        pthread_mutex_lock(&send_json_mutex);
        sendJSON(contacts_to_json());
        pthread_mutex_unlock(&send_json_mutex);
    }else if(strcmp(command, "sel_contact") == 0){

        actual_contact = contacts_list.getContactByID(doc["id"]);
        if(actual_contact != NULL){
            Serial.println("Contact selected");

            sendContactMessages(actual_contact->getID().c_str());
        }
        else
            Serial.println("Contact selection failed, id not found");
    }else if(strcmp(command, "edit_contact") == 0){
        Contact * c = NULL;
        bool edited = false;
        if(strcmp(doc["id"], doc["newid"]) != 0){

            c = contacts_list.getContactByID(doc["newid"]);
            if(c != NULL){
                while(sendingJson){
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
                pthread_mutex_lock(&send_json_mutex);
                sendJSON("{\"command\" : \"notification\", \"message\" : \"ID in use.\"}");
                pthread_mutex_unlock(&send_json_mutex);
            }else{
                edited = true;
            }
        }
        else
            edited = true;
        if(edited){

            c = contacts_list.getContactByID(doc["id"]);
            if(c != NULL){

                c->setID((const char *)doc["newid"]);
                c->setName((const char *)doc["newname"]);
                c->setKey((const char *)doc["newkey"]);

                saveContacts();
                while(sendingJson){
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }

                pthread_mutex_lock(&send_json_mutex);
                sendJSON(contacts_to_json().c_str());
                sendJSON("{\"command\" : \"notification\", \"message\" : \"Edited.\"}");
                pthread_mutex_unlock(&send_json_mutex);
            }
        }
    }else if(strcmp(command, "del_contact") == 0){

        Contact * c = contacts_list.getContactByID(doc["id"]);
        if(c != NULL){

            if(contacts_list.del(*c)){

                Serial.print("Contact id ");
                Serial.print((const char*)doc["id"]);
                Serial.println(" deleted.");

                saveContacts();
                while(sendingJson){
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }

                pthread_mutex_lock(&send_json_mutex);
                sendJSON(contacts_to_json().c_str());
                sendJSON("{\"command\" : \"notification\", \"message\" : \"Contact deleted.\"}");
                pthread_mutex_unlock(&send_json_mutex);
            }else{
                while(sendingJson){
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
                pthread_mutex_lock(&send_json_mutex);
                sendJSON("{\"command\" : \"notification\", \"message\" : \"Error deleting contact.\"}");
                pthread_mutex_unlock(&send_json_mutex);
            }
        }else{
            while(sendingJson){
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            pthread_mutex_lock(&send_json_mutex);
            sendJSON("{\"command\" : \"notification\", \"message\" : \"Contact not found.\"}");
            pthread_mutex_unlock(&send_json_mutex);
        }
    }else if(strcmp(command, "new_contact") == 0){

        Contact * c = contacts_list.getContactByID(doc["id"]);
        if(c == NULL){

            c = new Contact(doc["name"], doc["id"]);
            c->setKey(doc["key"]);

            if(contacts_list.add(*c)){

                Serial.print("New contact ID ");
                Serial.print((const char *)doc["id"]);
                Serial.println(" added.");

                saveContacts();
                while(sendingJson){
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }

                pthread_mutex_lock(&send_json_mutex);
                sendJSON(contacts_to_json());
                sendJSON("{\"command\" : \"notification\", \"message\" : \"Contact added.\"}");
                pthread_mutex_unlock(&send_json_mutex);
            }
            else{
                while(sendingJson){
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
                pthread_mutex_lock(&send_json_mutex);
                sendJSON("{\"command\" : \"notification\", \"message\" : \"Fail to add new contact.\"}");
                pthread_mutex_unlock(&send_json_mutex);
            }
        }else{
            while(sendingJson){
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            pthread_mutex_lock(&send_json_mutex);
            sendJSON("{\"command\" : \"notification\", \"message\" : \"ID in use.\"}");
            pthread_mutex_unlock(&send_json_mutex);
        }
    }else if(strcmp(command, "read_settings") == 0){
        while(sendingJson){
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        pthread_mutex_lock(&send_json_mutex);
        sendJSON(settingsJSON());
        pthread_mutex_unlock(&send_json_mutex);
    }else if(strcmp(command, "set_brightness") == 0){
        setBrightness2((uint8_t)doc["brightness"]);
    }else if(strcmp(command, "set_ui_color") == 0){
        char color[7] = "";

        strcpy(color, doc["color"]);

        if(strcmp(color, "") == 0)
            return;

        lv_textarea_set_text(frm_settings_color, color);

        apply_color(NULL);

        saveSettings();
    }else if(strcmp(command, "set_name_id") == 0){

        strcpy(user_id, doc["id"]);
        strcpy(user_name, doc["name"]);
        strcpy(user_key, doc["key"]);

        lv_textarea_set_text(frm_settings_name, user_name);
        lv_textarea_set_text(frm_settings_id, user_id);
        lv_textarea_set_text(frm_settings_key, user_key);

        saveSettings();
        while(sendingJson){
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        pthread_mutex_lock(&send_json_mutex);
        sendJSON("{\"command\" : \"notification\", \"message\" : \"Name, ID and key saved.\"}");
        pthread_mutex_unlock(&send_json_mutex);
    }else if(strcmp(command, "set_dx_mode") == 0){
        if(doc["dx"]){
            if(DXMode()){
            while(sendingJson){
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
                pthread_mutex_lock(&send_json_mutex);
                sendJSON("{\"command\" : \"notification\", \"message\" : \"DX mode.\"}");
                pthread_mutex_unlock(&send_json_mutex);
            }
        }
        else{
            if(normalMode()){
                while(sendingJson){
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
                pthread_mutex_lock(&send_json_mutex);
                sendJSON("{\"command\" : \"notification\", \"message\" : \"Normal mode.\"}");
                pthread_mutex_unlock(&send_json_mutex);
            }
        }
    }else if(strcmp(command, "set_date") == 0){


        lv_textarea_set_text(frm_settings_day, doc["d"]);
        lv_textarea_set_text(frm_settings_month, doc["m"]);
        lv_textarea_set_text(frm_settings_year, doc["y"]);
        lv_textarea_set_text(frm_settings_hour, doc["hh"]);
        lv_textarea_set_text(frm_settings_minute, doc["mm"]);


        setDateTime();
        while(sendingJson){
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        pthread_mutex_lock(&send_json_mutex);
        sendJSON("{\"command\" : \"notification\", \"message\" : \"Date and time set.\"}");
        pthread_mutex_unlock(&send_json_mutex);
    }else if(strcmp(command, "set_tz") == 0){
        if(doc["tz"] != ""){
            strcpy(time_zone, doc["tz"]);
            setTZ();
        }
    }else if(strcmp(command, "admin_pass") == 0){
        if(doc["admin_pass"] != ""){
            strcpy(http_admin_pass, doc["admin_pass"]);
            while(sendingJson){
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            pthread_mutex_lock(&send_json_mutex);
            sendJSON("{\"command\" : \"notification\", \"message\" : \"Admin password changed.\"}");
            pthread_mutex_unlock(&send_json_mutex);
            saveSettings();
        }
    }
    parsing = false;
}



void ChatHandler::onMessage(WebsocketInputStreambuf * inbuf) {
    if(!server_ready)
        return;

    std::ostringstream ss;
    std::string msg;
    ss << inbuf;
    msg = ss.str();

    Serial.println(msg.c_str());

    while(parsing){
        Serial.println("onMessage parsing");
        delay(100);
    }
    while(sendingJson){
        Serial.println("onMessage JSON");
        delay(100);
    }
    while(this->sending){
        Serial.println("onMessage http_send");
        delay(100);
    }
    parseCommands(msg);
}



void setupServer(void * param){
    server_ready = false;
    while(!wifi_got_ip){
        Serial.println("No IP address");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }


    if(activeClient != NULL){
        activeClient->close();
        activeClient = nullptr;
    }

    Serial.println("Creating ssl certificate...");
    lv_label_set_text(frm_home_title_lbl, "Creating ssl certificate...");
    lv_label_set_text(frm_home_symbol_lbl, LV_SYMBOL_HOME);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    SSLCert * cert;


    char cn[30] = {'\0'};
    strcpy(cn, "CN=TDECK");
    strcat(cn, user_id);
    strcat(cn, ",O=TDECK");
    strcat(cn, user_id);
    strcat(cn, ",C=BR");
    cert = new SSLCert();

    int createCertResult = createSelfSignedCert(
    *cert,
    KEYSIZE_2048,
    cn,
    "20240302000000",
    "20340302000000"
    );

    if (createCertResult != 0) {
        Serial.printf("Error generating certificate");
        return;
    }
    Serial.printf("cert size => %d\n", cert->getCertLength());
    Serial.println("Certificate done.");
    lv_label_set_text(frm_home_title_lbl, "Certificate done.");
    lv_label_set_text(frm_home_symbol_lbl, LV_SYMBOL_HOME);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    lv_label_set_text(frm_home_title_lbl, "Starting https server...");
    lv_label_set_text(frm_home_symbol_lbl, LV_SYMBOL_HOME);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("Starting server...");

    secureServer = new HTTPSServer(cert, 443, maxClients);





    ResourceNode * nodeRoot = new ResourceNode("/", "GET", &handleRoot);



    ResourceNode * node404 = new ResourceNode("", "GET", &handle404);


    secureServer->registerNode(nodeRoot);


    secureServer->registerNode(node404);


    WebsocketNode * chatNode = new WebsocketNode("/chat", ChatHandler::create);
    secureServer->registerNode(chatNode);
    secureServer->setDefaultNode(node404);

    secureServer->addMiddleware(&middlewareAuthentication);
    secureServer->addMiddleware(&middlewareAuthorization);

    secureServer->start();
    if (secureServer->isRunning()) {

        lv_obj_clear_flag(frm_settings_wifi_http_btn, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(frm_home_title_lbl, "Server ready.");
        lv_label_set_text(frm_home_symbol_lbl, LV_SYMBOL_HOME);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        Serial.println("Server ready.");
        lv_label_set_text(frm_home_title_lbl, "");
        lv_label_set_text(frm_home_symbol_lbl, "");
        server_ready = true;
    }

    if(param != NULL)
        vTaskDelete(NULL);
}

void shutdownServer(void *param){
    if(server_ready)
        if(secureServer != NULL){
            if(activeClient != NULL){
                activeClient->close(1000, "Server shutdown");
                activeClient = nullptr;
            }

            secureServer->stop();
            secureServer->~HTTPSServer();
            secureServer = NULL;

            lv_obj_add_flag(frm_settings_wifi_http_btn, LV_OBJ_FLAG_HIDDEN);
            Serial.println("Server ended.");
            server_ready = false;
        }

}




void update_rssi_snr_graph(float rssi, float snr){
    lv_chart_set_next_value(frm_home_rssi_chart, frm_home_rssi_series, rssi);
    lv_chart_set_next_value(frm_home_rssi_chart, frm_home_snr_series, snr);
    lv_chart_refresh(frm_home_rssi_chart);
}



void collectPackets(void * param){
    lora_packet p;
    uint16_t packet_size;
    lora_stats ls;
    float rssi, snr;

    while(true){
        if(gotPacket){


            if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
                radio.standby();
                packet_size = radio.getPacketLength();
                radio.readData((uint8_t*)&p, packet_size);

                rssi = radio.getRSSI();
                snr = radio.getSNR();
                sprintf(ls.rssi, "%.2f", rssi);
                sprintf(ls.snr, "%.2f", snr);
                gotPacket = false;

                radio.startReceive();
                xSemaphoreGive(xSemaphore);
            }

            if(packet_size > 0 && packet_size <= sizeof(lora_packet)){

                strftime(p.date_time, sizeof(p.date_time)," - %a, %b %d %Y %H:%M", &timeinfo);
                if(strcmp(p.sender, user_id) != 0){
                    received_packets.push_back(p);

                    received_stats.push_back(ls);
                    Serial.print("Updating rssi graph...");
                    update_rssi_snr_graph(rssi, snr);
                    Serial.println("rssi graph updated.");
                }
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void processPackets(void * param){
    lora_packet p, pong;
    char dec_msg[200] = {'\0'};
    char message[200] = {'\0'};
    char pmsg[200] = {'\0'};

    while(true){
        if(received_packets.size() > 0){

            pthread_mutex_lock(&lvgl_mutex);
            activity(lv_color_hex(0x00ff00));
            pthread_mutex_unlock(&lvgl_mutex);
            p = received_packets[0];
            received_packets.erase(received_packets.begin());

            if(strcmp(p.status, "send") == 0){
                lora_packet s;
                strcpy(s.status, "recv");
                strcpy(s.sender, user_id);
                strcpy(s.destiny, p.sender);
                transmiting_packets.push_back(s);
                Contact * c = contacts_list.getContactByID(p.sender);

                if(c != NULL){

                    unsigned char decrypted_text[p.msg_size + 1] = {'\0'};
                    decrypt_text((unsigned char *)p.msg, (unsigned char*)c->getKey().c_str(), p.msg_size, decrypted_text);
                    Serial.printf("msg size => %d\n", p.msg_size);
                    Serial.printf("Decrypted => %s\n", decrypted_text);
                    strcpy(dec_msg, (const char *)decrypted_text);

                    strcpy((char*)p.msg, dec_msg);

                    strcpy(p.destiny, p.sender);

                    pthread_mutex_lock(&messages_mutex);
                    messages_list.addMessage(p);
                    pthread_mutex_unlock(&messages_mutex);

                    sendContactMessages(p.sender);
                    while(sendingJson){
                        vTaskDelay(10 / portTICK_PERIOD_MS);
                    }

                    pthread_mutex_lock(&send_json_mutex);
                    sendJSON("{\"command\" : \"playNewMessage\"}");
                    pthread_mutex_unlock(&send_json_mutex);

                    strcpy(message, c->getName().c_str());
                    strcat(message, ": ");

                    if(sizeof(p.msg) > 149){
                        memcpy(pmsg, dec_msg, 149);
                        strcat(message, pmsg);
                    }
                    else
                        strcat(message, p.msg);

                    message[30] = '.';
                    message[31] = '.';
                    message[32] = '.';
                    message[33] = '\0';

                    notification_list.add(message, LV_SYMBOL_ENVELOPE);
                }
            }

            else if(strcmp(p.status, "recv") == 0){

                strcpy(p.destiny, p.sender);

                strcpy(p.msg, "[received]");

                pthread_mutex_lock(&messages_mutex);
                messages_list.addMessage(p);
                pthread_mutex_unlock(&messages_mutex);

                sendContactMessages(p.sender);
            }

            else if(strcmp(p.status, "ping") == 0 && strcmp(p.destiny, user_id) == 0){

                notification_list.add("ping", LV_SYMBOL_DOWNLOAD);

                strcpy(pong.sender, user_id);
                strcpy(pong.destiny, p.sender);
                strcpy(pong.status, "pong");
                transmiting_packets.push_back(pong);
            }

            else if(strcmp(p.status, "pong") == 0 && strcmp(p.destiny, user_id) == 0){

                notify_snd();
                Serial.println("pong");

                strcpy(message, "pong ");

                notification_list.add(message, LV_SYMBOL_DOWNLOAD);
            }

            else if(strcmp(p.status, "show") == 0){

                Contact * c = contacts_list.getContactByID(p.sender);
                if(c != NULL){

                    c->inrange = true;


                    c->timeout = millis();
                    Serial.println("Announcement packet received");
                    c = NULL;
                }
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void processTransmittingPackets(void * param){
    lora_packet p;
    lora_packet_status ps;

    while(true){
        if(transmiting_packets.size() > 0){
            vTaskDelay(100 / portTICK_PERIOD_MS);
            p = transmiting_packets[0];
            transmiting_packets.erase(transmiting_packets.begin());
            strcpy(ps.sender, p.sender);
            strcpy(ps.destiny, p.destiny);
            strcpy(ps.status, p.status);
            Serial.println("======processTransmittingPackets=======");
            Serial.print("Sender ");
            Serial.println(ps.sender);
            Serial.print("Destiny ");
            Serial.println(ps.destiny);
            Serial.print("Status ");
            Serial.println(ps.status);

            pthread_mutex_lock(&lvgl_mutex);
            activity(lv_color_hex(0xff0000));
            pthread_mutex_unlock(&lvgl_mutex);

            if(strcmp(p.status, "recv") == 0){
                while(gotPacket)
                    vTaskDelay(10 / portTICK_PERIOD_MS);

                transmiting = true;

                if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
                    if(radio.transmit((uint8_t*)&ps, sizeof(ps)) == RADIOLIB_ERR_NONE)
                        Serial.println("Confirmation sent");
                    else
                        Serial.println("Confirmation not sent");
                    xSemaphoreGive(xSemaphore);
                }
                transmiting = false;
            }

            if(strcmp(p.status, "send") == 0){
                while(gotPacket)
                    vTaskDelay(10 / portTICK_PERIOD_MS);

                activity(lv_color_hex(0xff0000));
                transmiting = true;

                if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
                    if(radio.transmit((uint8_t*)&p, sizeof(p)) == RADIOLIB_ERR_NONE)
                        Serial.println("Message sent");
                    else
                        Serial.println("Message not sent");
                    xSemaphoreGive(xSemaphore);
                }
                transmiting = false;
            }

            if(strcmp(p.status, "ping") == 0){
                while(gotPacket)
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                transmiting = true;

                if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
                    if(radio.transmit((uint8_t*)&ps, sizeof(ps)) == RADIOLIB_ERR_NONE)
                        Serial.println("Ping sent");
                    else
                        Serial.println("Ping not sent");
                    xSemaphoreGive(xSemaphore);
                }
                transmiting = false;
            }

            if(strcmp(p.status, "show") == 0){
                while(gotPacket)
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                transmiting = true;

                if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
                    if(radio.transmit((uint8_t*)&ps, sizeof(ps)) == RADIOLIB_ERR_NONE)
                        Serial.println("Announcement sent - Hi!");
                    else
                        Serial.println("Announcement not sent");
                    xSemaphoreGive(xSemaphore);
                }
                transmiting = false;
            }
            Serial.println("=======================================");
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}


void processReceivedStats(void * param){
    lora_stats st;
    string json;
    JsonDocument doc;
    char message[200] = {'\0'};

    while (true){
        if(received_stats.size() > 0){
            st = received_stats[0];
            received_stats.erase(received_stats.begin());

            strcpy(message, "[RSSI:");
            strcat(message, st.rssi);
            strcat(message, " dBm");
            strcat(message, " SNR:");
            strcat(message, st.snr);
            strcat(message, " dBm]");
            Serial.println(message);

            doc["command"] = "rssi_snr";
            doc["rssi"] = st.rssi;
            doc["snr"] = st.snr;
            serializeJson(doc, json);
            while(sendingJson){
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            while(parsing){
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            pthread_mutex_lock(&send_json_mutex);
            sendJSON(json);
            pthread_mutex_unlock(&send_json_mutex);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
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

        if (radio.setFrequency(RADIO_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY) {
            Serial.println(F("Selected frequency is invalid for this module!"));
            return false;
        }


        if (radio.setBandwidth(250.0) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
            Serial.println(F("Selected bandwidth is invalid for this module!"));
            return false;
        }


        if (radio.setSpreadingFactor(10) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
            Serial.println(F("Selected spreading factor is invalid for this module!"));
            return false;
        }


        if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE) {
            Serial.println(F("Selected coding rate is invalid for this module!"));
            return false;
        }


        if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
            Serial.println(F("Unable to set sync word!"));
            return false;
        }


        if (radio.setOutputPower(17) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
            Serial.println(F("Selected output power is invalid for this module!"));
            return false;
        }



        if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
            Serial.println(F("Selected current limit is invalid for this module!"));
            return false;
        }


        if (radio.setPreambleLength(15) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
            Serial.println(F("Selected preamble length is invalid for this module!"));
            return false;
        }


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
# 1796 "/home/rene/Documents/T-Deck-p/examples/T/T.ino"
    if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
        radio.sleep(false);
        radio.reset();
        radio.begin();


        if (radio.setFrequency(RADIO_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY) {
            Serial.println(F("Selected frequency is invalid for this module!"));
            return false;
        }


        if (radio.setBandwidth(125.0) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
            Serial.println(F("Selected bandwidth is invalid for this module!"));
            return false;
        }


        if (radio.setSpreadingFactor(12) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
            Serial.println(F("Selected spreading factor is invalid for this module!"));
            return false;
        }


        if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE) {
            Serial.println(F("Selected coding rate is invalid for this module!"));
            return false;
        }


        if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
            Serial.println(F("Unable to set sync word!"));
            return false;
        }


        if (radio.setOutputPower(22) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
            Serial.println(F("Selected output power is invalid for this module!"));
            return false;
        }



        if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
            Serial.println(F("Selected current limit is invalid for this module!"));
            return false;
        }


        if (radio.setPreambleLength(15) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
            Serial.println(F("Selected preamble length is invalid for this module!"));
            return false;
        }


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
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);

    int state = radio.begin(RADIO_FREQ);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Start Radio success!");
    } else {
        Serial.print("Start Radio failed,code:");
        Serial.println(state);

    }




    if (radio.setFrequency(RADIO_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Serial.println(F("Selected frequency is invalid for this module!"));

    }


    if (radio.setBandwidth(250.0) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        Serial.println(F("Selected bandwidth is invalid for this module!"));

    }


    if (radio.setSpreadingFactor(10) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        Serial.println(F("Selected spreading factor is invalid for this module!"));

    }


    if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        Serial.println(F("Selected coding rate is invalid for this module!"));

    }


    if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
        Serial.println(F("Unable to set sync word!"));

    }


    if (radio.setOutputPower(10) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("Selected output power is invalid for this module!"));

    }



    if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
        Serial.println(F("Selected current limit is invalid for this module!"));

    }


    if (radio.setPreambleLength(15) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
        Serial.println(F("Selected preamble length is invalid for this module!"));

    }


    if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
        Serial.println(F("Selected CRC is invalid for this module!"));

    }
    int32_t code = 0;


    radio.setPacketReceivedAction(onListen);



    code = radio.startReceive();
    Serial.print("setup radio start receive code ");
    Serial.println(code);

    if(code == RADIOLIB_ERR_NONE){

        hasRadio = true;
    }


}



void ping(lv_event_t * e){

    lora_packet my_packet;
    notify_snd();
    strcpy(my_packet.sender, user_id);

    if(actual_contact != NULL){
        strcpy(my_packet.destiny, actual_contact->getID().c_str());
        transmiting_packets.push_back(my_packet);
    }
    else
        return;

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

    lv_obj_t * obj = lv_event_get_target(e);

    lv_obj_t * menu = (lv_obj_t*)lv_event_get_user_data(e);
    bool isroot = false;
    if(code == LV_EVENT_SHORT_CLICKED){

        isroot = lv_menu_back_btn_is_root(menu, obj);
        if(isroot) {



            strcpy(user_name, lv_textarea_get_text(frm_settings_name));

            strcpy(user_id, lv_textarea_get_text(frm_settings_id));

            strcpy(user_key, lv_textarea_get_text(frm_settings_key));

            strcpy(http_admin_pass, lv_textarea_get_text(frm_settings_admin_password));

            lv_obj_add_flag(frm_settings, LV_OBJ_FLAG_HIDDEN);

            saveSettings();
        }
    }
}


void show_settings(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    char year[5], month[3], day[3], hour[3], minute[3], color[7];

    if(code == LV_EVENT_SHORT_CLICKED){

        lv_obj_clear_flag(frm_settings, LV_OBJ_FLAG_HIDDEN);

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
        lv_textarea_set_text(frm_settings_key, user_key);
        lv_textarea_set_text(frm_settings_admin_password, http_admin_pass);

        sprintf(color, "%lX", ui_primary_color);

        lv_textarea_set_text(frm_settings_color, color);

        lv_roller_set_selected(frm_settings_brightness_roller, brightness, LV_ANIM_OFF);
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


void hide_edit_contacts(lv_event_t * e){
    const char * name, * id, * key;
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_edit_contacts != NULL){

            name = lv_textarea_get_text(frm_edit_text_name);
            id = lv_textarea_get_text(frm_edit_text_ID);
            key = lv_textarea_get_text(frm_edit_text_key);

            if(strcmp(name, "") != 0 && strcmp(id, "") != 0){

                actual_contact->setID(id);
                actual_contact->setName(name);
                actual_contact->setKey(key);
                Serial.println("ID and name updated");

                actual_contact = NULL;

                lv_obj_add_flag(frm_edit_contacts, LV_OBJ_FLAG_HIDDEN);
                Serial.println("Contact updated");

                refresh_contact_list();

                saveContacts();
            }else
                Serial.println("Name or ID cannot be empty");
        }
    }
}


void show_edit_contacts(lv_event_t * e){
    char id[7] = {'\u0000'};

    lv_event_code_t code = lv_event_get_code(e);

    lv_obj_t * btn = (lv_obj_t * )lv_event_get_user_data(e);

    lv_obj_t * lbl_id = lv_obj_get_child(btn, 2);

    if(code == LV_EVENT_LONG_PRESSED){
        if(frm_edit_contacts != NULL){

            strcpy(id, lv_label_get_text(lbl_id));

            actual_contact = contacts_list.getContactByID(id);

            lv_obj_clear_flag(frm_edit_contacts, LV_OBJ_FLAG_HIDDEN);

            lv_textarea_set_text(frm_edit_text_name, actual_contact->getName().c_str());
            lv_textarea_set_text(frm_edit_text_ID, actual_contact->getID().c_str());
            lv_textarea_set_text(frm_edit_text_key, actual_contact->getKey().c_str());
        }
    }
}


void hide_chat(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_chat != NULL){
            if(task_check_new_msg != NULL){

                vTaskDelete(task_check_new_msg);
                task_check_new_msg = NULL;
                Serial.println("task_check_new_msg deleted");
            }

            msg_count = 0;

            lv_obj_add_flag(frm_chat, LV_OBJ_FLAG_HIDDEN);



        }
    }
}


void show_chat(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_SHORT_CLICKED){

        lv_obj_t * btn = (lv_obj_t *)lv_event_get_user_data(e);

        pthread_mutex_lock(&lvgl_mutex);
        lv_obj_t * lbl = lv_obj_get_child(btn, 1);
        pthread_mutex_unlock(&lvgl_mutex);

        pthread_mutex_lock(&lvgl_mutex);
        lv_obj_t * lbl_id = lv_obj_get_child(btn, 2);
        pthread_mutex_unlock(&lvgl_mutex);

        const char * name = lv_label_get_text(lbl);
        const char * id = lv_label_get_text(lbl_id);

        actual_contact = contacts_list.getContactByID(id);

        char title[60] = "Chat with ";
        strcat(title, name);
        if(frm_chat != NULL){

            lv_obj_clear_flag(frm_chat, LV_OBJ_FLAG_HIDDEN);

            lv_label_set_text(frm_chat_btn_title_lbl, title);

            lv_group_focus_obj(frm_chat_text_ans);

            if(task_check_new_msg == NULL){

                xTaskCreatePinnedToCore(check_new_msg, "check_new_msg", 11000, NULL, 1, &task_check_new_msg, 1);
                Serial.println("task_check_new_msg created");
            }
            Serial.print("actual contact is ");
            Serial.println(actual_contact->getName());
        }
    }
}


void send_message(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    String enc_msg;
    char msg[200] = {'\0'};

    if(code == LV_EVENT_SHORT_CLICKED){

        lora_packet pkt;

        strcpy(pkt.sender, user_id);

        strcpy(pkt.destiny, actual_contact->getID().c_str());

        strcpy(pkt.status, "send");

        strftime(pkt.date_time, sizeof(pkt.date_time)," - %a, %b %d %Y %H:%M", &timeinfo);

        if(hasRadio){

            if(strcmp(lv_textarea_get_text(frm_chat_text_ans), "") != 0){

                strcpy(msg, lv_textarea_get_text(frm_chat_text_ans));

                uint8_t text_length = strlen(msg);
                uint8_t padded_len = ((text_length + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
                unsigned char ciphertext[padded_len];
                encrypt_text((unsigned char *)msg, (unsigned char *)user_key, text_length, ciphertext);
                memcpy(pkt.msg, ciphertext, padded_len);
                pkt.msg_size = padded_len;
                transmiting_packets.push_back(pkt);



                pkt.me = true;
                strcpy(pkt.msg, msg);
                Serial.print("Adding answer to ");
                Serial.println(pkt.destiny);
                Serial.println(pkt.msg);

                pthread_mutex_lock(&messages_mutex);
                    messages_list.addMessage(pkt);
                pthread_mutex_unlock(&messages_mutex);

                lv_textarea_set_text(frm_chat_text_ans, "");

            }
        }else
            Serial.println("Radio not configured");
    }
}



void copy_text(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * lbl = (lv_obj_t *)lv_event_get_user_data(e);
    if(code == LV_EVENT_LONG_PRESSED){
        lv_textarea_add_text(frm_chat_text_ans, lv_label_get_text(lbl));
    }
}


void check_new_msg(void * param){

    vector<lora_packet> caller_msg;

    uint32_t actual_count = 0;
    lv_obj_t * btn = NULL, * lbl = NULL;
    char date[60] = {'\0'};
    char name[100] = {'\0'};

    pthread_mutex_lock(&lvgl_mutex);
    lv_obj_clean(frm_chat_list);
    pthread_mutex_unlock(&lvgl_mutex);
    while(true){

        pthread_mutex_lock(&messages_mutex);
        caller_msg = messages_list.getMessages(actual_contact->getID().c_str());
        pthread_mutex_unlock(&messages_mutex);

        actual_count = caller_msg.size();

        if(actual_count > msg_count){
            Serial.println("new messages");

            for(uint32_t i = msg_count; i < actual_count; i++){


                if(caller_msg[i].me){

                    strcpy(name, "Me");
                    strcat(name, caller_msg[i].date_time);
                    Serial.println(name);

                    pthread_mutex_lock(&lvgl_mutex);
                    lv_list_add_text(frm_chat_list, name);
                    pthread_mutex_unlock(&lvgl_mutex);
                }else{


                    if(strcmp(caller_msg[i].status, "recv") == 0){

                        pthread_mutex_lock(&lvgl_mutex);
                        btn = lv_list_add_btn(frm_chat_list, LV_SYMBOL_OK, "");
                        pthread_mutex_unlock(&lvgl_mutex);
                    }else{

                        strcpy(name, actual_contact->getName().c_str());
                        strcat(name, caller_msg[i].date_time);

                        pthread_mutex_lock(&lvgl_mutex);
                        lv_list_add_text(frm_chat_list, name);
                        pthread_mutex_unlock(&lvgl_mutex);
                        Serial.println(name);
                    }
                }
                Serial.println(caller_msg[i].msg);



                if(strcmp(caller_msg[i].status, "recv") != 0){

                    pthread_mutex_lock(&lvgl_mutex);

                    btn = lv_list_add_btn(frm_chat_list, NULL, caller_msg[i].msg);

                    lv_obj_add_event_cb(btn, copy_text, LV_EVENT_LONG_PRESSED, lv_obj_get_child(btn, 0));
                    pthread_mutex_unlock(&lvgl_mutex);
                }

                pthread_mutex_lock(&lvgl_mutex);
                lbl = lv_obj_get_child(btn, 0);
                pthread_mutex_unlock(&lvgl_mutex);

                if(strcmp(caller_msg[i].status, "recv") != 0){
                    pthread_mutex_lock(&lvgl_mutex);

                    lv_obj_set_style_text_font(lbl, &ubuntu, LV_PART_MAIN | LV_STATE_DEFAULT);

                    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
                    pthread_mutex_unlock(&lvgl_mutex);
                }

                pthread_mutex_lock(&lvgl_mutex);
                lv_obj_scroll_to_view(btn, LV_ANIM_OFF);
                pthread_mutex_unlock(&lvgl_mutex);
            }

            msg_count = actual_count;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}


void add_contact(lv_event_t * e){
    const char * name, * id, * key;
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){

        id = lv_textarea_get_text(frm_add_contact_textarea_id);
        name = lv_textarea_get_text(frm_add_contact_textarea_name);
        key = lv_textarea_get_text(frm_add_contact_textarea_key);

        if(strcmp(name, "") != 0 && strcmp(id, "") != 0){

            Contact c = Contact(name, id);
            c.setKey(key);

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
        }else
            Serial.println("Name, ID or KEY cannot be emty");
    }
}


void del_contact(lv_event_t * e){
    char id[7] = {'\u0000'};
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){

        strcpy(id, lv_textarea_get_text(frm_edit_text_ID));

        if(strcmp(id, "") != 0){

            Contact * c = contacts_list.getContactByID(id);
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
        Serial.println("ID is empty");
}


void generateID(lv_event_t * e){
    uint8_t size = (int)lv_event_get_user_data(e);
    lv_textarea_set_text(frm_settings_id, generate_ID(size).c_str());
}


void generateKEY(lv_event_t * e){
    uint8_t size = (int)lv_event_get_user_data(e);
    lv_textarea_set_text(frm_settings_key, generate_ID(size).c_str());
}



void DX(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_VALUE_CHANGED){

        if(lv_obj_has_state(frm_settings_switch_dx, LV_STATE_CHECKED)){
            if(DXMode()){
                Serial.println("DX mode on");
                notification_list.add("DX mode", LV_SYMBOL_SETTINGS);
                isDX = true;
            }else{
                notification_list.add("DX mode failed", LV_SYMBOL_SETTINGS);
                Serial.println("DX mode failed");
            }
        }else{
            if(normalMode()){
                notification_list.add( "Normal mode", LV_SYMBOL_SETTINGS);
                Serial.println("DX mode off");
                isDX = false;
            }else{
                notification_list.add("Normal mode failed",LV_SYMBOL_SETTINGS);
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
    }
    vTaskDelete(task_date_time);
}
# 2489 "/home/rene/Documents/T-Deck-p/examples/T/T.ino"
void setDate(int yr, int month, int mday, int hr, int minute, int sec, int isDst){
    timeinfo.tm_year = yr - 1900;
    timeinfo.tm_mon = month-1;
    timeinfo.tm_mday = mday;
    timeinfo.tm_hour = hr;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = sec;
    timeinfo.tm_isdst = isDst;
    time_t t = mktime(&timeinfo);
    Serial.printf("Setting time: %s", asctime(&timeinfo));
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, NULL);
    notification_list.add("date & time updated", LV_SYMBOL_SETTINGS);
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

    strcpy(ui_primary_color_hex_text, lv_textarea_get_text(frm_settings_color));

    if(strcmp(ui_primary_color_hex_text, "") == 0)
        return;

    ui_primary_color = strtoul(ui_primary_color_hex_text, NULL, 16);

    lv_disp_t *dispp = lv_disp_get_default();

    lv_theme_t *theme = lv_theme_default_init(dispp, lv_color_hex(ui_primary_color), lv_palette_main(LV_PALETTE_RED), false, &lv_font_montserrat_14);

    lv_disp_set_theme(dispp, theme);
}



void update_frm_contacts_status(uint16_t index, bool in_range){

    if(index > contacts_list.size() - 1 || index < 0)
        return;

    pthread_mutex_lock(&lvgl_mutex);
    lv_obj_t * btn = lv_obj_get_child(frm_contacts_list, index);
    pthread_mutex_unlock(&lvgl_mutex);


    pthread_mutex_lock(&lvgl_mutex);
    lv_obj_t * obj_status = lv_obj_get_child(btn, 3);
    pthread_mutex_unlock(&lvgl_mutex);


    if(in_range){

        pthread_mutex_lock(&lvgl_mutex);
        lv_obj_set_style_bg_color(obj_status, lv_color_hex(0x00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
        pthread_mutex_unlock(&lvgl_mutex);
    }
    else{
        pthread_mutex_lock(&lvgl_mutex);
        lv_obj_set_style_bg_color(obj_status, lv_color_hex(0xaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
        pthread_mutex_unlock(&lvgl_mutex);
    }
}



void sendContactsStatusJson(const char * id, bool status){
    JsonDocument doc;
    std::string json;

    doc["command"] = "contact_status";
    doc["contact"]["id"] = id;
    doc["contact"]["status"] = status;

    serializeJson(doc, json);
    while(sendingJson){
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    pthread_mutex_lock(&send_json_mutex);
    sendJSON(json);
    pthread_mutex_unlock(&send_json_mutex);
}

void check_contacts_in_range(){
    Serial.println("========check_contacts_in_range========");

    activity(lv_color_hex(0x0095ff));

    contacts_list.check_inrange();

    for(uint32_t i = 0; i < contacts_list.size(); i++){

        update_frm_contacts_status(i, contacts_list.getList()[i].inrange);

        Serial.print(contacts_list.getList()[i].getName());
        Serial.println(contacts_list.getList()[i].inrange ? " is in range" : " is out of range");

        sendContactsStatusJson(contacts_list.getList()[i].getID().c_str(), contacts_list.getList()[i].inrange);
    }
    Serial.println("=======================================");
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







long mapv(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


uint32_t read_bat(){
    uint16_t v = analogRead(BOARD_BAT_ADC);
    double vBat = ((double)v / 4095.0) * 2.0 * 3.3 * (vRef / 1000.0);
    if (vBat > 4.2) {
        vBat = 4.2;
    }
    uint32_t batPercent = mapv(vBat * 100, 320, 420, 0, 100);
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
    string json;
    JsonDocument doc;

    while(true){
        itoa(p, pc, 10);
        strcpy(msg, pc);
        strcat(msg, "% ");
        strcat(msg, get_battery_icon(p));
        doc["command"] = "bat_level";
        doc["level"] = p;
        serializeJson(doc, json);
        pthread_mutex_lock(&lvgl_mutex);
        lv_label_set_text(frm_home_bat_lbl, msg);
        pthread_mutex_unlock(&lvgl_mutex);
        while(sendingJson){
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        while(parsing){
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        pthread_mutex_lock(&send_json_mutex);
        sendJSON(json);
        pthread_mutex_unlock(&send_json_mutex);
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
            lv_label_set_text(frm_wifi_login_title_lbl, "Connecting...");

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

            strcpy(connected_to, wifi_list[i].SSID);
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
            Serial.println("SSID deleted from connected nets");

            WiFi.disconnect(true);

            lv_obj_set_style_text_color(frm_settings_btn_wifi_lbl, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

            if(wifi_connected_nets.save()){
                Serial.println("connected nets list save failed");
            }
        }else{
            Serial.println("SSID not found in connected nets");
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

    lv_label_set_text(frm_wifi_connected_to_lbl, "Scanning...");



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

    vTaskDelete(task_wifi_scan);
}


void wifi_scan(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        xTaskCreatePinnedToCore(wifi_scan_task, "wifi_scan_task", 10000, NULL, 1, &task_wifi_scan, 1);
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

    wifi_con_info();
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

            wifi_auto_connect(NULL);
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
            Serial.printf("Total space: %lu MB\n", cardTotal);
            Serial.printf("Used space: %lu MB\n", cardUsed);
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

    if(WiFi.isConnected()){

            digitalWrite(BOARD_POWERON, HIGH);
            audio.setPinout(BOARD_I2S_BCK, BOARD_I2S_WS, BOARD_I2S_DOUT);
            audio.setVolume(10);
            audio.connecttohost("0n-80s.radionetz.de:8000/0n-70s.mp3");

            while (audio.isRunning()) {
                audio.loop();
            }
            audio.stopSong();


    }
    vTaskDelete(task_play_radio);
}


void taskplaySong(void *p) {
    if(xSemaphoreTake(xSemaphore, 100 / portTICK_PERIOD_MS) == pdTRUE){
        const char *path = "comp_up.mp3";
        digitalWrite(BOARD_POWERON, HIGH);
        audio.setPinout(BOARD_I2S_BCK, BOARD_I2S_WS, BOARD_I2S_DOUT);
        audio.setVolume(21);
        audio.connecttoFS(SPIFFS, path);
        Serial.printf("play %s\r\n", path);
        while (audio.isRunning()) {
            audio.loop();
        }
        audio.stopSong();
        xSemaphoreGive(xSemaphore);
    }
    vTaskDelete(NULL);
}

void notify_snd(){

}
# 3230 "/home/rene/Documents/T-Deck-p/examples/T/T.ino"
void show_especial(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(lv_obj_has_flag(kb, LV_OBJ_FLAG_HIDDEN))
            lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}


void activity(lv_color_t color){
    lv_obj_set_style_bg_color(frm_home_activity_led, color, LV_PART_MAIN | LV_STATE_DEFAULT);
}

const char * kbmap[] = {"á", "à", "ã", "â", "Á", "À", "Ã", "Â","\n",
                        "é", "ê", "í", "ó", "É", "Ê", "Í", "Ó","\n",
                        "ô", "õ", "ú", "ç", "Ô", "Õ", "Ú", "Ç",""};

const lv_btnmatrix_ctrl_t kbctrl[] = {
LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT,
LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT,
LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT,
LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT,
LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT,
LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT};

const char * brightness_levels = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10";


void setBrightness(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * roller = (lv_obj_t *)lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED){

        brightness = lv_roller_get_selected(roller);

        analogWrite(BOARD_BL_PIN, mapv(brightness, 0, 9, 100, 255));
    }
}

enum {
    LV_MENU_ITEM_BUILDER_VARIANT_1,
    LV_MENU_ITEM_BUILDER_VARIANT_2
};
typedef uint8_t lv_menu_builder_variant_t;






static lv_obj_t * create_text(lv_obj_t * parent, const char * icon, const char * txt,
                              lv_menu_builder_variant_t builder_variant)
{
    lv_obj_t * obj = lv_menu_cont_create(parent);

    lv_obj_t * img = NULL;
    lv_obj_t * label = NULL;

    if(icon) {
        img = lv_img_create(obj);
        lv_img_set_src(img, icon);
    }

    if(txt) {
        label = lv_label_create(obj);
        lv_label_set_text(label, txt);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(label, 1);
    }

    if(builder_variant == LV_MENU_ITEM_BUILDER_VARIANT_2 && icon && txt) {
        lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
        lv_obj_swap(img, label);
    }

    return obj;
}

void wifi_con_info(){
    char buf[100] = {'\0'};
    char num[10] = {'\0'};

    if(WiFi.isConnected() && last_wifi_con < wifi_connected_nets.list.size()){
        strcpy(buf, "SSID: ");
        strcat(buf, wifi_connected_nets.list[last_wifi_con].SSID);

        lv_label_set_text(frm_settings_wifi_ssid, buf);

        strcpy(buf, "Auth: ");
        strcat(buf, wifi_auth_mode_to_str(wifi_connected_nets.list[last_wifi_con].auth_type));

        lv_label_set_text(frm_settings_wifi_auth, buf);

        itoa(wifi_connected_nets.list[last_wifi_con].ch, num, 10);
        strcpy(buf, "Channel: ");
        strcat(buf, num);

        lv_label_set_text(frm_settings_wifi_ch, buf);

        itoa(WiFi.RSSI(last_wifi_con), num, 10);
        strcpy(buf, "RSSI: ");
        strcat(buf, num);

        lv_label_set_text(frm_settings_wifi_rssi, buf);

        strcpy(buf, "MAC: ");
        strcat(buf, WiFi.macAddress().c_str());

        lv_label_set_text(frm_settings_wifi_mac, buf);

        strcpy(buf, "IP: ");
        strcat(buf, WiFi.localIP().toString().c_str());

        lv_label_set_text(frm_settings_wifi_ip, buf);

        strcpy(buf, "Subnet: ");
        strcat(buf, WiFi.subnetMask().toString().c_str());

        lv_label_set_text(frm_settings_wifi_mask, buf);

        strcpy(buf, "Gateway: ");
        strcat(buf, WiFi.gatewayIP().toString().c_str());

        lv_label_set_text(frm_settings_wifi_gateway, buf);

        strcpy(buf, "DNS: ");
        strcat(buf, WiFi.dnsIP().toString().c_str());

        lv_label_set_text(frm_settings_wifi_dns, buf);

    }else{

        lv_label_set_text(frm_settings_wifi_ssid, "Disconnected");
        lv_label_set_text(frm_settings_wifi_auth, "");
        lv_label_set_text(frm_settings_wifi_ch, "");
        lv_label_set_text(frm_settings_wifi_rssi, "");
        lv_label_set_text(frm_settings_wifi_mac, "");
        lv_label_set_text(frm_settings_wifi_ip, "");
        lv_label_set_text(frm_settings_wifi_mask, "");
        lv_label_set_text(frm_settings_wifi_gateway, "");
        lv_label_set_text(frm_settings_wifi_dns, "");

    }
}

void tz_event(lv_event_t * e){
    lv_obj_t * tz = (lv_obj_t*)lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    int8_t err = 0;

    if(code == LV_EVENT_VALUE_CHANGED){
        strcpy(time_zone, tz_list(lv_dropdown_get_selected(tz)));
        Serial.print(time_zone);
        setTZ();
    }
}


void ui(){

    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_color_hex(ui_primary_color), lv_palette_main(LV_PALETTE_RED), false, &lv_font_montserrat_16);
    lv_disp_set_theme(dispp, theme);


    frm_home = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_home, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(frm_home, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_color(frm_home, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(frm_home, 0, 0 ) ;
    lv_obj_set_style_border_width(frm_home, 0, 0);


    lv_obj_set_style_bg_img_src(frm_home, &bg2, LV_PART_MAIN | LV_STATE_DEFAULT);


    frm_home_symbol_lbl = lv_label_create(frm_home);
    lv_obj_align(frm_home_symbol_lbl, LV_ALIGN_TOP_LEFT, -10, -10);
    lv_obj_set_style_text_color(frm_home_symbol_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);

    frm_home_title_lbl = lv_label_create(frm_home);
    lv_obj_set_style_text_font(frm_home_title_lbl, &ubuntu, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(frm_home_title_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(frm_home_title_lbl, LV_ALIGN_TOP_LEFT, 7, -10);
    lv_obj_set_size(frm_home_title_lbl, 200, 30);
    lv_label_set_long_mode(frm_home_title_lbl, LV_LABEL_LONG_SCROLL);


    frm_home_bat_lbl = lv_label_create(frm_home);
    lv_label_set_text(frm_home_bat_lbl, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(frm_home_bat_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(frm_home_bat_lbl, LV_ALIGN_TOP_RIGHT, 5, -10);


    frm_home_wifi_lbl = lv_label_create(frm_home);
    lv_label_set_text(frm_home_wifi_lbl, "");
    lv_obj_set_style_text_color(frm_home_wifi_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(frm_home_wifi_lbl, LV_ALIGN_TOP_RIGHT, -55, -10);


    frm_home_rssi_chart = lv_chart_create(frm_home);
    lv_obj_set_size(frm_home_rssi_chart, 90, 55);
    lv_obj_align(frm_home_rssi_chart, LV_ALIGN_OUT_TOP_LEFT, 0, 15);
    lv_chart_set_type(frm_home_rssi_chart, LV_CHART_TYPE_LINE);
    frm_home_rssi_series = lv_chart_add_series(frm_home_rssi_chart, lv_palette_main(LV_PALETTE_LIGHT_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    frm_home_snr_series = lv_chart_add_series(frm_home_rssi_chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_SECONDARY_Y);
    lv_chart_set_range(frm_home_rssi_chart, LV_CHART_AXIS_PRIMARY_Y, -147, 0);
    lv_chart_set_range(frm_home_rssi_chart, LV_CHART_AXIS_SECONDARY_Y, 0, 20);
    lv_chart_set_next_value(frm_home_rssi_chart, frm_home_rssi_series, -147);
    lv_chart_set_next_value(frm_home_rssi_chart, frm_home_snr_series, 0);
    lv_chart_refresh(frm_home_rssi_chart);


    frm_home_frm_date_time = lv_obj_create(frm_home);
    lv_obj_set_size(frm_home_frm_date_time, 90, 55);
    lv_obj_align(frm_home_frm_date_time, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_clear_flag(frm_home_frm_date_time, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(frm_home_frm_date_time, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(frm_home_frm_date_time, 64, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(frm_home_frm_date_time, lv_color_hex(0x151515), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(frm_home_frm_date_time, 0, LV_PART_MAIN | LV_STATE_DEFAULT);


    frm_home_date_lbl = lv_label_create(frm_home_frm_date_time);
    lv_obj_set_style_text_color(frm_home_date_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(frm_home_date_lbl, "-");
    lv_obj_align(frm_home_date_lbl, LV_ALIGN_TOP_MID, 0, -10);


    frm_home_time_lbl = lv_label_create(frm_home_frm_date_time);
    lv_obj_set_style_text_font(frm_home_time_lbl, &clocknum, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(frm_home_time_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(frm_home_time_lbl, "00:00");
    lv_obj_align(frm_home_time_lbl, LV_ALIGN_TOP_MID, 0, 10);


    frm_home_btn_contacts = lv_btn_create(frm_home);
    lv_obj_set_size(frm_home_btn_contacts, 50, 35);
    lv_obj_set_align(frm_home_btn_contacts, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_add_event_cb(frm_home_btn_contacts, show_contacts_form, LV_EVENT_SHORT_CLICKED, NULL);


    frm_home_contacts_img = lv_img_create(frm_home_btn_contacts);
    lv_img_set_src(frm_home_contacts_img, &icon_lora2);
    lv_obj_set_align(frm_home_contacts_img, LV_ALIGN_CENTER);


    frm_home_btn_settings = lv_btn_create(frm_home);
    lv_obj_set_size(frm_home_btn_settings, 20, 20);
    lv_obj_align(frm_home_btn_settings, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(frm_home_btn_settings, show_settings, LV_EVENT_SHORT_CLICKED, NULL);

    frm_home_btn_settings_lbl = lv_label_create(frm_home_btn_settings);
    lv_label_set_text(frm_home_btn_settings_lbl, LV_SYMBOL_SETTINGS);
    lv_obj_set_align(frm_home_btn_settings_lbl, LV_ALIGN_CENTER);


    btn_test = lv_btn_create(frm_home);
    lv_obj_set_size(btn_test, 50, 20);
    lv_obj_set_align(btn_test, LV_ALIGN_BOTTOM_RIGHT);

    lbl_btn_test = lv_label_create(btn_test);
    lv_obj_set_style_text_font(lbl_btn_test, &ubuntu, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(lbl_btn_test, "Ping");
    lv_obj_align(lbl_btn_test, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_test, ping, LV_EVENT_SHORT_CLICKED, NULL);


    frm_home_activity_led = lv_obj_create(frm_home);
    lv_obj_set_size(frm_home_activity_led, 7, 15);
    lv_obj_align(frm_home_activity_led, LV_ALIGN_TOP_RIGHT, -75, -10);
    lv_obj_clear_flag(frm_home_activity_led, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(frm_home_activity_led, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(frm_home_activity_led, 0, LV_PART_MAIN | LV_STATE_DEFAULT);


    frm_contacts = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_contacts, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(frm_contacts, LV_OBJ_FLAG_SCROLLABLE);


    frm_contatcs_btn_title = lv_btn_create(frm_contacts);
    lv_obj_set_size(frm_contatcs_btn_title, 200, 20);
    lv_obj_align(frm_contatcs_btn_title, LV_ALIGN_TOP_LEFT, -15, -15);

    frm_contatcs_btn_title_lbl = lv_label_create(frm_contatcs_btn_title);
    lv_label_set_text(frm_contatcs_btn_title_lbl, "Contacts");
    lv_obj_set_align(frm_contatcs_btn_title_lbl, LV_ALIGN_LEFT_MID);


    frm_contacts_btn_back = lv_btn_create(frm_contacts);
    lv_obj_set_size(frm_contacts_btn_back, 50, 20);
    lv_obj_align(frm_contacts_btn_back, LV_ALIGN_TOP_RIGHT, 15, -15);

    frm_contacts_btn_back_lbl = lv_label_create(frm_contacts_btn_back);
    lv_label_set_text(frm_contacts_btn_back_lbl, "Back");
    lv_obj_set_align(frm_contacts_btn_back_lbl, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(frm_contacts_btn_back, hide_contacts_frm, LV_EVENT_SHORT_CLICKED, NULL);


    frm_contacts_list = lv_list_create(frm_contacts);
    lv_obj_set_size(frm_contacts_list, LV_HOR_RES, 220);
    lv_obj_align(frm_contacts_list, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_border_opa(frm_contacts_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(frm_contacts_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    refresh_contact_list();


    frm_contacts_btn_add = lv_btn_create(frm_contacts);
    lv_obj_set_size(frm_contacts_btn_add, 50, 20);
    lv_obj_align(frm_contacts_btn_add, LV_ALIGN_BOTTOM_RIGHT, 15, 15);

    frm_contacts_btn_add_lbl = lv_label_create(frm_contacts_btn_add);
    lv_label_set_text(frm_contacts_btn_add_lbl, "Add");
    lv_obj_set_align(frm_contacts_btn_add_lbl, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(frm_contacts_btn_add, show_add_contacts_frm, LV_EVENT_SHORT_CLICKED, NULL);


    lv_obj_add_flag(frm_contacts, LV_OBJ_FLAG_HIDDEN);


    frm_add_contact = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_add_contact, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(frm_add_contact, LV_OBJ_FLAG_SCROLLABLE);


    frm_add_contacts_btn_title = lv_btn_create(frm_add_contact);
    lv_obj_set_size(frm_add_contacts_btn_title, 200, 20);
    lv_obj_align(frm_add_contacts_btn_title, LV_ALIGN_TOP_LEFT, -15, -15);

    frm_add_contacts_btn_title_lbl = lv_label_create(frm_add_contacts_btn_title);
    lv_label_set_text(frm_add_contacts_btn_title_lbl, "Add contact");
    lv_obj_set_align(frm_add_contacts_btn_title_lbl, LV_ALIGN_LEFT_MID);


    frm_add_contact_textarea_id = lv_textarea_create(frm_add_contact);
    lv_obj_set_size(frm_add_contact_textarea_id, 150, 30);
    lv_obj_align(frm_add_contact_textarea_id, LV_ALIGN_TOP_LEFT, 0, 25);
    lv_textarea_set_placeholder_text(frm_add_contact_textarea_id, "ID");


    frm_add_contact_textarea_name = lv_textarea_create(frm_add_contact);
    lv_obj_set_size(frm_add_contact_textarea_name, 240, 30);
    lv_obj_align(frm_add_contact_textarea_name, LV_ALIGN_TOP_LEFT, 0, 60);
    lv_textarea_set_placeholder_text(frm_add_contact_textarea_name, "Name");


    frm_add_contact_textarea_key = lv_textarea_create(frm_add_contact);
    lv_obj_set_size(frm_add_contact_textarea_key, 240, 30);
    lv_obj_align(frm_add_contact_textarea_key, LV_ALIGN_TOP_LEFT, 0, 95);
    lv_textarea_set_placeholder_text(frm_add_contact_textarea_key, "KEY");


    frm_add_contacts_btn_add = lv_btn_create(frm_add_contact);
    lv_obj_set_size(frm_add_contacts_btn_add, 50, 20);
    lv_obj_align(frm_add_contacts_btn_add, LV_ALIGN_BOTTOM_RIGHT, 15, 15);

    frm_add_contacts_btn_add_lbl = lv_label_create(frm_add_contacts_btn_add);
    lv_label_set_text(frm_add_contacts_btn_add_lbl, "Add");
    lv_obj_set_align(frm_add_contacts_btn_add_lbl, LV_ALIGN_CENTER);

    lv_obj_add_event_cb(frm_add_contacts_btn_add, add_contact, LV_EVENT_SHORT_CLICKED, NULL);


    btn_frm_add_contatcs_close = lv_btn_create(frm_add_contact);
    lv_obj_set_size(btn_frm_add_contatcs_close, 50, 20);
    lv_obj_align(btn_frm_add_contatcs_close, LV_ALIGN_TOP_RIGHT, 15, -15);

    lbl_btn_frm_add_contacts_close = lv_label_create(btn_frm_add_contatcs_close);
    lv_label_set_text(lbl_btn_frm_add_contacts_close, "Back");
    lv_obj_align(lbl_btn_frm_add_contacts_close, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_frm_add_contatcs_close, hide_add_contacts_frm, LV_EVENT_SHORT_CLICKED, NULL);


    lv_obj_add_flag(frm_add_contact, LV_OBJ_FLAG_HIDDEN);


    frm_edit_contacts = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_edit_contacts, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(frm_edit_contacts, LV_OBJ_FLAG_SCROLLABLE);


    frm_edit_btn_title = lv_btn_create(frm_edit_contacts);
    lv_obj_set_size(frm_edit_btn_title, 200, 20);
    lv_obj_align(frm_edit_btn_title, LV_ALIGN_TOP_LEFT, -15, -15);

    frm_edit_btn_title_lbl = lv_label_create(frm_edit_btn_title);
    lv_label_set_text(frm_edit_btn_title_lbl, "Edit contact");
    lv_obj_set_align(frm_edit_btn_title_lbl, LV_ALIGN_LEFT_MID);


    frm_edit_btn_back = lv_btn_create(frm_edit_contacts);
    lv_obj_set_size(frm_edit_btn_back, 50, 20);
    lv_obj_align(frm_edit_btn_back, LV_ALIGN_TOP_RIGHT, 15, -15);

    frm_edit_btn_back_lbl = lv_label_create(frm_edit_btn_back);
    lv_label_set_text(frm_edit_btn_back_lbl, "Back");
    lv_obj_set_align(frm_edit_btn_back_lbl, LV_ALIGN_CENTER);

    lv_obj_add_event_cb(frm_edit_btn_back, hide_edit_contacts, LV_EVENT_SHORT_CLICKED, NULL);


    frm_edit_btn_del = lv_btn_create(frm_edit_contacts);
    lv_obj_set_size(frm_edit_btn_del, 50, 20);
    lv_obj_align(frm_edit_btn_del, LV_ALIGN_BOTTOM_RIGHT, 15, 15);
    lv_obj_add_event_cb(frm_edit_btn_del, del_contact, LV_EVENT_SHORT_CLICKED, NULL);

    frm_edit_btn_del_lbl = lv_label_create(frm_edit_btn_del);
    lv_label_set_text(frm_edit_btn_del_lbl, "Del");
    lv_obj_set_align(frm_edit_btn_del_lbl, LV_ALIGN_CENTER);


    frm_edit_text_ID = lv_textarea_create(frm_edit_contacts);
    lv_obj_set_size(frm_edit_text_ID, 150, 30);
    lv_obj_align(frm_edit_text_ID, LV_ALIGN_TOP_LEFT, 0, 25);
    lv_textarea_set_placeholder_text(frm_edit_text_ID, "ID");


    frm_edit_text_name = lv_textarea_create(frm_edit_contacts);
    lv_obj_set_size(frm_edit_text_name, 240, 30);
    lv_obj_align(frm_edit_text_name, LV_ALIGN_TOP_LEFT, 0, 60);
    lv_textarea_set_placeholder_text(frm_edit_text_name, "Name");


    frm_edit_text_key = lv_textarea_create(frm_edit_contacts);
    lv_obj_set_size(frm_edit_text_key, 240, 30);
    lv_obj_align(frm_edit_text_key, LV_ALIGN_TOP_LEFT, 0, 95);
    lv_textarea_set_placeholder_text(frm_edit_text_key, "KEY");

    lv_obj_add_flag(frm_edit_contacts, LV_OBJ_FLAG_HIDDEN);


    frm_chat = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_chat, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(frm_chat, LV_OBJ_FLAG_SCROLLABLE);


    frm_chat_btn_title = lv_btn_create(frm_chat);
    lv_obj_set_size(frm_chat_btn_title, 200, 20);
    lv_obj_align(frm_chat_btn_title, LV_ALIGN_OUT_TOP_LEFT, -15, -15);

    frm_chat_btn_title_lbl = lv_label_create(frm_chat_btn_title);
    lv_label_set_text(frm_chat_btn_title_lbl, "chat with ");
    lv_obj_set_align(frm_chat_btn_title_lbl, LV_ALIGN_LEFT_MID);


    frm_chat_btn_back = lv_btn_create(frm_chat);
    lv_obj_set_size(frm_chat_btn_back, 50, 20);
    lv_obj_align(frm_chat_btn_back, LV_ALIGN_TOP_RIGHT, 15, -15);

    frm_chat_btn_back_lbl = lv_label_create(frm_chat_btn_back);
    lv_label_set_text(frm_chat_btn_back_lbl, "Back");
    lv_obj_set_align(frm_chat_btn_back_lbl, LV_ALIGN_CENTER);

    lv_obj_add_event_cb(frm_chat_btn_back, hide_chat, LV_EVENT_SHORT_CLICKED, NULL);


    frm_chat_list = lv_list_create(frm_chat);
    lv_obj_set_size(frm_chat_list, 320, 170);
    lv_obj_align(frm_chat_list, LV_ALIGN_TOP_MID, 0, 5);


    frm_chat_text_ans = lv_textarea_create(frm_chat);
    lv_obj_set_size(frm_chat_text_ans, 260, 50);
    lv_obj_align(frm_chat_text_ans, LV_ALIGN_BOTTOM_LEFT, -15, 15);
    lv_textarea_set_max_length(frm_chat_text_ans, 120);
    lv_textarea_set_placeholder_text(frm_chat_text_ans, "Answer");
    lv_obj_set_style_text_font(frm_chat_text_ans, &ubuntu, LV_PART_MAIN | LV_STATE_DEFAULT);


    frm_chat_btn_send = lv_btn_create(frm_chat);
    lv_obj_set_size(frm_chat_btn_send, 50, 50);
    lv_obj_align(frm_chat_btn_send, LV_ALIGN_BOTTOM_RIGHT, 15, 15);
    lv_obj_add_event_cb(frm_chat_btn_send, send_message, LV_EVENT_SHORT_CLICKED, NULL);

    frm_chat_btn_send_lbl = lv_label_create(frm_chat_btn_send);
    lv_label_set_text(frm_chat_btn_send_lbl, "Send");
    lv_obj_set_align(frm_chat_btn_send_lbl, LV_ALIGN_CENTER);



    frm_chat_group = lv_group_create();
    lv_group_add_obj(frm_chat_group, frm_chat_btn_back);
    lv_group_add_obj(frm_chat_group, frm_chat_btn_send);
    lv_group_add_obj(frm_chat_group, frm_chat_btn_back_lbl);
    lv_group_add_obj(frm_chat_group, frm_chat_btn_send_lbl);
    lv_group_add_obj(frm_chat_group, frm_chat_btn_title);
    lv_group_add_obj(frm_chat_group, frm_chat_btn_title_lbl);
    lv_group_add_obj(frm_chat_group, frm_chat_list);


    kb = lv_keyboard_create(frm_chat);
    lv_obj_set_style_text_font(kb, &ubuntu, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_SPECIAL);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_SPECIAL, kbmap, kbctrl);
    lv_keyboard_set_textarea(kb, frm_chat_text_ans);
    lv_obj_set_size(kb, 200, 100);
    lv_obj_align(kb, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0xcccccc), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);


    frm_chat_btn_kb = lv_btn_create(frm_chat);
    lv_obj_set_size(frm_chat_btn_kb, 30, 20);
    lv_obj_align(frm_chat_btn_kb, LV_ALIGN_TOP_RIGHT, -50, -15);
    lv_obj_add_event_cb(frm_chat_btn_kb, show_especial, LV_EVENT_SHORT_CLICKED, NULL);

    frm_chat_btn_kb_lbl = lv_label_create(frm_chat_btn_kb);

    lv_label_set_text(frm_chat_btn_kb_lbl, LV_SYMBOL_KEYBOARD);
    lv_obj_set_align(frm_chat_btn_kb_lbl, LV_ALIGN_CENTER);

    lv_obj_add_flag(frm_chat, LV_OBJ_FLAG_HIDDEN);



    frm_settings = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_settings, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(frm_settings, LV_OBJ_FLAG_SCROLLABLE);


    frm_settings_menu = lv_menu_create(frm_settings);
    lv_color_t bg_color = lv_obj_get_style_bg_color(frm_settings_menu, 0);
    if(lv_color_brightness(bg_color) > 127) {
        lv_obj_set_style_bg_color(frm_settings_menu, lv_color_darken(lv_obj_get_style_bg_color(frm_settings_menu, 0), 10), 0);
    }
    else {
        lv_obj_set_style_bg_color(frm_settings_menu, lv_color_darken(lv_obj_get_style_bg_color(frm_settings_menu, 0), 50), 0);
    }

    lv_menu_set_mode_root_back_btn(frm_settings_menu, LV_MENU_ROOT_BACK_BTN_ENABLED);
    lv_obj_add_event_cb(frm_settings_menu, hide_settings, LV_EVENT_SHORT_CLICKED, frm_settings_menu);
    lv_obj_set_size(frm_settings_menu, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_center(frm_settings_menu);


    frm_settings_id_page = lv_menu_page_create(frm_settings_menu, NULL);
    lv_obj_set_style_pad_ver(frm_settings_id_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(frm_settings_menu), 0), 0);
    lv_menu_separator_create(frm_settings_id_page);
    frm_settings_id_section = lv_menu_section_create(frm_settings_id_page);
    lv_obj_clear_flag(frm_settings_id_page, LV_OBJ_FLAG_SCROLLABLE);

    frm_settings_obj_id = lv_obj_create(frm_settings_id_section);
    lv_obj_set_size(frm_settings_obj_id, 230, 230);


    frm_settings_name = lv_textarea_create(frm_settings_obj_id);
    lv_textarea_set_one_line(frm_settings_name, true);
    lv_obj_set_size(frm_settings_name, 180, 30);
    lv_textarea_set_placeholder_text(frm_settings_name, "Name");
    lv_obj_align(frm_settings_name, LV_ALIGN_OUT_TOP_LEFT, 0, -10);


    frm_settings_id = lv_textarea_create(frm_settings_obj_id);
    lv_textarea_set_one_line(frm_settings_id, true);
    lv_obj_set_size(frm_settings_id, 90, 30);
    lv_textarea_set_max_length(frm_settings_id, 6);
    lv_textarea_set_placeholder_text(frm_settings_id, "ID");
    lv_obj_align(frm_settings_id, LV_ALIGN_TOP_LEFT, 0, 20);


    frm_settings_btn_generate = lv_btn_create(frm_settings_obj_id);
    lv_obj_set_size(frm_settings_btn_generate, 80, 20);
    lv_obj_align(frm_settings_btn_generate, LV_ALIGN_TOP_LEFT, 100, 25);
    lv_obj_add_event_cb(frm_settings_btn_generate, generateID, LV_EVENT_SHORT_CLICKED, (void*)6);

    frm_settings_btn_generate_lbl = lv_label_create(frm_settings_btn_generate);
    lv_label_set_text(frm_settings_btn_generate_lbl, "Generate");
    lv_obj_set_align(frm_settings_btn_generate_lbl, LV_ALIGN_CENTER);


    frm_settings_key = lv_textarea_create(frm_settings_obj_id);
    lv_textarea_set_one_line(frm_settings_key, true);
    lv_obj_set_size(frm_settings_key, 180, 30);
    lv_textarea_set_max_length(frm_settings_key, 16);
    lv_textarea_set_placeholder_text(frm_settings_key, "KEY");
    lv_obj_align(frm_settings_key, LV_ALIGN_TOP_LEFT, 0, 50);


    frm_settings_btn_generate_key = lv_btn_create(frm_settings_obj_id);
    lv_obj_set_size(frm_settings_btn_generate_key, 80, 20);
    lv_obj_align(frm_settings_btn_generate_key, LV_ALIGN_TOP_LEFT, 0, 85);
    lv_obj_add_event_cb(frm_settings_btn_generate_key, generateKEY, LV_EVENT_SHORT_CLICKED, (void*)16);

    frm_settings_btn_generate_key_lbl = lv_label_create(frm_settings_btn_generate_key);
    lv_label_set_text(frm_settings_btn_generate_key_lbl, "Generate");
    lv_obj_set_align(frm_settings_btn_generate_key_lbl, LV_ALIGN_CENTER);


    frm_settings_admin_password = lv_textarea_create(frm_settings_obj_id);
    lv_textarea_set_placeholder_text(frm_settings_admin_password, "adm pass");
    lv_textarea_set_text(frm_settings_admin_password, http_admin_pass);
    lv_textarea_set_max_length(frm_settings_admin_password, 40);
    lv_obj_set_size(frm_settings_admin_password, 180, 30);
    lv_obj_align(frm_settings_admin_password, LV_ALIGN_TOP_LEFT, 0, 115);


    frm_settings_dx_section;
    frm_settings_dx_page = lv_menu_page_create(frm_settings_menu, NULL);
    lv_obj_set_style_pad_hor(frm_settings_dx_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(frm_settings_menu), 0), 0);
    lv_menu_separator_create(frm_settings_dx_page);
    frm_settings_dx_section = lv_menu_section_create(frm_settings_dx_page);

    frm_settings_obj_dx = lv_obj_create(frm_settings_dx_section);
    lv_obj_set_size(frm_settings_obj_dx, 230, 230);


    frm_settings_switch_dx = lv_switch_create(frm_settings_obj_dx);
    lv_obj_align(frm_settings_switch_dx, LV_ALIGN_OUT_TOP_LEFT, 30, -10);
    lv_obj_add_event_cb(frm_settings_switch_dx, DX, LV_EVENT_VALUE_CHANGED, NULL);

    frm_settings_switch_dx_lbl = lv_label_create(frm_settings_obj_dx);
    lv_label_set_text(frm_settings_switch_dx_lbl, "DX");
    lv_obj_align(frm_settings_switch_dx_lbl, LV_ALIGN_TOP_LEFT, 0, -5);


    lv_obj_t * frm_settings_wifi_section;
    lv_obj_t * frm_settings_wifi_page = lv_menu_page_create(frm_settings_menu, NULL);
    lv_obj_set_style_pad_hor(frm_settings_wifi_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(frm_settings_menu), 0), 0);
    lv_menu_separator_create(frm_settings_wifi_page);
    frm_settings_wifi_section = lv_menu_section_create(frm_settings_wifi_page);

    lv_obj_t * frm_settings_obj_wifi = lv_obj_create(frm_settings_wifi_section);
    lv_obj_set_size(frm_settings_obj_wifi, 230, 230);


    frm_settings_btn_wifi = lv_btn_create(frm_settings_obj_wifi);
    lv_obj_set_size(frm_settings_btn_wifi, 30, 30);
    lv_obj_align(frm_settings_btn_wifi, LV_ALIGN_TOP_LEFT, 0, -10);
    lv_obj_add_event_cb(frm_settings_btn_wifi, wifi_toggle, LV_EVENT_SHORT_CLICKED, NULL);

    frm_settings_btn_wifi_lbl = lv_label_create(frm_settings_btn_wifi);
    lv_label_set_text(frm_settings_btn_wifi_lbl, LV_SYMBOL_WIFI);
    lv_obj_set_align(frm_settings_btn_wifi_lbl, LV_ALIGN_CENTER);


    frm_settings_btn_wifi_conf = lv_btn_create(frm_settings_obj_wifi);
    lv_obj_set_size(frm_settings_btn_wifi_conf, 80, 30);
    lv_obj_align(frm_settings_btn_wifi_conf, LV_ALIGN_TOP_LEFT, 40, -10);
    lv_obj_add_event_cb(frm_settings_btn_wifi_conf, show_wifi, LV_EVENT_SHORT_CLICKED, NULL);

    frm_settings_btn_wifi_conf_lbl = lv_label_create(frm_settings_btn_wifi_conf);
    lv_label_set_text(frm_settings_btn_wifi_conf_lbl, "Configure");
    lv_obj_set_align(frm_settings_btn_wifi_conf_lbl, LV_ALIGN_CENTER);


    frm_settings_wifi_http_btn = lv_btn_create(frm_settings_obj_wifi);
    lv_obj_set_size(frm_settings_wifi_http_btn, 50, 30);
    lv_obj_align(frm_settings_wifi_http_btn, LV_ALIGN_TOP_LEFT, 130, -10);
    lv_obj_add_flag(frm_settings_wifi_http_btn, LV_OBJ_FLAG_HIDDEN);

    frm_settings_wifi_http_label = lv_label_create(frm_settings_wifi_http_btn);
    lv_label_set_text(frm_settings_wifi_http_label, "https");
    lv_obj_set_align(frm_settings_wifi_http_label, LV_ALIGN_CENTER);


    frm_settings_wifi_ssid = lv_label_create(frm_settings_obj_wifi);
    lv_label_set_text(frm_settings_wifi_ssid, "SSID: ");
    lv_obj_align(frm_settings_wifi_ssid, LV_ALIGN_TOP_LEFT, 0, 30);

    frm_settings_wifi_auth = lv_label_create(frm_settings_obj_wifi);
    lv_label_set_text(frm_settings_wifi_auth, "auth: ");
    lv_obj_align(frm_settings_wifi_auth, LV_ALIGN_TOP_LEFT, 0, 50);

    frm_settings_wifi_ch = lv_label_create(frm_settings_obj_wifi);
    lv_label_set_text(frm_settings_wifi_ch, "Channel: ");
    lv_obj_align(frm_settings_wifi_ch, LV_ALIGN_TOP_LEFT, 0, 70);

    frm_settings_wifi_rssi = lv_label_create(frm_settings_obj_wifi);
    lv_label_set_text(frm_settings_wifi_rssi, "RSSI: ");
    lv_obj_align(frm_settings_wifi_rssi, LV_ALIGN_TOP_LEFT, 0, 90);

    frm_settings_wifi_mac = lv_label_create(frm_settings_obj_wifi);
    lv_label_set_text(frm_settings_wifi_mac, "Mac: ");
    lv_obj_align(frm_settings_wifi_mac, LV_ALIGN_TOP_LEFT, 0, 110);

    frm_settings_wifi_ip = lv_label_create(frm_settings_obj_wifi);
    lv_label_set_text(frm_settings_wifi_ip, "IP: ");
    lv_obj_align(frm_settings_wifi_ip, LV_ALIGN_TOP_LEFT, 0, 130);

    frm_settings_wifi_mask = lv_label_create(frm_settings_obj_wifi);
    lv_label_set_text(frm_settings_wifi_mask, "Mask: ");
    lv_obj_align(frm_settings_wifi_mask, LV_ALIGN_TOP_LEFT, 0, 150);

    frm_settings_wifi_gateway = lv_label_create(frm_settings_obj_wifi);
    lv_label_set_text(frm_settings_wifi_gateway, "gateway: ");
    lv_obj_align(frm_settings_wifi_gateway, LV_ALIGN_TOP_LEFT, 0, 170);

    frm_settings_wifi_dns = lv_label_create(frm_settings_obj_wifi);
    lv_label_set_text(frm_settings_wifi_dns, "DNS: ");
    lv_obj_align(frm_settings_wifi_dns, LV_ALIGN_TOP_LEFT, 0, 190);


    lv_obj_t * frm_settings_date_section;
    lv_obj_t * frm_settings_date_page = lv_menu_page_create(frm_settings_menu, NULL);
    lv_obj_set_style_pad_hor(frm_settings_date_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(frm_settings_menu), 0), 0);
    lv_menu_separator_create(frm_settings_date_page);
    frm_settings_date_section = lv_menu_section_create(frm_settings_date_page);

    lv_obj_t * frm_settings_obj_date = lv_obj_create(frm_settings_date_section);
    lv_obj_set_size(frm_settings_obj_date, 230, 230);
    lv_obj_clear_flag(frm_settings_obj_date, LV_OBJ_FLAG_SCROLLABLE);


    frm_settings_date_lbl = lv_label_create(frm_settings_obj_date);
    lv_label_set_text(frm_settings_date_lbl, "Date");
    lv_obj_align(frm_settings_date_lbl, LV_ALIGN_TOP_LEFT, 0, -5);


    frm_settings_day = lv_textarea_create(frm_settings_obj_date);
    lv_obj_set_size(frm_settings_day, 60, 30);
    lv_obj_align(frm_settings_day, LV_ALIGN_TOP_LEFT, 0, 20);
    lv_textarea_set_accepted_chars(frm_settings_day, "1234567890");
    lv_textarea_set_max_length(frm_settings_day, 2);
    lv_textarea_set_placeholder_text(frm_settings_day, "dd");


    frm_settings_month = lv_textarea_create(frm_settings_obj_date);
    lv_obj_set_size(frm_settings_month, 60, 30);
    lv_obj_align(frm_settings_month, LV_ALIGN_TOP_LEFT, 60, 20);
    lv_textarea_set_accepted_chars(frm_settings_month, "1234567890");
    lv_textarea_set_max_length(frm_settings_month, 2);
    lv_textarea_set_placeholder_text(frm_settings_month, "mm");


    frm_settings_year = lv_textarea_create(frm_settings_obj_date);
    lv_obj_set_size(frm_settings_year, 60, 30);
    lv_obj_align(frm_settings_year, LV_ALIGN_TOP_LEFT, 120, 20);
    lv_textarea_set_accepted_chars(frm_settings_year, "1234567890");
    lv_textarea_set_max_length(frm_settings_year, 4);
    lv_textarea_set_placeholder_text(frm_settings_year, "yyyy");


    frm_settings_time_lbl = lv_label_create(frm_settings_obj_date);
    lv_label_set_text(frm_settings_time_lbl, "Time");
    lv_obj_align(frm_settings_time_lbl, LV_ALIGN_TOP_LEFT, 0, 60);


    frm_settings_hour = lv_textarea_create(frm_settings_obj_date);
    lv_obj_set_size(frm_settings_hour, 60, 30);
    lv_obj_align(frm_settings_hour, LV_ALIGN_TOP_LEFT, 0, 80);
    lv_textarea_set_accepted_chars(frm_settings_hour, "1234567890");
    lv_textarea_set_max_length(frm_settings_hour, 2);
    lv_textarea_set_placeholder_text(frm_settings_hour, "hh");


    frm_settings_minute = lv_textarea_create(frm_settings_obj_date);
    lv_obj_set_size(frm_settings_minute, 60, 30);
    lv_obj_align(frm_settings_minute, LV_ALIGN_TOP_LEFT, 60, 80);
    lv_textarea_set_accepted_chars(frm_settings_minute, "1234567890");
    lv_textarea_set_max_length(frm_settings_minute, 2);
    lv_textarea_set_placeholder_text(frm_settings_minute, "mm");


    frm_settings_btn_setDate = lv_btn_create(frm_settings_obj_date);
    lv_obj_set_size(frm_settings_btn_setDate, 50, 20);
    lv_obj_align(frm_settings_btn_setDate, LV_ALIGN_TOP_LEFT, 130, 85);
    lv_obj_add_event_cb(frm_settings_btn_setDate, applyDate, LV_EVENT_SHORT_CLICKED, NULL);


    frm_settings_btn_setDate_lbl = lv_label_create(frm_settings_btn_setDate);
    lv_label_set_text(frm_settings_btn_setDate_lbl, "Set");
    lv_obj_set_align(frm_settings_btn_setDate_lbl, LV_ALIGN_CENTER);


    frm_settings_timezone = lv_dropdown_create(frm_settings_obj_date);
    lv_dropdown_set_text(frm_settings_timezone, "Time zone");
    lv_dropdown_set_options(frm_settings_timezone, "(GMT-11:00)\n"
                                                    "(GMT-10:00)\n"
                                                    "(GMT-09:00)\n"
                                                    "(GMT-08:00)\n"
                                                    "(GMT-07:00)\n"
                                                    "(GMT-06:00)\n"
                                                    "(GMT-05:00)\n"
                                                    "(GMT-03:00)\n"
                                                    "(GMT-01:00)\n"
                                                    "(GMT+00:00)\n"
                                                    "(GMT+01:00)\n"
                                                    "(GMT+03:00)\n"
                                                    "(GMT+03:30)\n"
                                                    "(GMT+04:00)\n"
                                                    "(GMT+05:30)\n"
                                                    "(GMT+08:00)\n"
                                                    "(GMT+09:00)\n"
                                                    "(GMT+10:00)\n");
    lv_dropdown_set_selected(frm_settings_timezone, 7);
    lv_obj_align(frm_settings_timezone, LV_ALIGN_OUT_TOP_LEFT, 0, 120);
    lv_obj_add_event_cb(frm_settings_timezone, tz_event,LV_EVENT_VALUE_CHANGED, NULL);


    lv_obj_t * frm_settings_ui_section;
    lv_obj_t * frm_settings_ui_page = lv_menu_page_create(frm_settings_menu, NULL);
    lv_obj_set_style_pad_hor(frm_settings_ui_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(frm_settings_menu), 0), 0);
    lv_menu_separator_create(frm_settings_ui_page);
    frm_settings_ui_section = lv_menu_section_create(frm_settings_ui_page);

    frm_settings_obj_ui = lv_obj_create(frm_settings_ui_section);
    lv_obj_set_size(frm_settings_obj_ui, 230, 230);
    lv_obj_clear_flag(frm_settings_obj_ui, LV_OBJ_FLAG_SCROLLABLE);


    frm_settings_btn_color_lbl = lv_label_create(frm_settings_obj_ui);
    lv_label_set_text(frm_settings_btn_color_lbl, "UI color");
    lv_obj_align(frm_settings_btn_color_lbl, LV_ALIGN_TOP_LEFT, 0, 5);


    frm_settings_color = lv_textarea_create(frm_settings_obj_ui);
    lv_obj_set_size(frm_settings_color , 100, 30);
    lv_obj_align(frm_settings_color, LV_ALIGN_TOP_LEFT, 60, 0);
    lv_textarea_set_max_length(frm_settings_color, 6);
    lv_textarea_set_accepted_chars(frm_settings_color, "abcdefABCDEF1234567890");


    frm_settings_btn_applycolor = lv_btn_create(frm_settings_obj_ui);
    lv_obj_set_size(frm_settings_btn_applycolor, 50, 20);
    lv_obj_align(frm_settings_btn_applycolor, LV_ALIGN_TOP_LEFT, 0, 30);
    lv_obj_add_event_cb(frm_settings_btn_applycolor, apply_color, LV_EVENT_SHORT_CLICKED, NULL);


    frm_settings_btn_applycolor_lbl = lv_label_create(frm_settings_btn_applycolor);
    lv_label_set_text(frm_settings_btn_applycolor_lbl, "Apply");
    lv_obj_set_align(frm_settings_btn_applycolor_lbl, LV_ALIGN_CENTER);


    frm_settings_brightness_section;
    frm_settings_brightness_page = lv_menu_page_create(frm_settings_menu, NULL);
    lv_obj_set_style_pad_hor(frm_settings_brightness_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(frm_settings_menu), 0), 0);
    lv_menu_separator_create(frm_settings_brightness_page);
    frm_settings_brightness_section = lv_menu_section_create(frm_settings_brightness_page);

    frm_settings_obj_brightness = lv_obj_create(frm_settings_brightness_section);
    lv_obj_set_size(frm_settings_obj_brightness, 230, 230);
    lv_obj_clear_flag(frm_settings_obj_brightness, LV_OBJ_FLAG_SCROLLABLE);

    static lv_style_t style_sel;
    lv_style_init(&style_sel);
    lv_style_set_text_font(&style_sel, &lv_font_montserrat_20);
    lv_style_set_text_color(&style_sel, lv_color_hex(0xffffff));

    frm_settings_brightness_roller = lv_roller_create(frm_settings_obj_brightness);
    lv_roller_set_options(frm_settings_brightness_roller, brightness_levels, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(frm_settings_brightness_roller, 3);
    lv_obj_set_style_text_align(frm_settings_brightness_roller, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_style(frm_settings_brightness_roller, &style_sel, LV_PART_SELECTED);
    lv_obj_set_width(frm_settings_brightness_roller, 40);
    lv_obj_align(frm_settings_brightness_roller, LV_ALIGN_TOP_MID, -10, 5);
    lv_obj_add_event_cb(frm_settings_brightness_roller, setBrightness, LV_EVENT_VALUE_CHANGED, NULL);

    frm_settings_brightness_lbl = lv_img_create(frm_settings_obj_brightness);
    lv_img_set_src(frm_settings_brightness_lbl, &icon_brightness);
    lv_obj_align(frm_settings_brightness_lbl, LV_ALIGN_TOP_MID, -13, -10);


    frm_settings_root_section;
    frm_settings_root_page = lv_menu_page_create(frm_settings_menu, "Settings");
    lv_obj_set_style_pad_hor(frm_settings_root_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(frm_settings_menu), 0), 0);
    frm_settings_root_section = lv_menu_section_create(frm_settings_root_page);
    lv_obj_add_event_cb(frm_settings_root_page, hide_settings, LV_EVENT_SHORT_CLICKED, NULL);

    frm_settings_context = create_text(frm_settings_root_page, NULL, "ID", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(frm_settings_menu, frm_settings_context, frm_settings_id_page);

    frm_settings_context = create_text(frm_settings_root_page, NULL, "Date", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(frm_settings_menu, frm_settings_context, frm_settings_date_page);

    frm_settings_context = create_text(frm_settings_root_page, NULL, "DX", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(frm_settings_menu, frm_settings_context, frm_settings_dx_page);

    frm_settings_context = create_text(frm_settings_root_page, NULL, "Wifi", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(frm_settings_menu, frm_settings_context, frm_settings_wifi_page);

    frm_settings_context = create_text(frm_settings_root_page, NULL, "UI", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(frm_settings_menu, frm_settings_context, frm_settings_ui_page);

    frm_settings_context = create_text(frm_settings_root_page, NULL, "Brightness", LV_MENU_ITEM_BUILDER_VARIANT_1);
    lv_menu_set_load_page_event(frm_settings_menu, frm_settings_context, frm_settings_brightness_page);

    lv_menu_set_sidebar_page(frm_settings_menu, frm_settings_root_page);
    lv_event_send(lv_obj_get_child(lv_obj_get_child(lv_menu_get_cur_sidebar_page(frm_settings_menu), 0), 0), LV_EVENT_CLICKED, NULL);




    lv_obj_add_flag(frm_settings, LV_OBJ_FLAG_HIDDEN);


    frm_not = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_not, 300, 25);
    lv_obj_align(frm_not, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_border_color(frm_not, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(frm_not, 0, 0 ) ;
    lv_obj_set_style_border_width(frm_not, 0, 0);


    frm_not_lbl = lv_label_create(frm_not);
    lv_obj_set_style_text_font(frm_not_lbl, &ubuntu, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(frm_not_lbl, LV_ALIGN_TOP_LEFT, 5, -10);
    lv_label_set_long_mode(frm_not_lbl, LV_LABEL_LONG_SCROLL);


    frm_not_symbol_lbl = lv_label_create(frm_not);
    lv_obj_align(frm_not_symbol_lbl, LV_ALIGN_TOP_LEFT, -10, -10);

    lv_obj_add_flag(frm_not, LV_OBJ_FLAG_HIDDEN);


    frm_wifi = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_wifi, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(frm_wifi, LV_OBJ_FLAG_SCROLLABLE);


    frm_wifi_btn_title = lv_btn_create(frm_wifi);
    lv_obj_set_size(frm_wifi_btn_title, 200, 20);
    lv_obj_align(frm_wifi_btn_title, LV_ALIGN_TOP_LEFT, -15, -15);

    frm_wifi_btn_title_lbl = lv_label_create(frm_wifi_btn_title);
    lv_label_set_text(frm_wifi_btn_title_lbl, "WiFi Networks");
    lv_obj_set_align(frm_wifi_btn_title_lbl, LV_ALIGN_LEFT_MID);


    frm_wifi_btn_back = lv_btn_create(frm_wifi);
    lv_obj_set_size(frm_wifi_btn_back, 50, 20);
    lv_obj_align(frm_wifi_btn_back, LV_ALIGN_TOP_RIGHT, 15, -15);
    lv_obj_add_event_cb(frm_wifi_btn_back, hide_wifi, LV_EVENT_SHORT_CLICKED, NULL);

    frm_wifi_btn_back_lbl = lv_label_create(frm_wifi_btn_back);
    lv_label_set_text(frm_wifi_btn_back_lbl, "Back");
    lv_obj_set_align(frm_wifi_btn_back_lbl, LV_ALIGN_CENTER);


    frm_wifi_btn_scan = lv_btn_create(frm_wifi);
    lv_obj_set_size(frm_wifi_btn_scan, 50, 20);
    lv_obj_align(frm_wifi_btn_scan, LV_ALIGN_TOP_RIGHT, -45, -15);
    lv_obj_add_event_cb(frm_wifi_btn_scan, wifi_scan, LV_EVENT_SHORT_CLICKED, NULL);

    frm_wifi_btn_scan_lbl = lv_label_create(frm_wifi_btn_scan);
    lv_label_set_text(frm_wifi_btn_scan_lbl, "Scan");
    lv_obj_set_align(frm_wifi_btn_scan_lbl, LV_ALIGN_CENTER);


    frm_wifi_list = lv_list_create(frm_wifi);
    lv_obj_set_size(frm_wifi_list, 310, 190);
    lv_obj_align(frm_wifi_list, LV_ALIGN_TOP_LEFT, -10, 30);


    frm_wifi_connected_to_lbl = lv_label_create(frm_wifi);
    lv_obj_align(frm_wifi_connected_to_lbl, LV_ALIGN_TOP_LEFT, 0, 10);
    lv_label_set_long_mode(frm_wifi_connected_to_lbl, LV_LABEL_LONG_SCROLL);
    lv_label_set_text(frm_wifi_connected_to_lbl, "");

    lv_obj_add_flag(frm_wifi, LV_OBJ_FLAG_HIDDEN);


    frm_wifi_simple = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_wifi_simple, 230, 110);
    lv_obj_align(frm_wifi_simple, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(frm_wifi_simple, LV_OBJ_FLAG_SCROLLABLE);


    frm_wifi_simple_title_lbl = lv_label_create(frm_wifi_simple);
    lv_obj_align(frm_wifi_simple_title_lbl, LV_ALIGN_TOP_MID, -10, -10);
    lv_label_set_long_mode(frm_wifi_simple_title_lbl, LV_LABEL_LONG_SCROLL);
    lv_label_set_text(frm_wifi_simple_title_lbl, "Connect to network");


    frm_wifi_simple_ta_pass = lv_textarea_create(frm_wifi_simple);
    lv_obj_set_size(frm_wifi_simple_ta_pass, 180, 30);
    lv_obj_align(frm_wifi_simple_ta_pass, LV_ALIGN_OUT_TOP_MID, -10, 20);
    lv_textarea_set_placeholder_text(frm_wifi_simple_ta_pass, "password");
    lv_textarea_set_password_mode(frm_wifi_simple_ta_pass, true);


    frm_wifi_simple_btn_see = lv_btn_create(frm_wifi_simple);
    lv_obj_set_size(frm_wifi_simple_btn_see, 30, 20);
    lv_obj_align(frm_wifi_simple_btn_see, LV_ALIGN_TOP_LEFT, 180, 25);
    lv_obj_add_event_cb(frm_wifi_simple_btn_see, show_pass, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(frm_wifi_simple_btn_see, show_pass, LV_EVENT_RELEASED, NULL);

    frm_wifi_simple_btn_see_lbl = lv_label_create(frm_wifi_simple_btn_see);
    lv_label_set_text(frm_wifi_simple_btn_see_lbl, LV_SYMBOL_EYE_OPEN);
    lv_obj_set_align(frm_wifi_simple_btn_see_lbl, LV_ALIGN_CENTER);


    frm_wifi_simple_btn_connect = lv_btn_create(frm_wifi_simple);
    lv_obj_set_size(frm_wifi_simple_btn_connect, 70, 20);
    lv_obj_align(frm_wifi_simple_btn_connect, LV_ALIGN_OUT_TOP_LEFT, -10, 60);

    frm_wifi_simple_btn_connect_lbl = lv_label_create(frm_wifi_simple_btn_connect);
    lv_label_set_text(frm_wifi_simple_btn_connect_lbl, "Connect");
    lv_obj_set_align(frm_wifi_simple_btn_connect_lbl, LV_ALIGN_CENTER);


    frm_wifi_simple_btn_back = lv_btn_create(frm_wifi_simple);
    lv_obj_set_size(frm_wifi_simple_btn_back, 50, 20);
    lv_obj_align(frm_wifi_simple_btn_back, LV_ALIGN_TOP_RIGHT, 10, 60);
    lv_obj_add_event_cb(frm_wifi_simple_btn_back, hide_simple, LV_EVENT_SHORT_CLICKED, NULL);

    frm_wifi_simple_btn_back_lbl = lv_label_create(frm_wifi_simple_btn_back);
    lv_label_set_text(frm_wifi_simple_btn_back_lbl, "Back");
    lv_obj_set_align(frm_wifi_simple_btn_back_lbl, LV_ALIGN_CENTER);

    lv_obj_add_flag(frm_wifi_simple, LV_OBJ_FLAG_HIDDEN);


    frm_wifi_login = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_wifi_login, 230, 140);
    lv_obj_align(frm_wifi_login, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(frm_wifi_login, LV_OBJ_FLAG_SCROLLABLE);


    frm_wifi_login_title_lbl = lv_label_create(frm_wifi_login);
    lv_obj_align(frm_wifi_login_title_lbl, LV_ALIGN_TOP_MID, -10, -10);
    lv_label_set_long_mode(frm_wifi_login_title_lbl, LV_LABEL_LONG_SCROLL);
    lv_label_set_text(frm_wifi_login_title_lbl, "Connect to network");


    frm_wifi_login_ta_login = lv_textarea_create(frm_wifi_login);
    lv_obj_set_size(frm_wifi_login_ta_login, 180, 30);
    lv_obj_align(frm_wifi_login_ta_login, LV_ALIGN_OUT_TOP_MID, -10, 20);
    lv_textarea_set_placeholder_text(frm_wifi_login_ta_login, "login");


    frm_wifi_login_ta_pass = lv_textarea_create(frm_wifi_login);
    lv_obj_set_size(frm_wifi_login_ta_pass, 180, 30);
    lv_obj_align(frm_wifi_login_ta_pass, LV_ALIGN_OUT_TOP_MID, -10, 50);
    lv_textarea_set_placeholder_text(frm_wifi_login_ta_pass, "password");
    lv_textarea_set_password_mode(frm_wifi_login_ta_pass, true);


    frm_wifi_login_btn_see = lv_btn_create(frm_wifi_login);
    lv_obj_set_size(frm_wifi_login_btn_see, 30, 20);
    lv_obj_align(frm_wifi_login_btn_see, LV_ALIGN_TOP_LEFT, 180, 55);
    lv_obj_add_event_cb(frm_wifi_login_btn_see, show_login_pass, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(frm_wifi_login_btn_see, show_login_pass, LV_EVENT_RELEASED, NULL);

    frm_wifi_login_btn_see_lbl = lv_label_create(frm_wifi_login_btn_see);
    lv_label_set_text(frm_wifi_login_btn_see_lbl, LV_SYMBOL_EYE_OPEN);
    lv_obj_set_align(frm_wifi_login_btn_see_lbl, LV_ALIGN_CENTER);


    frm_wifi_login_btn_connect = lv_btn_create(frm_wifi_login);
    lv_obj_set_size(frm_wifi_login_btn_connect, 70, 20);
    lv_obj_align(frm_wifi_login_btn_connect, LV_ALIGN_OUT_TOP_LEFT, -10, 90);

    frm_wifi_login_btn_connect_lbl = lv_label_create(frm_wifi_login_btn_connect);
    lv_label_set_text(frm_wifi_login_btn_connect_lbl, "Connect");
    lv_obj_set_align(frm_wifi_login_btn_connect_lbl, LV_ALIGN_CENTER);


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
        setenv("TZ", time_zone, 1);
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



const char * wifi_auth_mode_to_str(wifi_auth_mode_t auth_mode){
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
    return "unknown";
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
    wifi_got_ip = true;
}

void WiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
    wifi_got_ip = false;
}



void wifi_auto_connect(void * param){
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(WiFiDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    int n = 0;

    int c = 0;

    wifi_info wi;

    vector<wifi_info>list;

    char a[50] = {'\0'};

    if(wifi_connected_nets.list.size() != 0){
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        Serial.print("Searching for wifi connections...");

        lv_label_set_text(frm_home_title_lbl, "Searching for wifi connections...");
        lv_label_set_text(frm_home_symbol_lbl, LV_SYMBOL_WIFI);

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
            lv_label_set_text(frm_home_title_lbl, "No wifi networks found");
            lv_label_set_text(frm_home_symbol_lbl, LV_SYMBOL_WIFI);
        }
        Serial.println("done");

        lv_label_set_text(frm_home_title_lbl, "done");
        lv_label_set_text(frm_home_symbol_lbl, LV_SYMBOL_WIFI);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        if(wifi_connected_nets.list.size() > 0){
            for(uint32_t i = 0; i < wifi_connected_nets.list.size(); i++){
                for(uint32_t j = 0; j < list.size(); j++){
                    if(strcmp(wifi_connected_nets.list[i].SSID, list[j].SSID) == 0){

                        Serial.print("Connecting to ");
                        Serial.print(wifi_connected_nets.list[i].SSID);
                        strcpy(a, "Connecting to ");
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
                                vTaskDelay(1000 / portTICK_PERIOD_MS);
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
                                vTaskDelay(1000 / portTICK_PERIOD_MS);
                            }

                            if(WiFi.isConnected()){
                                last_wifi_con = i;
                            }
                        }
                    }
                }
            }

            if(WiFi.isConnected()){

                lv_label_set_text(frm_home_title_lbl, "connected");
                lv_label_set_text(frm_home_symbol_lbl, LV_SYMBOL_WIFI);

                lv_obj_set_style_text_color(frm_settings_btn_wifi_lbl, lv_color_hex(0x00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);

                strcpy(connected_to, wifi_connected_nets.list[last_wifi_con].SSID);
                strcat(connected_to, " ");
                strcat(connected_to, WiFi.localIP().toString().c_str());
                Serial.println(" connected");
                vTaskDelay(2000 / portTICK_PERIOD_MS);

                lv_label_set_text(frm_home_title_lbl, "");
                lv_label_set_text(frm_home_symbol_lbl, "");

                datetime();
                if(param != NULL){

                    xTaskCreatePinnedToCore(setupServer, "server", 12000, (void*)"ok", 1, NULL, 1);

                }
            }else{

                Serial.println("disconnected");
                lv_label_set_text(frm_home_title_lbl, "disconnected");
                lv_label_set_text(frm_home_symbol_lbl, LV_SYMBOL_WIFI);
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                lv_label_set_text(frm_home_title_lbl, "");
                lv_label_set_text(frm_home_symbol_lbl, "");
                WiFi.disconnect();
                WiFi.mode(WIFI_OFF);
            }
        }
    }

    if(param != NULL){
        vTaskDelete(task_wifi_auto);
        task_wifi_auto = NULL;
    }
}

void announce(){
    lora_packet hi;

    Serial.println("================announce===============");

    strcpy(hi.sender, user_id);
    strcpy(hi.status, "show");
    transmiting_packets.push_back(hi);
    Serial.println("=======================================");
}

void task_beacon(void * param){
    uint32_t r = 0;
    while(true){
        r = rand() % 30;
        if(r < 10)
            r += 10;
        Serial.print("R = ");
        Serial.println(r);
        vTaskDelay(1000 * r / portTICK_PERIOD_MS);
        announce();
    }
    vTaskDelete(NULL);
}

void setup_sound(){
    bool findMp3 = false;

    audio.setPinout(BOARD_I2S_BCK, BOARD_I2S_WS, BOARD_I2S_DOUT);
    audio.setVolume(21);

    if(SPIFFS.exists("/comp_up.mp3")){
        findMp3 = audio.connecttoFS(SPIFFS, "/comp_up.mp3");
        if(findMp3){

        }
    }else
        Serial.println("comp_up.mp3 not found");

}


void setup(){
    if(!SPIFFS.begin(true)){
        Serial.println("failed mounting SPIFFS");
    }
    bool ret = false;
    Serial.begin(115200);



    contacts_list.setCheckPeriod(1);

    loadContacts();


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

    pthread_mutexattr_t Attr;
    pthread_mutexattr_init(&Attr);
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&lvgl_mutex, &Attr);
    pthread_mutex_init(&messages_mutex, NULL);
    pthread_mutex_init(&send_json_mutex, NULL);
    pthread_mutex_init(&websocket_send, NULL);

    Wire.beginTransmission(touchAddress);
    ret = Wire.endTransmission() == 0;
    touchDected = ret;
    kbDected = checkKb();

    setupLvgl();

    ui();
# 4660 "/home/rene/Documents/T-Deck-p/examples/T/T.ino"
    loadSettings();


    analogWrite(BOARD_BL_PIN, mapv(brightness, 0, 9, 100, 255));

    if(wifi_connected_nets.list.size() > 0)
        xTaskCreatePinnedToCore(wifi_auto_connect, "wifi_auto", 10000, (void*)"ok", 1, &task_wifi_auto, 0);

    if(wifi_connected)
        datetime();

    xTaskCreatePinnedToCore(update_time, "update_time", 11000, (struct tm*)&timeinfo, 2, &task_date_time, 1);


    setDate(2024, 1, 1, 0, 0, 0, 0);


    initBat();
    xTaskCreatePinnedToCore(update_bat, "task_bat", 4000, NULL, 2, &task_bat, 1);


    xTaskCreatePinnedToCore(notify, "notify", 4000, NULL, 1, &task_not, 1);

    xTaskCreatePinnedToCore(task_beacon, "beacon", 4000, NULL, 1, NULL, 1);


    xTaskCreatePinnedToCore(collectPackets, "collect_pkt", 3000, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(processPackets, "process_pkt", 5000, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(processReceivedStats, "proc_stats_pkt", 3000, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(processTransmittingPackets, "proc_tx_pkt", 3000, NULL, 1, NULL, 1);

    setup_sound();
}

void loop(){
    pthread_mutex_lock(&lvgl_mutex);
    lv_task_handler();
    pthread_mutex_unlock(&lvgl_mutex);

    if(secureServer != NULL)
        if(secureServer->isRunning()){
        xSemaphoreTake(xSemaphore, portMAX_DELAY);
        secureServer->loop();
        xSemaphoreGive(xSemaphore);
    }
    delay(5);
    if(audio.isRunning()){
        audio.loop();
        delay(3);
    }
}