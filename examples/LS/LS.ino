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
//#include "favicon.h"
#include "ArduinoJson.hpp"
#include "apps/discovery/app_discovery.hpp"
#include "apps/tictactoe/tictactoe.hpp"

using namespace httpsserver;
using namespace ArduinoJson;

// Using custom fonts
LV_FONT_DECLARE(clocknum);
LV_FONT_DECLARE(ubuntu);
// Using customs images for icons and background
LV_IMG_DECLARE(icon_brightness);
LV_IMG_DECLARE(icon_lora2);
LV_IMG_DECLARE(bg2);
// List of wifi access points used after a scan
vector <wifi_info> wifi_list;
// List that represents a history of wifi nets once connected
Wifi_connected_nets wifi_connected_nets = Wifi_connected_nets();

#define TOUCH_MODULES_GT911
#include "TouchLib.h"
#include <lwip/apps/sntp.h>
// Depending on LoRa module, set the frequency here (433, 868, 915)
#define RADIO_FREQ 915.0
// See utilities.h and online documentation about the pins of the esp32s3
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
// Used to play sounds like mp3
Audio audio;
uint8_t touchAddress = GT911_SLAVE_ADDRESS1;
TouchLib *touch = NULL;
TFT_eSPI tft;
// Used to avoid simultanous access to the same resource, avoids esporadic resets,
// corrupt data, load prohibited..., use with xSemaphoreTake and release with,
// xSemaphoreGive, see other parts of this code.
SemaphoreHandle_t xSemaphore = NULL;
// Mutexes used to control access, using pthread_mutex_lock.
static pthread_mutex_t lvgl_mutex;
static pthread_mutex_t messages_mutex;
static pthread_mutex_t send_json_mutex;
static pthread_mutex_t websocket_send;
static pthread_mutex_t sound_mutex;
// Used to represent the state of some resources
bool touchDected = false;
bool kbDected = false;
bool hasRadio = false; 
bool wifi_connected = false;
bool isDX = false;
bool chart_zoomed = false;
uint8_t rssi_chart_width = 90;
uint8_t rssi_chart_height = 55;
volatile bool transmiting = false;
volatile bool processing = false;
volatile bool gotPacket = false;
volatile bool sendingJson = false;
volatile bool server_ready = false;
volatile bool parsing = false;
volatile bool new_stats = false;
volatile bool using_transmit_pkt_list = false;
volatile bool msg_confirmed = false;
// Hardware inputs
lv_indev_t *touch_indev = NULL;
lv_indev_t *kb_indev = NULL;
// Handles to access tasks in threads
TaskHandle_t task_recv_pkt = NULL, // when we receive a LoRa packet
             task_check_new_msg = NULL, // when a new message is detected
             task_not = NULL, // when we have a new notification
             task_date_time = NULL, // updates date and time periodically
             task_bat = NULL, // update the battery status
             task_wifi_auto = NULL, // as soon as the board boots, connects to a wifi
             task_wifi_scan = NULL, // to scan without blocking the interface
             task_play = NULL, // test purpose
             task_play_radio = NULL; // test purpose
// Holds how many messages a selected contact have.
uint32_t msg_count = 0;
// List of contacts, LoRa packets and notification
Contact_list contacts_list = Contact_list();
lora_incomming_packets pkt_list = lora_incomming_packets();
static int16_t transmit(uint8_t * data, size_t len);
static uint32_t time_on_air(size_t len);
lora_outgoing_packets transmit_pkt_list = lora_outgoing_packets(transmit);
lora_pkt_history pkt_history = lora_pkt_history();
notification notification_list = notification();
vector<lora_packet> received_packets;
vector<lora_packet> transmiting_packets;
// As soon as we select a contact on a contact list, it is pointed to this
// variable, so other routines like send a message will use this contact
Contact * actual_contact = NULL;
// Used in UI objects, settings
char user_name[50] = "";
char user_id[7] = "";
char user_key[17] = "";
char http_admin_pass[50] = "admin";
char connected_to[200] = "";
char ui_primary_color_hex_text[7] = {'\u0000'};
uint32_t ui_primary_color = 0x5c81aa;
uint8_t brightness = 1;
// This is where we get and set the date and time
struct tm timeinfo;
char time_zone[10] = "<+03>3";
// A reference to measure the battery vcc
int vRef = 0;
// Update the battery status on screen
void update_bat(void * param);
// Update the date from internet
void datetime();
// Used when we touch the wifi button on settings, it is a toggle wich
// connects to a wifi saved on wifi_connected_nets or disconnects.
void wifi_auto_toggle();
// Function to retrieve a string that represents the wifi mode
// where auth_mode is the code of the mode
const char * wifi_auth_mode_to_str(wifi_auth_mode_t auth_mode);
// Just an index representing the position on the last wifi connection on wifi_connected_nets
volatile int last_wifi_con = -1;
// Web server object.
HTTPSServer * secureServer = NULL;
// Max number of clients connections.
const uint8_t maxClients = 1;
#define BLOCK_SIZE 16  // AES bloco size (16 bytes)
volatile bool wifi_got_ip = false;
volatile float rssi, snr;
#define APP_SYSTEM 1
#define APP_LORA_CHAT 2
discovery_app discoveryApp = discovery_app();

/// @brief Loads the user name, id, key, color of the interface and brightness.
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
/// @brief Saves the user name, id, key, color of the interface and brightness.
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
/// @brief Load the contacts list.
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
/// @brief Saves the contacts.
static void saveContacts(){
    if(!SPIFFS.begin(true)){
        Serial.println("failed mounting SPIFFS");
        return;
    }

    SPIFFS.remove("/contacts");

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
/// @brief Refreshes the contacts list object.
static void refresh_contact_list(){
    // Clean the list
    lv_obj_clean(frm_contacts_list);
    
    lv_obj_t * btn;
    // Populate the list creating buttons based on the contact's info
    for(uint32_t i = 0; i < contacts_list.size(); i++){
        // Adds a new item on the list with the contact's name and return a pointer to the button
        btn = lv_list_add_btn(frm_contacts_list, LV_SYMBOL_CALL, contacts_list.getContact(i).getName().c_str());
        // Create a label with the id of the contact, will be used later on
        lv_obj_t * lbl = lv_label_create(btn);
        lv_label_set_text(lbl, contacts_list.getContact(i).getID().c_str());
        // Create a small square 20 x 20 pixels that represents the online status by color
        lv_obj_t * obj_status = lv_obj_create(btn);
        lv_obj_set_size(obj_status, 20, 20);
        // Normally the object has scrollbars so we need to hide them
        lv_obj_clear_flag(obj_status, LV_OBJ_FLAG_SCROLLABLE);
        // If the current contact is near by we set the background color of the status square to green
        // otherwise to light gray, use the colors you want. Keep in mind, we are creating the label
        // and the status object with the button as parent, so it will be part of the button as well,
        // so every contact in the list will have its own id(hidden) and status object(visible). The
        // list created will be orded the same as the contact_list.
        if(contacts_list.getContact(i).inrange){
            lv_obj_set_style_bg_color(obj_status, lv_color_hex(0x00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        else{
            lv_obj_set_style_bg_color(obj_status, lv_color_hex(0xaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        // Align the status object in the right most position
        lv_obj_align(obj_status, LV_ALIGN_RIGHT_MID, 0, 0);
        // We need to hide the label with the id
        lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
        // Add some events when clicked or long press the contact on the list
        lv_obj_add_event_cb(btn, show_edit_contacts, LV_EVENT_LONG_PRESSED, btn);
        lv_obj_add_event_cb(btn, show_chat, LV_EVENT_SHORT_CLICKED, btn);
        
    }
    // After this, we save the list o contacts, there are other routines that modifies the contacts info
    // so for now this is the best place to save the contacts every time its info changes. Not perfect.
    saveContacts();
}

/// @brief This task runs forever and every 100 ms. We watch for a new message and show in a small area on top of the screen
/// @brief a symbol and a message.
/// @param param 
static void notify(void * param){
    char n[30] = {'\0'}; // the message
    char b[13] = {'\0'}; // the symbol
    while(true){
        // Reusing this task to verify the conectivity of wifi, its just adds or remove the wifi symbol from the main
        // screen.
        update_wifi_icon();
        // Every time we got a message on the notification list...
        if(notification_list.size() > 0){
            // We copy the symbol and the message, also erases from the list.
            notification_list.pop(n, b);
            // This happen really fast, we show the notification object with his message and symbols empty.
            lv_obj_clear_flag(frm_not, LV_OBJ_FLAG_HIDDEN);
            // Set to foreground
            lv_obj_move_foreground(frm_not);
            // Set the symbol and message on the labels.
            lv_label_set_text(frm_not_lbl, n);
            lv_label_set_text(frm_not_symbol_lbl, b);
            // The main screen(or home screen) has two labels for its own symbol and message
            // we could use it to display a message, so we're using it too.
            lv_label_set_text(frm_home_title_lbl, n);
            lv_label_set_text(frm_home_symbol_lbl, b);
            // We make this task wait a second.
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            // We clean the labels and hide the notification object.
            lv_label_set_text(frm_not_lbl, "");
            lv_label_set_text(frm_not_symbol_lbl, "");
            lv_obj_add_flag(frm_not, LV_OBJ_FLAG_HIDDEN);
            // Just for debug
            Serial.println("notified");
        }
        // Give the cpu core a rest
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

/// @brief T-Deck's routine to update the screen.
/// @param disp 
/// @param area 
/// @param color_p 
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
/// @brief Gets the touch coodinates.
/// @param x 
/// @param y 
/// @return 
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
/// @brief T-Deck's routine.
/// @param indev_driver 
/// @param data 
static void touchpad_read( lv_indev_drv_t *indev_driver, lv_indev_data_t *data )
{
    data->state = getTouch(data->point.x, data->point.y) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}
/// @brief Reuturns a char typed on physical keyboard.
/// @param  
/// @return 
static uint32_t keypad_get_key(void)
{
    char key_ch = 0;
    Wire.requestFrom(0x55, 1);
    while (Wire.available() > 0) {
        key_ch = Wire.read();
        
    }
    return key_ch;
}
/// @brief T-Deck's routine.
/// @param indev_drv 
/// @param data 
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
/// @brief Sets up the lvgl's objects, drivers, interfaces...
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
/// @brief Handy way to get i2c address of the modules on the board.
/// @param w 
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
/// @brief Used to detect the presence of the physical keyboard.
/// @return 
bool checkKb()
{
    Wire.requestFrom(0x55, 1);
    return Wire.read() != -1;
}
/// @brief Return the TZ string
/// @param index 
/// @return const char *
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

/// @brief Sets the Time zone
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

/// @brief  Transforms the contact list in a JSON string 
/// @brief  "{"command" : "contacts", "contacts" : [{"id":"abcdef","name":"john doe","status":"on"},
/// @brief                                          {"id":"aaaaaa","name":"joe","status":"off"}]}"
/// @return 
std::string contacts_to_json(){
    JsonDocument doc;
    std::string json;

    // Fist the command parsed by the javascript on client side.
    doc["command"] = "contacts";
    vector<Contact> * cl = contacts_list.getContactsList();
    for(uint32_t i = 0; i < (*cl).size(); i++){
        // A list of contacts represented by "contacts", not the command.
        doc["contacts"][i]["id"] = (*cl)[i].getID();
        doc["contacts"][i]["name"] = (*cl)[i].getName();
        doc["contacts"][i]["key"] = (*cl)[i].getKey();
        doc["contacts"][i]["status"] = (*cl)[i].inrange ? "on" : "off";
    }
    // This transforms the doc object into a string
    serializeJson(doc, json);
    return json;
}
// This is the starting point of the http server, it is a work in progress, may have bugs.
#define HEADER_USERNAME "X-USERNAME"
#define HEADER_GROUP    "X-GROUP"
// This reads a http response from the client, extract the user and password and compare
// if is equal to the configuration, if not the server sends a 401 and a warning.
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
/// @brief This checks if the client provided a valid user, if not a warning is sent.
/// @param req 
/// @param res 
/// @param next 
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
    //res->printStd(FAVICON_DATA);
    Serial.println("favicon sent.");
}

/// @brief This is used to send the index.html stored on the variable index_html, se index.h.
/// @param req 
/// @param res 
void handleRoot(HTTPRequest * req, HTTPResponse * res) {
    server_ready = false;
    Serial.println("Sending main page");
    res->setHeader("Content-Type", "text/html");
    res->setStatusCode(200);
    res->setStatusText("OK");
    res->printStd(index_html);
    server_ready = true;
}
/// @brief This is used to send the style.css stored on the variable style_css, se style.h.
/// @param req 
/// @param res 
void handleStyle(HTTPRequest * req, HTTPResponse * res) {
    Serial.println("Sending style.css");
    res->setHeader("Content-Type", "text/css");
    res->setStatusCode(200);
    res->setStatusText("OK");
    res->println(style_css);
}
/// @brief This is used to send the script.js stored on the variable script_js, se script.h.
/// @param req 
/// @param res 
void handleScript(HTTPRequest * req, HTTPResponse * res) {
    Serial.println("Sending script.js");
    res->setHeader("Content-Type", "application/javascript");
    res->setStatusCode(200);
    res->setStatusText("OK");
    res->println(script_js);
}
/// @brief This is sent in case when index.html has a resource that is not available in the server.
/// @param req 
/// @param res 
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
/// @brief This class represents a chat object. It is a WebSocket handler wich is open when a client connects.
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

// List of clients's websockets.
ChatHandler * activeClient;
// Constructor for a new client WebSocket
WebsocketHandler * ChatHandler::create() {
    Serial.println("Creating new chat client!");
    ChatHandler * handler = new ChatHandler();
    if (activeClient == nullptr) {
        activeClient = handler;

    }
    
    return handler;
}

/// @brief Search and close the client instance.
void ChatHandler::onClose() {
    if (activeClient != NULL) {
        Serial.println("closing websocket");
        activeClient = NULL;
    }
}
/// @brief This gets a JSON string built with seralizeJSON and send it through the websockets.
/// @param json 
void sendJSON(string json){
    //char j[json.size() + 1];
    //strcpy(j, json.c_str());

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
/// @brief This is for debug purposes, a received decrypted message sometimes could bring non-printable
/// @brief chars and it was making the board reboot.
/// @param str 
/// @return bool
bool containsNonPrintableChars(const char *str) {
    while (*str) {
        if (*str < 32 || *str > 126) { // ASCII values for printable characters
            return true;
        }
        str++;
    }
    return false;
}
// For debug purposes.
void printMessages(const char * id){
    vector<ContactMessage> msgs;
    // A mutex is used here to prevent concurrent access and memory corruption.
    pthread_mutex_lock(&messages_mutex);
        msgs = contacts_list.getContactMessages(id);
    pthread_mutex_unlock(&messages_mutex);
    Serial.println("=================================");
    if((msgs).size() > 0){
        for(uint32_t i = 0; i < (msgs).size(); i++)
            Serial.println((msgs)[i].message);
    }
    Serial.println("=================================");
}
/// @brief This creates and sends a JSON to the client with the list of messages from a selected contact.
/// @param id 
void sendContactMessages(const char * id){
    vector<ContactMessage> msgs;
    JsonDocument doc;
    string json;
    // Mutex to avoid concurrent access and memory corruption.
    pthread_mutex_lock(&messages_mutex);
        msgs = contacts_list.getContactMessages(id);
    pthread_mutex_unlock(&messages_mutex);
    // Command and id for the requested contact.
    doc["command"] = "msg_list";
    doc["id"] = id;
    // Creating a list of messages. 'Me' is a bool that represents when the message is from the sender, if false
    // so the message is from the destination. We also add the date and time of the message.
    if((msgs).size() > 0){
        for(uint32_t i = 0; i < (msgs).size(); i++){
            doc["messages"][i]["me"] = (msgs)[i].me;
            doc["messages"][i]["msg_date"] = (msgs)[i].dateTime;
            doc["messages"][i]["msg"] = (msgs)[i].message;
        }
    }
    // Transform the doc in JSON string.
    serializeJson(doc, json);
    // This is a simple way to wait other tasks to finish using sendJSON.
    while(sendingJson){
        Serial.println("waiting other json to finish...");
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    // Mutex to avoid concurrent access and memory corruption.
    pthread_mutex_lock(&send_json_mutex);
    sendJSON(json.c_str());
    pthread_mutex_unlock(&send_json_mutex);
    Serial.println(json.c_str());
}
/// @brief Set the brightness of the screen.
/// @param level 
void setBrightness2(uint8_t level){
    // Store the level.
    brightness = level;
    // Apply to the backlight pin.
    analogWrite(BOARD_BL_PIN, mapv(brightness, 0, 9, 100, 255));
    // Update the value displayed on settings.
    lv_roller_set_selected(frm_settings_brightness_roller, brightness, LV_ANIM_OFF);
    // Save the settings.
    saveSettings();
}
/// @brief Returns a JSON with the settings.
/// @return 
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

/// @brief Complete the string with zero until it becomes a multiple of 16 bytes
/// @param plaintext 
/// @param length 
/// @param padded_plaintext 
void zero_padding(unsigned char *plaintext, size_t length, unsigned char *padded_plaintext) {
    // Copy the original text to the buffer that will be padded
    memcpy(padded_plaintext, plaintext, length);
    // Fill the rest of the buffer with zero.
    memset(padded_plaintext + length, 0, BLOCK_SIZE - (length % BLOCK_SIZE));
}

/// @brief Function to encrypt the text with a 16 bytes key using ECB
/// @param text 
/// @param key 
/// @param text_length 
/// @param ciphertext 
void encrypt_text(unsigned char *text, unsigned char *key, size_t text_length, unsigned char *ciphertext) {
    // Calculate the size to fill the text
    size_t padded_len = ((text_length + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;

    // Buffer to hold the padded text
    unsigned char padded_text[padded_len];

    // Fill the buffer with zero
    zero_padding(text, text_length, padded_text);

    // Initialize the AES structure
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    // Set the crypto key
    mbedtls_aes_setkey_enc(&aes, key, 128); // 128 bits

    // Encrypt every 16 bytes block of the text
    for (size_t i = 0; i < padded_len; i += BLOCK_SIZE) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, &padded_text[i], &ciphertext[i]);
    }

    // Free the AES structure
    mbedtls_aes_free(&aes);
}

/// @brief Function to decrypt the text using AES 16 bytes key
/// @param ciphertext 
/// @param key 
/// @param cipher_length 
/// @param decrypted_text 
void decrypt_text(unsigned char *ciphertext, unsigned char *key, size_t cipher_length, unsigned char *decrypted_text) {
    // Init the AES structure
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    // Set the decrypt key
    mbedtls_aes_setkey_dec(&aes, key, 128); // 128 bits (16B)

    // Decrypt every 16 bytes of the encrypted text
    for (size_t i = 0; i < cipher_length; i += BLOCK_SIZE) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, &ciphertext[i], &decrypted_text[i]);
    }

    // Free the AES structure
    mbedtls_aes_free(&aes);
}

/// @brief This is used to parse commands and redirect data received or sent through a websocket.
/// @param jsonString 
void parseCommands(std::string jsonString){
    while(sendingJson){
        Serial.println("(sendingJSON)WebSocket busy, wait...");
        delay(100);
    }
    parsing = true;
    JsonDocument doc;
    // Transform a JSON string in a JSON object.
    //Serial.println(jsonString.c_str());
    deserializeJson(doc, jsonString);
    // Extract the command string.
    const char * command = doc["command"];
    if(strcmp(command, "send") == 0){
        // Send means a message to be sent to a destination.
        // We need to ensure id and msg are strings.
        const char * id = (const char*)doc["id"];
        const char * msg = (const char*)doc["msg"];
        // If the id of the sender is the default on client's javascript or the contact wasn't selected, return.
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

        // It will send only if the LoRa module is configured.
        //hasRadio = false;
        if(hasRadio){
            // We need a new LoRa packet.
            lora_packet pkt;
            ContactMessage cm;

            pkt.type = LORA_PKT_DATA;
            strcpy(pkt.sender, user_id);
            strcpy(pkt.destiny, id);
            // There are two statuses, 'send' when sending to a destination, and 'recv' when we received a confirmation
            // from the destination. This is how we know the destination received the message. Not guaranteed.
            strcpy(pkt.status, "send");
            //strftime(pkt.date_time, sizeof(pkt.date_time)," - %a, %b %d %Y %H:%M", &timeinfo);
            memcpy(pkt.data, ciphertext, padded_len);
            pkt.data_size = padded_len;
            transmiting_packets.push_back(pkt);
            // And add the unencrypted message.
            strcpy(pkt.data, msg);
            Serial.print("Adding answer to ");
            Serial.println(pkt.destiny);
            Serial.println(pkt.data);

            // Populating a contact message struct
            cm.me = true;
            strftime(cm.dateTime, sizeof(cm.dateTime)," - %a, %b %d %Y %H:%M", &timeinfo);
            strcpy(cm.message, msg);
            strcpy(cm.messageID, generate_ID(6).c_str());

            // Mutex to avoid errors, curruptions and concurrency.
            pthread_mutex_lock(&messages_mutex);
            // Adding the contact message packet to his list of messages
            contacts_list.getContactByID(id)->addMessage(cm);
            pthread_mutex_unlock(&messages_mutex);
            
        }else  
            Serial.println("Radio not configured");
    }else if(strcmp(command, "contacts") == 0){// When we are asked to send the contacts list.
        // Wait if sendJSON is being used.
        while(sendingJson){
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        // Obtain the exclusive access to sendJSON
        pthread_mutex_lock(&send_json_mutex);
        sendJSON(contacts_to_json());
        pthread_mutex_unlock(&send_json_mutex);
    }else if(strcmp(command, "sel_contact") == 0){// As soons as the client selects a contact on the list.
        // actual_contact points to the selected contact on the client side list of contacts.
        actual_contact = contacts_list.getContactByID(doc["id"]);
        if(actual_contact != NULL){
            Serial.println("Contact selected");
            // Send it's messages.
            sendContactMessages(actual_contact->getID().c_str());
        }
        else
            Serial.println("Contact selection failed, id not found");
    }else if(strcmp(command, "edit_contact") == 0){// Edit contacts info.
        Contact * c = NULL;
        bool edited = false;
        if(strcmp(doc["id"], doc["newid"]) != 0){// If the new id is different, we need to ensure nobody is using it.
            // Search for a contact using the new id. If NULL we enable edition.
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
        else // If the ids are the same, maybe the user wants to change the name or key.
            edited = true;
        if(edited){
            // c is pointing to the contact, so we have direct access to the contact info.
            c = contacts_list.getContactByID(doc["id"]);
            if(c != NULL){
                // Set new id and name.
                c->setID((const char *)doc["newid"]);
                c->setName((const char *)doc["newname"]);
                c->setKey((const char *)doc["newkey"]);
                // Save the contact list.
                saveContacts();
                while(sendingJson){
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
                // We need to send the contacts list again and a notification, so lock on it.
                pthread_mutex_lock(&send_json_mutex);
                sendJSON(contacts_to_json().c_str());
                sendJSON("{\"command\" : \"notification\", \"message\" : \"Edited.\"}");
                pthread_mutex_unlock(&send_json_mutex);
            }
        }
    }else if(strcmp(command, "del_contact") == 0){
        // Verify if the contact exists.
        Contact * c = contacts_list.getContactByID(doc["id"]);
        if(c != NULL){
            // Erase it.
            if(contacts_list.del(*c)){
                // For debug purposes.
                Serial.print("Contact id ");
                Serial.print((const char*)doc["id"]);
                Serial.println(" deleted.");
                // Save the contacts list.
                saveContacts();
                while(sendingJson){
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
                // Lock and send the contacts list and a JSON notification.
                pthread_mutex_lock(&send_json_mutex);
                sendJSON(contacts_to_json().c_str());
                sendJSON("{\"command\" : \"notification\", \"message\" : \"Contact deleted.\"}");
                pthread_mutex_unlock(&send_json_mutex);
            }else{// If it was impossible to delete.
                while(sendingJson){
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
                pthread_mutex_lock(&send_json_mutex);
                sendJSON("{\"command\" : \"notification\", \"message\" : \"Error deleting contact.\"}");
                pthread_mutex_unlock(&send_json_mutex);
            }
        }else{// If contact not found.
            while(sendingJson){
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            pthread_mutex_lock(&send_json_mutex);
            sendJSON("{\"command\" : \"notification\", \"message\" : \"Contact not found.\"}");
            pthread_mutex_unlock(&send_json_mutex);
        }
    }else if(strcmp(command, "new_contact") == 0){
        // Verify if the contact id already exists.
        Contact * c = contacts_list.getContactByID(doc["id"]);
        if(c == NULL){
            // Creating a new contact.
            c = new Contact(doc["name"], doc["id"]);
            c->setKey(doc["key"]);
            // Add it to the list of contacts.
            if(contacts_list.add(*c)){
                // Print some info on console.
                Serial.print("New contact ID ");
                Serial.print((const char *)doc["id"]);
                Serial.println(" added.");
                // Save the contacts list.
                saveContacts();
                while(sendingJson){
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
                // Lock and send the contacts list and notification.
                pthread_mutex_lock(&send_json_mutex);
                sendJSON(contacts_to_json());
                sendJSON("{\"command\" : \"notification\", \"message\" : \"Contact added.\"}");
                pthread_mutex_unlock(&send_json_mutex);
            }
            else{// If adding fails.
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
    }else if(strcmp(command, "read_settings") == 0){ // Send a JSON string with the settings
        while(sendingJson){
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        pthread_mutex_lock(&send_json_mutex);
        sendJSON(settingsJSON());
        pthread_mutex_unlock(&send_json_mutex);
    }else if(strcmp(command, "set_brightness") == 0){// Sets the screen brightness, note the cast.
        setBrightness2((uint8_t)doc["brightness"]);
    }else if(strcmp(command, "set_ui_color") == 0){
        char color[7] = "";
        
        strcpy(color, doc["color"]);
        // We don't need to do any change if the client side sends a empty string.
        if(strcmp(color, "") == 0)
            return;
        // Update the text area on the settings.
        lv_textarea_set_text(frm_settings_color, color);
        // Lets use the event to apply color.
        apply_color(NULL);
        // Save the settings.
        saveSettings();
    }else if(strcmp(command, "set_name_id") == 0){// Sets the new name and id for the sender(T-Deck owner).
        // Set the id and name of the sender(T-Deck owner).
        strcpy(user_id, doc["id"]);
        strcpy(user_name, doc["name"]);
        strcpy(user_key, doc["key"]);
        // Update the text areas on settings.
        lv_textarea_set_text(frm_settings_name, user_name);
        lv_textarea_set_text(frm_settings_id, user_id);
        lv_textarea_set_text(frm_settings_key, user_key);
        // The owner's info make part of settings.
        saveSettings();
        while(sendingJson){
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        // Send a notification
        pthread_mutex_lock(&send_json_mutex);
        sendJSON("{\"command\" : \"notification\", \"message\" : \"Name, ID and key saved.\"}");
        pthread_mutex_unlock(&send_json_mutex);
    }else if(strcmp(command, "set_dx_mode") == 0){// This toggles between DX and normal mode of LoRa transmission.
        if(doc["dx"]){
            if(DXMode()){ // If dx mode is configured successfully, send a notification.
            while(sendingJson){
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
                pthread_mutex_lock(&send_json_mutex);
                sendJSON("{\"command\" : \"notification\", \"message\" : \"DX mode.\"}");
                pthread_mutex_unlock(&send_json_mutex);
            }
        }
        else{
            if(normalMode()){// Same as above. Doesn't need to reset the device.
                while(sendingJson){
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
                pthread_mutex_lock(&send_json_mutex);
                sendJSON("{\"command\" : \"notification\", \"message\" : \"Normal mode.\"}");
                pthread_mutex_unlock(&send_json_mutex);
            }
        }
    }else if(strcmp(command, "set_date") == 0){// Sets manually the date and time.
        // Gather from the client side all related to date and time, separately, and update all the text areas
        // on settings(click and see the cog wheel at the feet of the astronaut :p).
        lv_textarea_set_text(frm_settings_day, doc["d"]);
        lv_textarea_set_text(frm_settings_month, doc["m"]);
        lv_textarea_set_text(frm_settings_year, doc["y"]);
        lv_textarea_set_text(frm_settings_hour, doc["hh"]);
        lv_textarea_set_text(frm_settings_minute, doc["mm"]);
        
        // setDateTime will retrieve from the settings and update the date and time.
        setDateTime();
        while(sendingJson){
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        // Notify the client.
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

/// @brief This event is used by the websocket object when a message is received from the client.
/// @param inbuf 
void ChatHandler::onMessage(WebsocketInputStreambuf * inbuf) {
    if(!server_ready)
        return;
    // Get the input message.
    std::ostringstream ss;
    std::string msg;
    ss << inbuf;
    msg = ss.str();
    // For debug purposes.
    Serial.println(msg.c_str());
    // By now this server only receives JSON strings over the websocket, so parse them. 
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

/// @brief This task runs a basic https server with websocket capabilities.
/// @param param 
void setupServer(void * param){
    server_ready = false;
    while(!wifi_got_ip){
        Serial.println("No IP address");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    // Before start a new server, close the socket.
    if(activeClient != NULL){
        activeClient->close();
        activeClient = nullptr;
    }
    
    Serial.println("Creating ssl certificate...");
    lv_label_set_text(frm_home_title_lbl, "Creating ssl certificate...");
    lv_label_set_text(frm_home_symbol_lbl, LV_SYMBOL_HOME);
    //vTaskDelay(1000 / portTICK_PERIOD_MS);
    SSLCert * cert;
    
    // Create a self signed certificate using the owner's id as part of the config.
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
    // Create the server object, port 443 and limit the clients;
    secureServer = new HTTPSServer(cert, 443, maxClients);
    
    // A resource is a file like index.html, style.css..., represented by a node.
    // The server sends the content of the string that holds the web page on index_html,
    // style_css and script_js. For now, the style and javascript is already on index.html.
    // nodeRoot is the equivalent as the index.html, the other nodes corresponds to the other files.
    ResourceNode * nodeRoot = new ResourceNode("/", "GET", &handleRoot);
    //ResourceNode * nodeStyle = new ResourceNode("/style.css", "GET", &handleStyle);
    //ResourceNode * nodeScript = new ResourceNode("/script.js", "GET", &handleScript);
    // If the client request a inexistent resource, send a 404 content.
    ResourceNode * node404 = new ResourceNode("", "GET", &handle404);
    //ResourceNode * nodeFav = new ResourceNode("/favicon.ico", "GET", &handleFav);
    // Register the nodes.
    secureServer->registerNode(nodeRoot);
    //secureServer->registerNode(nodeStyle);
    //secureServer->registerNode(nodeScript);
    secureServer->registerNode(node404);
    //secureServer->registerNode(nodeFav);
    // Create a websocket node (wss://server_address/chat on client side).
    WebsocketNode * chatNode = new WebsocketNode("/chat", ChatHandler::create);
    secureServer->registerNode(chatNode);
    secureServer->setDefaultNode(node404);
    // Adds browser authentication.
    secureServer->addMiddleware(&middlewareAuthentication);
    secureServer->addMiddleware(&middlewareAuthorization);
    // Start the service.
    secureServer->start();
    if (secureServer->isRunning()) {
        // If all good, show a button with 'https' on settings, see wifi section.
        lv_obj_clear_flag(frm_settings_wifi_http_btn, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(frm_home_title_lbl, "Server ready.");
        lv_label_set_text(frm_home_symbol_lbl, LV_SYMBOL_HOME);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        Serial.println("Server ready.");
        lv_label_set_text(frm_home_title_lbl, "");
        lv_label_set_text(frm_home_symbol_lbl, "");
        server_ready = true;
    }
    // In case when a external routine ends this task.
    if(param != NULL)
        vTaskDelete(NULL);
}
/// @brief Close all sockets and close the HTTPSServer.
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
            // Hide the https button on settings.
            lv_obj_add_flag(frm_settings_wifi_http_btn, LV_OBJ_FLAG_HIDDEN);
            Serial.println("Server ended.");
            server_ready = false;
        }
    //vTaskDelete(NULL);
}

bool non_printable_chars(const char *str) {
    for(uint8_t i = 0; i < 6; i++){ 
        if (!isalnum((unsigned char)str[i])) { 
            return true; 
        }
    }
    return false; 
}

/// @brief Updates de rssi and snr graph widget on home screen
/// @param rssi 
/// @param snr 
void update_rssi_snr_graph(float rssi, float snr){
    lv_chart_set_next_value(frm_home_rssi_chart, frm_home_rssi_series, rssi);
    lv_chart_set_next_value(frm_home_rssi_chart, frm_home_snr_series, snr);
    lv_chart_refresh(frm_home_rssi_chart);
}

/// @brief Collects lora packets and save them in received_packets.
/// @param param 
void collectPackets(void * param){
    void * packet = NULL;
    lora_packet * p;
    lora_packet lp;
    uint16_t packet_size;
    bool invalid_pkt_size = false;

    while(true){
        if(gotPacket){
            packet = calloc(1, sizeof(lora_packet));
            if(packet){
                // Get exclusive access to the radio module and read the payload.
                if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
                    radio.standby();
                    packet_size = radio.getPacketLength();
                    radio.readData((uint8_t*)packet, packet_size);
                    rssi = radio.getRSSI();
                    snr = radio.getSNR();
                    gotPacket = false;
                    // Put the radio to listen.
                    radio.startReceive();
                    xSemaphoreGive(xSemaphore);
                }

                // Check if the packet is valid.
                if(packet_size == sizeof(lora_packet_ack) || packet_size == sizeof(lora_packet_announce) ||
                    packet_size == sizeof(lora_packet_comm) || packet_size == sizeof(lora_packet_data) || 
                    packet_size == sizeof(lora_packet_ping)){
                    invalid_pkt_size = false;
                }
                else{
                    invalid_pkt_size = true;
                    Serial.printf("Unknown packet size - %d bytes\n", packet_size);
                }
                // A pointer justo to get the packet type
                p = (lora_packet*)packet;
                // Common properties to all packet types
                lp.type = p->type;
                strcpy(lp.id, p->id);
                strcpy(lp.sender, p->sender);
                // Especific packet type properties
                switch(p->type){
                    case LORA_PKT_ANNOUNCE:
                        lp.hops = ((lora_packet_announce*)packet)->hops;
                        break;
                    case LORA_PKT_ACK:
                        strcpy(lp.destiny, ((lora_packet_ack*)packet)->destiny);
                        strcpy(lp.status, ((lora_packet_ack*)packet)->status);
                        lp.app_id = ((lora_packet_ack*)packet)->app_id;
                        break;
                    case LORA_PKT_DATA:
                        strcpy(lp.destiny, ((lora_packet_data*)packet)->destiny);
                        lp.data_size = ((lora_packet_data*)packet)->data_size;
                        memcpy(lp.data, ((lora_packet_data*)packet)->data, ((lora_packet_data*)packet)->data_size);
                        lp.app_id = ((lora_packet_data*)packet)->app_id;
                        // Date time of arrival.
                        strftime(lp.date_time, sizeof(lp.date_time)," - %a, %b %d %Y %H:%M", &timeinfo);
                        if(calculate_data_crc(((lora_packet_data*)packet)->data, 208) == ((lora_packet_data*)packet)->crc){
                            lp.crc = ((lora_packet_data*)packet)->crc;
                        }
                        else{
                            Serial.printf("Data crc mismatch\n");
                            invalid_pkt_size = true;
                        }
                        break;
                    default:
                        Serial.printf("Packet type %d unknown\n", lp.type);
                        invalid_pkt_size = true;
                        break;
                }

                // If the size of ID is abnormal
                if(sizeof(lp.id) != 7){
                    Serial.printf("Packet ID abnormal\n");
                    invalid_pkt_size = true;
                }

                if(non_printable_chars(lp.id) || non_printable_chars(lp.sender)){
                    Serial.printf("Packet ID %s or sender %s corrupted\n", lp.id, lp.sender);
                    invalid_pkt_size = true;
                }
                
                if(lp.app_id != APP_DISCOVERY)
                    if( lp.app_id != APP_LORA_CHAT)
                        if(lp.app_id != APP_SYSTEM) 
                            if(lp.app_id != APP_TICTACTOE){
                                if(lp.type != LORA_PKT_ANNOUNCE){
                                    Serial.printf("APP_ID %d unknown\n", lp.app_id);
                                    invalid_pkt_size = true;
                                }
                }
                // Save the packet id on received_packets.
                if(!invalid_pkt_size && strcmp(lp.sender, user_id) != 0){
                    pthread_mutex_lock(&lvgl_mutex);
                    activity(lv_color_hex(0x00ff00));
                    pthread_mutex_unlock(&lvgl_mutex);
                    if(lp.type == LORA_PKT_DATA || lp.type == LORA_PKT_ACK){
                        //Serial.printf("ID %s\nAPP ID %d\nTYPE %d\nSENDER %s\nSTATUS %s\nDATA SIZE %d\nDATA %s\n\n", lp.id, lp.app_id, lp.type, lp.sender, lp.status, lp.data_size, lp.data);
                    }
                    new_stats = true;
                    update_rssi_snr_graph(rssi, snr);
                    if(lp.hops > 0){
                        // Decrement the TTL
                        lp.hops--;
                        // If we receive the same packet we transmitted, drop it. This is a thing that happen
                        // as soon as we send a packet. The radio uses the same buffer to transmit and receive.
                        if(!pkt_history.exists(lp.id)){
                            pkt_history.add(lp.id);
                            pkt_list.add(lp);
                            //Serial.println("\nUpdating rssi graph...");
                            //Serial.println("rssi graph updated.");
                            // If the packet belongs to other, retransmit it
                            if((strcmp(lp.destiny, user_id) != 0 || lp.type == LORA_PKT_ANNOUNCE) && lp.hops > 0){
                                if(!non_printable_chars(lp.id)){
                                    Serial.printf("Redirecting packet %s\n", lp.id);
                                    lp.confirmed = true;
                                    transmit_pkt_list.add(lp);
                                }
                                else
                                    Serial.printf("Packet ID %s corrupted\n");
                            }
                        }
                        else{
                            if(lp.type == LORA_PKT_DATA){
                                lora_packet ack;
                                ack.type = LORA_PKT_ACK;
                                //Serial.printf("collectPackets - p.app_id %d\n");
                                ack.app_id = lp.app_id;
                                strcpy(ack.id, generate_ID(6).c_str());
                                strcpy(ack.sender, user_id);
                                strcpy(ack.status, lp.id);
                                transmit_pkt_list.add(ack);
                                Serial.printf("Retransmitting ACK app_id %d to %s\n", ack.app_id, lp.id);
                            }
                            //Serial.printf("Packet %s already received\n", lp.id);
                        }
                    }
                    else
                        Serial.printf("Packet %s dropped\ntype %d\n", lp.id, lp.type);
                    if(packet){
                        free(packet);
                        packet = NULL;
                    }
                }else{
                    if(strcpy(lp.sender, user_id) != 0){
                        pthread_mutex_lock(&lvgl_mutex);
                        activity(lv_color_hex(0x000000));
                        pthread_mutex_unlock(&lvgl_mutex);
                    }
                }
            }
            else{
                Serial.println("collectPackets - packet NULL");
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void processPackets2(void * param){
    lora_packet p;
    ContactMessage cm;
    Contact * c = NULL;

    while(true){
        if(pkt_list.has_packets()){
            // Change the squared status on home screen to green.
            pthread_mutex_lock(&lvgl_mutex);
            activity(lv_color_hex(0x00ff00));
            pthread_mutex_unlock(&lvgl_mutex);
            p = pkt_list.get();
            if(p.type == LORA_PKT_ANNOUNCE){
                // Save the node ID in the discovery list
                discovery_node dn = discovery_node(p.sender, MAX_HOPS - p.hops, 0, 0);
                if(discoveryApp.add(dn)){
                    //Serial.printf("Node ID %s added to discovery list\n", dn.gridLocalization.node_id);
                }
                else{
                    //Serial.printf("Node ID %s already exists in discovery list\n", dn.gridLocalization.node_id);
                }
                // We need to know who is saying Hi! If is on our contact list, we'll update his status, if not, drop it.
                Contact * c = contacts_list.getContactByID(p.sender);
                if(c != NULL){
                    // Set this to true if the contact is in range.
                    c->inrange = true;
                    // There's a time out in minutes, if the contacts don't send a "show" status in time they will be shown as out of range with a greyish squared mark after their names.
                    c->timeout = millis();
                    //Serial.println("Announcement packet received");
                    c = NULL;
                }else{// If not in contact_list, go to the discovery service.
                    Serial.printf("Discovery service  - ID %s\n", p.id);
                }
            }
            else if(p.type == LORA_PKT_DATA){
                // Create a ack packet
                Serial.printf("DATA packet received p.app_id %d\n", p.app_id);
                lora_packet ack;
                ack.app_id = p.app_id;
                strcpy(ack.id, generate_ID(6).c_str());
                strcpy(ack.sender, user_id);
                strcpy(ack.destiny, p.destiny);
                strcpy(ack.status, p.id);
                // Put on the transmit queue
                transmit_pkt_list.add(ack);
                // Redirect the data to its application
                if(p.app_id == APP_LORA_CHAT)
                    Serial.println("APP_LORA_CHAT");
                else if(p.app_id == APP_DISCOVERY)
                    Serial.println("APP_DISCOVERY");
                else if(p.app_id == APP_SYSTEM)
                    Serial.println("APP_SYSTEM");
                else
                    Serial.println("APP_ID UNKNOWN");

                if(p.app_id == APP_LORA_CHAT){
                    Serial.println("LoRa Chat packet");
                    c = contacts_list.getContactByID(p.sender);
                    if(c){
                        strcpy(cm.messageID, generate_ID(6).c_str());
                        // Decrypt the message.
                        unsigned char decrypted_text[p.data_size + 1] = {'\0'};
                        decrypt_text((unsigned char *)p.data, (unsigned char*)c->getKey().c_str(), p.data_size, decrypted_text);
                        Serial.printf("msg size => %d\n", p.data_size);
                        Serial.printf("Decrypted => %s\n", decrypted_text);
                        strcpy(cm.message, (const char *)decrypted_text);
                        strcpy(cm.dateTime, p.date_time);
                        pthread_mutex_lock(&messages_mutex);
                        c->addMessage(cm);
                        pthread_mutex_unlock(&messages_mutex);
                        sendContactMessages(p.sender);
                        while(sendingJson){
                            vTaskDelay(100 / portTICK_PERIOD_MS);
                        }
                        // On client side there is a javascript function that reproduces a sound when a message is received.
                        pthread_mutex_lock(&send_json_mutex);
                        sendJSON("{\"command\" : \"playNewMessage\"}");
                        pthread_mutex_unlock(&send_json_mutex);
                        // Now w prepare a string to be shown on the notification area.
                        char message[208] = {'\0'};
                        char cmsg[208] = {'\0'};
                        strcpy(message, c->getName().c_str());
                        strcat(message, ": ");
                        // This ensure the message is 149 bytes long.
                        if(sizeof(p.data) > 207){
                            memcpy(cmsg, cm.message, 207);
                            strcat(message, cmsg);
                        }
                        else
                            strcat(message, cm.message);
                        // Eclipse the message if bigger than 30 bytes.
                        message[30] = '.';
                        message[31] = '.';
                        message[32] = '.';
                        message[33] = '\0';
                        // Add to the notification list, there is a task to process it.
                        notification_list.add(message, LV_SYMBOL_ENVELOPE);
                    }
                    else{
                        Serial.printf("Contact ID %s not found\n", p.sender);
                    }
                }
            }
            else if(p.type == LORA_PKT_ACK){
                
                if(p.app_id == APP_LORA_CHAT){
                    Serial.printf("Received an ACK from %s to message ID %s\n", p.sender, p.status);
                    Contact * c = contacts_list.getContactByID(p.sender);
                    if(c){
                        Serial.printf("ACK to %s\n", c->getName());
                        ContactMessage * cm = c->getMessageByID(p.status);
                        if(cm){
                            cm->ack = true;
                            msg_confirmed = true;
                            if(!transmit_pkt_list.del(p.status))
                                Serial.printf("Couldn't delete packet from transmit list\n");
                            Serial.printf("ACK confirmed to message ID %s\n", cm->messageID);
                        }
                        else{
                            Serial.printf("Message ID %s not found in contact's messages\n", p.status);
                        }
                    }
                    else{
                        Serial.printf("Contact ID %s not found in contact list\n", p.sender);
                    }
                }
                else if(p.app_id == APP_DISCOVERY){

                }
                else{
                    Serial.println("process_packets2 - APP_ID UNKNOWN");
                }
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static int16_t transmit(uint8_t * data, size_t len){
    int16_t r;

    transmiting = true;
    pthread_mutex_lock(&lvgl_mutex);
    activity(lv_color_hex(0xff0000));
    pthread_mutex_unlock(&lvgl_mutex);
    if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
        //Serial.println("Transmitting packet...");
        if(!gotPacket){
            transmit_pkt_list.setOnAirTime(radio.getTimeOnAir(len) / 1000);
            r = radio.startTransmit((uint8_t*)data, len);
        }
        else
            r = RADIOLIB_ERR_TX_TIMEOUT;
        xSemaphoreGive(xSemaphore);
    }
    if(r == RADIOLIB_ERR_NONE){
        //Serial.println("Packet sent");
        //Serial.printf("On Air time => %1.3fs\n", (float)radio.getTimeOnAir(len) / 1000000);
    }
    else
        Serial.println("Packet not sent");
        
    transmiting = false;
    return r;
}

void processTransmitingPackets(void * param){  
    uint32_t r = 100;
    while(true){
        if(!using_transmit_pkt_list){
            using_transmit_pkt_list = true;
            lora_packet p = transmit_pkt_list.check_packets();
            using_transmit_pkt_list = false;
            //if(p.type != LORA_PKT_EMPTY)
            //    Serial.printf("%s -> %s\n%d\n%d\n", p.sender, p.destiny, p.type, p.hops);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

/// @brief Gets the status of the lora radio and send them to the web client
/// @param param 
void processReceivedStats(void * param){
    string json;
    JsonDocument doc;
    char rssi_text[7] = {'\0'}, snr_text[7] = {'\0'};

    while (true){
        if(new_stats){
            sprintf(rssi_text, "%.2f", rssi);
            sprintf(snr_text, "%.2f", snr);
            // Lets print this statistics on console.
            //Serial.printf("[RSSI:%.2f dBm SNR:%.2f dBm]\n", rssi, snr);
            // The client side has a javascript to plot a graphic about the rssi and s/n ratio. So we send it also as JSON.
            doc["command"] = "rssi_snr";
            doc["rssi"] = rssi_text;
            doc["snr"] = snr_text;
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
            new_stats = false;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

/// @brief This is called every time the radio gets a packet, see radio.setPacketReceivedAction(onListen).
void onListen(){
    gotPacket = true;
    transmiting = false;
}
/// @brief The same thing as above but when the radio finishes a transmission. By now, not used.
void onTransmit(){
    transmiting = false;
}
/// @brief This sets a more modest configuration for the radio, less power, more broadband, less distance, faster.
/// @return bool
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
        if (radio.setCRC(true) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
            Serial.println(F("Selected CRC is invalid for this module!"));
            return false;
        }
        xSemaphoreGive(xSemaphore);
    }
    return true;
}
/// @brief This sets a more performance like configuration for the radio, more power, less broadband, more distance, slower.
/// @return bool
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
        if (radio.setCodingRate(5) == RADIOLIB_ERR_INVALID_CODING_RATE) {
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
        if (radio.setCRC(true) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
            Serial.println(F("Selected CRC is invalid for this module!"));
            return false;
        }
        xSemaphoreGive(xSemaphore);
    }
    return true;
    
}

/// @brief This is the initial setup as soon as the board boots.
/// @param e 
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
    if (radio.setOutputPower(22) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
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
    if (radio.setCRC(true) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
        Serial.println(F("Selected CRC is invalid for this module!"));
        //return false;
    }
    int32_t code = 0;
    
    // The function onListen will be called every time a packet is received.
    radio.setPacketReceivedAction(onListen);
    // The onTransmit function is called every time when the radio finishes a transmission.
    //radio.setPacketSentAction(onTransmit);
    // Put the radio module to listen for LoRa packets.
    code = radio.startReceive();
    Serial.print("setup radio start receive code ");
    Serial.println(code);
    // Check if the radio is up.
    if(code == RADIOLIB_ERR_NONE){
        // This can be used everywhere to check if the radio module is ready.
        hasRadio = true;
    }
    //return true;

}

/// @brief This sends a ping packet.
/// @param e 
void ping(lv_event_t * e){
    /*
    // Lets send a simple byte
    while(gotPacket)
        delay(10 / portTICK_PERIOD_MS);
    // Get exclusive access through SPI.
    transmiting = true;
    // Using a SDR radio and SDR++ set the radio to listen to 915 MHz, put this device on DX mode,
    // touch 'ping' button and watch the waterfall ou record the screen to analyse further more.
    if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
        if(radio.transmit((uint8_t*)'A', 1) == RADIOLIB_ERR_NONE){
            Serial.println("Ping sent");
        }
        else
            Serial.println("Ping not sent");
        xSemaphoreGive(xSemaphore);
    }
    transmiting = false;
    */
}
/// @brief This hides the contact list.
/// @param e 
void hide_contacts_frm(lv_event_t * e){
    // Get the event type.
    lv_event_code_t code = lv_event_get_code(e);
    // If the event is whats we're expecting, set the hidden flag on the window object.
    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_contacts != NULL){
            lv_obj_add_flag(frm_contacts, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
/// @brief This shows the contact list.
/// @param e 
void show_contacts_form(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_contacts != NULL){
            lv_obj_clear_flag(frm_contacts, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
/// @brief This hides the settings menu.
/// @param e 
void hide_settings(lv_event_t * e){
    // Get the event code.
    lv_event_code_t code = lv_event_get_code(e);
    // Get the button the user clicked.
    lv_obj_t * obj = lv_event_get_target(e);
    // Get the menu. Could be more menus, se we need to know what is used.
    lv_obj_t * menu = (lv_obj_t*)lv_event_get_user_data(e);
    bool isroot = false;
    if(code == LV_EVENT_SHORT_CLICKED){
        // We need to know if the button on menu is the one which closes it.
        isroot = lv_menu_back_btn_is_root(menu, obj);
        if(isroot) {
            // The ID section of menu has no save button. We save the ID info when we close
            // the menu. First, we need to update user_name, user_id and user_key.
            // Copy the name of the user to user_name.
            strcpy(user_name, lv_textarea_get_text(frm_settings_name));
            // Copy the ID of the user to user_id.
            strcpy(user_id, lv_textarea_get_text(frm_settings_id));
            // Copy the ky of the user to user_key.
            strcpy(user_key, lv_textarea_get_text(frm_settings_key));
            // Copy the http server admin password
            strcpy(http_admin_pass, lv_textarea_get_text(frm_settings_admin_password));
            // Hides the menu.
            lv_obj_add_flag(frm_settings, LV_OBJ_FLAG_HIDDEN);
            // Save the modified data.
            saveSettings();
        }
    }
}
/// @brief Shows the settings menu with pre loaded info.
/// @param e 
void show_settings(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    // Used on text areas on Date tab.
    char year[5], month[3], day[3], hour[3], minute[3], color[7];

    if(code == LV_EVENT_SHORT_CLICKED){
        // Show the menu.
        lv_obj_clear_flag(frm_settings, LV_OBJ_FLAG_HIDDEN);
        // Load the system settings.
        loadSettings();
        // Get the date and time segments from the RTC and convert them to string.
        itoa(timeinfo.tm_year + 1900, year, 10);
        itoa(timeinfo.tm_mon + 1, month, 10);
        itoa(timeinfo.tm_mday, day, 10);
        itoa(timeinfo.tm_hour, hour, 10);
        itoa(timeinfo.tm_min, minute, 10);
        // Set the date and time segments strings on the text areas of the Date tab in Settings..
        lv_textarea_set_text(frm_settings_year, year);
        lv_textarea_set_text(frm_settings_month, month);
        lv_textarea_set_text(frm_settings_day, day);
        lv_textarea_set_text(frm_settings_hour, hour);
        lv_textarea_set_text(frm_settings_minute, minute);
        lv_textarea_set_text(frm_settings_name, user_name);
        lv_textarea_set_text(frm_settings_id, user_id);
        lv_textarea_set_text(frm_settings_key, user_key);
        lv_textarea_set_text(frm_settings_admin_password, http_admin_pass);
        // Convert the color from int to his HEX representation as string.
        sprintf(color, "%lX", ui_primary_color);
        // Set the color text area with the HEX string.
        lv_textarea_set_text(frm_settings_color, color);
        // Set the brightness roller object value.
        lv_roller_set_selected(frm_settings_brightness_roller, brightness, LV_ANIM_OFF);
    }
}
/// @brief Hides the add contact dialog.
/// @param e 
void hide_add_contacts_frm(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_add_contact != NULL){
            lv_obj_add_flag(frm_add_contact, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
/// @brief Shows the add contact dialog.
/// @param e 
void show_add_contacts_frm(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_add_contact != NULL){
            lv_obj_clear_flag(frm_add_contact, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
/// @brief This alters the info of a contact pointed by actual_contact.
/// @param e 
void hide_edit_contacts(lv_event_t * e){
    const char * name, * id, * key;
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_edit_contacts != NULL){
            // Get name and ID from the text areas.
            name = lv_textarea_get_text(frm_edit_text_name);
            id = lv_textarea_get_text(frm_edit_text_ID);
            key = lv_textarea_get_text(frm_edit_text_key);
            // We only do alter the info a contact if the fields ars not empty, otherwise print a warnig.
            if(strcmp(name, "") != 0 && strcmp(id, "") != 0){
                // Set the ID, key and name on the contact pointed by actual_contact.
                actual_contact->setID(id);
                actual_contact->setName(name);
                actual_contact->setKey(key);
                Serial.println("ID and name updated");
                // Lets point this out of the contacts list.
                actual_contact = NULL;
                // Hide the edit dialog.
                lv_obj_add_flag(frm_edit_contacts, LV_OBJ_FLAG_HIDDEN);
                Serial.println("Contact updated");
                // Rebuild the contacts list(contacts list dialog).
                refresh_contact_list();
                // Save the contacts list.
                saveContacts();
            }else
                Serial.println("Name or ID cannot be empty");
        }
    }
}
/// @brief Shows the edit contact info dialog when press and hold a contact's name on the list.
/// @param e 
void show_edit_contacts(lv_event_t * e){
    char id[7] = {'\u0000'};
    // Get the code of the event.
    lv_event_code_t code = lv_event_get_code(e);
    // Get the data object represented by the item of the list(button).
    lv_obj_t * btn = (lv_obj_t * )lv_event_get_user_data(e);
    // Extract the hidden id label.
    lv_obj_t * lbl_id = lv_obj_get_child(btn, 2);

    if(code == LV_EVENT_LONG_PRESSED){
        if(frm_edit_contacts != NULL){
            // Get the id saved on the label.
            strcpy(id, lv_label_get_text(lbl_id));
            // Search for the contact using his id. This pointer allow us to modify the info was soon as we close the dialog.
            actual_contact = contacts_list.getContactByID(id);
            // Show the edit dialog.
            lv_obj_clear_flag(frm_edit_contacts, LV_OBJ_FLAG_HIDDEN);
            // Populate the text areas with ID and name of the actual_contact.
            lv_textarea_set_text(frm_edit_text_name, actual_contact->getName().c_str());
            lv_textarea_set_text(frm_edit_text_ID, actual_contact->getID().c_str());
            lv_textarea_set_text(frm_edit_text_key, actual_contact->getKey().c_str());
        }
    }
}
/// @brief Hides the chat dialog and delete the task_check_new_msg.
/// @param e 
void hide_chat(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_chat != NULL){
            if(task_check_new_msg != NULL){
                // Closes the task.
                vTaskDelete(task_check_new_msg);
                task_check_new_msg = NULL;
                Serial.println("task_check_new_msg deleted");
            }
            // Set this counter to 0.
            msg_count = 0;
            // Hide the chat dialog.
            lv_obj_add_flag(frm_chat, LV_OBJ_FLAG_HIDDEN);
            // This is temporary, closing the chat returns to the contact list and the contact continues selected
            // to be used to ping to him.
            //actual_contact = NULL;
        }
    }
}
/// @brief Show a chat dialog when selecting a contact.
/// @param e 
void show_chat(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_SHORT_CLICKED){
        // Get the entire button from the list.
        lv_obj_t * btn = (lv_obj_t *)lv_event_get_user_data(e);
        // Get its label with the contact's name.
        pthread_mutex_lock(&lvgl_mutex);
        lv_obj_t * lbl = lv_obj_get_child(btn, 1);
        pthread_mutex_unlock(&lvgl_mutex);
        // Get the contact's ID label.
        pthread_mutex_lock(&lvgl_mutex);
        lv_obj_t * lbl_id = lv_obj_get_child(btn, 2);
        pthread_mutex_unlock(&lvgl_mutex);
        // Extract the name and ID.
        const char * name = lv_label_get_text(lbl);
        const char * id = lv_label_get_text(lbl_id);
        // Get a pointer to the contact by his ID.
        actual_contact = contacts_list.getContactByID(id);
        // Biuld a string title.
        char title[60] = "Chat with ";
        strcat(title, name);
        if(frm_chat != NULL){
            // Show the chat dialog.
            lv_obj_clear_flag(frm_chat, LV_OBJ_FLAG_HIDDEN);
            // Set the title.
            lv_label_set_text(frm_chat_btn_title_lbl, title);
            // Send the focus to the answer text area.
            lv_group_focus_obj(frm_chat_text_ans);
            
            if(task_check_new_msg == NULL){
                // Load a task which watches for a new message related to the selected contact.
                xTaskCreatePinnedToCore(check_new_msg, "check_new_msg", 11000, NULL, 1, &task_check_new_msg, 1);
                Serial.println("task_check_new_msg created");
            }
            Serial.print("actual contact is ");
            Serial.println(actual_contact->getName());
        }
    }
}

/// @brief Transmits a message through the LoRa module. Used with the chat dialog.
/// @param e 
void send_message(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    char msg[208] = {'\0'};

    if(code == LV_EVENT_SHORT_CLICKED){
        // Lets assemble a lora packet.
        lora_packet pkt;
        // Set the lora_packet type to DATA
        pkt.type = LORA_PKT_DATA;
        // Set the app id
        pkt.app_id = APP_LORA_CHAT;
        // The sender is the owner.
        strcpy(pkt.sender, user_id);
        // The destiny is the selected contact.
        strcpy(pkt.destiny, actual_contact->getID().c_str());
        // 'send' to a normal packet, we'll expect to receive a 'recv' packet from the contact(not guaranteed).
        strcpy(pkt.status, "send");
        // Put the actual date and time.
        strftime(pkt.date_time, sizeof(pkt.date_time)," - %a, %b %d %Y %H:%M", &timeinfo);
        
        if(hasRadio){
            // We won't send an empty message.
            if(strcmp(lv_textarea_get_text(frm_chat_text_ans), "") != 0){
                // Get the message from the answer text area.
                strcpy(msg, lv_textarea_get_text(frm_chat_text_ans));
                // Encrypt the message with our key.
                uint8_t text_length = strlen(msg);
                uint8_t padded_len = ((text_length + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
                unsigned char ciphertext[padded_len];
                encrypt_text((unsigned char *)msg, (unsigned char *)user_key, text_length, ciphertext);
                memcpy(pkt.data, ciphertext, padded_len);
                strcpy(pkt.id, generate_ID(6).c_str());
                pkt.data_size = padded_len;
                transmit_pkt_list.add(pkt);
                // add the message to the contact's list of messages.
                // This is used when we are assembling the chat messages list, every time we found me = true we are 
                // creating a new button with the messages created by us with the title 'Me - Date time'. 
                Contact * c = contacts_list.getContactByID(pkt.destiny);
                if(c != NULL){
                    ContactMessage cm = ContactMessage();
                    strcpy(cm.dateTime, pkt.date_time);
                    strcpy(cm.message, msg);
                    strcpy(cm.messageID, pkt.id);
                    cm.rssi = 0;
                    cm.snr = 0;
                    cm.me = true;
                    Serial.print("Adding answer to ");
                    Serial.println(pkt.destiny);
                    Serial.println(pkt.data);
                    // Get exclusive access to the addMessage.
                    pthread_mutex_lock(&messages_mutex);
                    c->addMessage(cm);
                    pthread_mutex_unlock(&messages_mutex);
                    // Clear the asnwer text area, if fails for some reason you don't need to retype.
                    lv_textarea_set_text(frm_chat_text_ans, "");
                }
            }
            else{
                Serial.println("send_message() - radio not configured.");
            }
        }
    }
}

/// @brief This is an event driven by touch and hold over a message on the messages list. It will copy the message to the
// answer text area, avoiding retyping on that tini tiny keyboard.
/// @param e 
void copy_text(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * lbl = (lv_obj_t *)lv_event_get_user_data(e);
    if(code == LV_EVENT_LONG_PRESSED){
        lv_textarea_add_text(frm_chat_text_ans, lv_label_get_text(lbl));
    }
}

/// @brief Task that runs when a contact is selected through the contacts list. It updates the messages list
/// object on the chat.
/// @param param 
void check_new_msg(void * param){
    // Vector that holds the contact's mesages.
    vector<ContactMessage> cm;
    // Save the actual messages count.
    uint32_t actual_count = 0;
    lv_obj_t * btn = NULL, * lbl = NULL;
    char date[60] = {'\0'};
    char name[100] = {'\0'};
    
    while(true){
        // Get the contact's messages on a vector.
        pthread_mutex_lock(&messages_mutex);
        cm = contacts_list.getContactMessages(actual_contact->getID().c_str());
        pthread_mutex_unlock(&messages_mutex);
        // Save the count.
        actual_count = (cm).size();
        // When actual_count is bigger than msg_count means that we have new messages.
        
        if(actual_count > msg_count || msg_confirmed == true){
            Serial.println("new messages");
            // Clear the chat messages. LVGL documentation says the functions are not thread safe, so we need a mutex.
            pthread_mutex_lock(&lvgl_mutex);
            lv_obj_clean(frm_chat_list);
            pthread_mutex_unlock(&lvgl_mutex);
            for(uint32_t i = 0; i < (cm).size(); i++){
                // We create a new entry on the messages list based on sender and destiny messages.
                // If the message was sent by us we'll create a button with 'Me date time' on title.
                if((cm)[i].me){
                    // The title 'Me date time'.
                    strcpy(name, "Me");
                    strcat(name, (cm)[i].dateTime);
                    // Add on the list a simple text, there's a visual difference between a button.
                    pthread_mutex_lock(&lvgl_mutex);
                    lv_list_add_text(frm_chat_list, name);
                    pthread_mutex_unlock(&lvgl_mutex);
                }else{
                    // If it is a message from the destination, create a text item with the title 'Contact name date time'.
                    strcpy(name, actual_contact->getName().c_str());
                    strcat(name, (cm)[i].dateTime);
                    // LVGL's unsave thread access functions.
                    pthread_mutex_lock(&lvgl_mutex);
                    lv_list_add_text(frm_chat_list, name);
                    pthread_mutex_unlock(&lvgl_mutex);
                    
                }
                Serial.println(name);
                Serial.println((cm)[i].message);
                
                // Crete a new instance on a button with the message.
                pthread_mutex_lock(&lvgl_mutex);
                btn = lv_list_add_btn(frm_chat_list, NULL, (cm)[i].message);
                //pthread_mutex_unlock(&lvgl_mutex);
                // Get the label of the button which holds the message.
                //pthread_mutex_lock(&lvgl_mutex);
                lbl = lv_obj_get_child(btn, 0);
                //pthread_mutex_unlock(&lvgl_mutex);
                // Set the font type, this one includes accents and latin chars.
                //pthread_mutex_lock(&lvgl_mutex);
                lv_obj_set_style_text_font(lbl, &ubuntu, LV_PART_MAIN | LV_STATE_DEFAULT);
                //pthread_mutex_unlock(&lvgl_mutex);
                // Add the event 'copy message to answer text area'.
                //pthread_mutex_lock(&lvgl_mutex);
                lv_obj_add_event_cb(btn, copy_text, LV_EVENT_LONG_PRESSED, lv_obj_get_child(btn, 0));
                //pthread_mutex_unlock(&lvgl_mutex);
                // This configures the label to do a word wrap and hide the scroll bars in case the message is too long.
                //pthread_mutex_lock(&lvgl_mutex);
                lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
                //pthread_mutex_unlock(&lvgl_mutex);
                if((cm)[i].ack){
                    //pthread_mutex_lock(&lvgl_mutex);
                    btn = lv_list_add_btn(frm_chat_list, LV_SYMBOL_OK, "");
                    //pthread_mutex_unlock(&lvgl_mutex);
                }
                // Force a scroll to the last message added on the list.
                //pthread_mutex_lock(&lvgl_mutex);
                lv_obj_scroll_to_view(btn, LV_ANIM_OFF);
                pthread_mutex_unlock(&lvgl_mutex);
            }
            // Update the message count.
            msg_count = actual_count;
            msg_confirmed = false;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
/// @brief This event adds a contact to the contacts list.
/// @param e 
void add_contact(lv_event_t * e){
    const char * name, * id, * key;
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_SHORT_CLICKED){
        // Get the name, ID and key from the text areas.
        id = lv_textarea_get_text(frm_add_contact_textarea_id);
        name = lv_textarea_get_text(frm_add_contact_textarea_name);
        key = lv_textarea_get_text(frm_add_contact_textarea_key);
        // These fields cannot be empty, otherwise do nothing.
        if(strcmp(name, "") != 0 && strcmp(id, "") != 0){
            // Create a contact.
            Contact c = Contact(name, id);
            c.setKey(key);
            // Verify if exists someone with the same ID. If not, add it.
            if(!contacts_list.find(c)){
                if(contacts_list.add(c)){
                    Serial.println("Contact added");
                    // Rebuild the contacts list dialog.
                    refresh_contact_list();
                    // Hide the add dialog.
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
/// @brief This event deletes a contact from the list.
/// @param e 
void del_contact(lv_event_t * e){
    char id[7] = {'\u0000'};
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        // Get the ID from the text area.
        strcpy(id, lv_textarea_get_text(frm_edit_text_ID));
        // We'll continue only if the ID is not empty.
        if(strcmp(id, "") != 0){
            // Search for the contact using his ID.
            Contact * c = contacts_list.getContactByID(id);
            if(c != NULL){
                // Delete the contact.
                if(contacts_list.del(*c)){
                    // Rebuild the list dialog.
                    refresh_contact_list();
                    // Hide the delete dialog.
                    lv_obj_add_flag(frm_edit_contacts, LV_OBJ_FLAG_HIDDEN);
                    Serial.println("Contact deleted");
                }
            }else
                Serial.println("Contact not found");
        }
    }else
        Serial.println("ID is empty");
}
/// @brief This event calls for generate_ID and set the string returned into the text area.
/// @param e 
void generateID(lv_event_t * e){
    uint8_t size = (int)lv_event_get_user_data(e);
    lv_textarea_set_text(frm_settings_id, generate_ID(size).c_str());
}
/// @brief This event calls for generate_ID and set the string returned into the text area.
/// @param e 
void generateKEY(lv_event_t * e){
    uint8_t size = (int)lv_event_get_user_data(e);
    lv_textarea_set_text(frm_settings_key, generate_ID(size).c_str());
}

/// @brief This event checks the DX toggle switch and apply the apropriate configuration on the radio module.
/// @param e 
void DX(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_VALUE_CHANGED){
        // Verify the state of the switch. If checked, DX mode, if not, normal mode.
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
                notification_list.add("Normal mode failed", LV_SYMBOL_SETTINGS);
                Serial.println("Normal mode failed");
            }
        }
    }
}
/// @brief This task runs every minute. Updates the time and date widget.
/// @param timeStruct 
void update_time(void *timeStruct) {
    char hourMin[6];
    char date[12];
    while(true){
        if(getLocalTime((struct tm*)timeStruct)){
            // Format the time like 00:00
            strftime(hourMin, 6, "%H:%M %p", (struct tm *)timeStruct);
            lv_label_set_text(frm_home_time_lbl, hourMin);
            // Format the date like Fri, Mar 15.
            strftime(date, 12, "%a, %b %d", (struct tm *)timeStruct);
            lv_label_set_text(frm_home_date_lbl, date);
        }
        vTaskDelay(5000 / portTICK_RATE_MS);
    }
    vTaskDelete(task_date_time);
}
/// @brief This function is used to set the time and date manually.
/// @param yr 
/// @param month 
/// @param mday 
/// @param hr 
/// @param minute 
/// @param sec 
/// @param isDst 
void setDate(int yr, int month, int mday, int hr, int minute, int sec, int isDst){
    timeinfo.tm_year = yr - 1900;   // Set date
    timeinfo.tm_mon = month-1;
    timeinfo.tm_mday = mday;
    timeinfo.tm_hour = hr;      // Set time
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = sec;
    timeinfo.tm_isdst = isDst;  // 1 or 0
    time_t t = mktime(&timeinfo);
    Serial.printf("Setting time: %s\n", asctime(&timeinfo));
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, NULL);
    notification_list.add("date & time updated", LV_SYMBOL_SETTINGS);
}
/// @brief This function gets the date time segments on settings, convert into int and set the date time on the RTC.
void setDateTime(){
    char day[3] = {'\0'}, month[3] = {'\0'}, year[5] = {'\0'}, hour[3] = {'\0'}, minute[3] = {'\0'};
    // Get the date and time segments from the text areas.
    strcpy(day, lv_textarea_get_text(frm_settings_day));
    strcpy(month, lv_textarea_get_text(frm_settings_month));
    strcpy(year, lv_textarea_get_text(frm_settings_year));
    strcpy(hour, lv_textarea_get_text(frm_settings_hour));
    strcpy(minute, lv_textarea_get_text(frm_settings_minute));
    
    // Check if the fields are not empty.
    if(strcmp(day, "") != 0 && strcmp(month, "") != 0 && strcmp(year, "") != 0 && strcmp(hour, "") != 0 && strcmp(minute, "") != 0){
        try{
            // Convert the strings into ints and set the date.
            setDate(atoi(year), atoi(month), atoi(day), atoi(hour), atoi(minute), 0, 0);
        }
        catch(exception &ex){
            // In case of error.
            Serial.println(ex.what());
        }
    }
}
/// @brief This is used in settings after the user inform the date and time parameters.
/// @param e 
void applyDate(lv_event_t * e){
    char hourMin[6];
    char date[12];
    // Set the date, format the text and show on the date time widget.
    setDateTime();
    strftime(hourMin, 6, "%H:%M %p", (struct tm *)&timeinfo);
    lv_label_set_text(frm_home_time_lbl, hourMin);
    strftime(date, 12, "%a, %b %d", (struct tm *)&timeinfo);
    lv_label_set_text(frm_home_date_lbl, date);
}
/// @brief This event changes the color of the objects.
/// @param e 
void apply_color(lv_event_t * e){
    // Get the hex representation of a 565 rgb color.
    strcpy(ui_primary_color_hex_text, lv_textarea_get_text(frm_settings_color));
    // If empty do nothing.
    if(strcmp(ui_primary_color_hex_text, "") == 0)
        return;
    // The hex representation is a string, convert to int.
    ui_primary_color = strtoul(ui_primary_color_hex_text, NULL, 16);
    // Get the default display object, we can configure various displays.
    lv_disp_t *dispp = lv_disp_get_default();
    // Create a theme based on the color.
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_color_hex(ui_primary_color), lv_palette_main(LV_PALETTE_RED), false, &lv_font_montserrat_14);
    // Apply the new theme, the changes happens instantaneously.
    lv_disp_set_theme(dispp, theme);
}
/// @brief This function verify the status of each contact on the list and change the status indicator accordingly.
/// @param index 
/// @param in_range 
void update_frm_contacts_status(uint16_t index, bool in_range){
    //No contacts, no update, return.
    if(index > contacts_list.size() - 1 || index < 0)
        return;
    // This gets the entire onject on the list of contacts by its position(index).
    pthread_mutex_lock(&lvgl_mutex);
    lv_obj_t * btn = lv_obj_get_child(frm_contacts_list, index);
    pthread_mutex_unlock(&lvgl_mutex);
    // Strangely this was causing loadProhibited, as the lvgl functions are not thread safe, we need to get exclusive access.
    // In other places could make the interface freeze, we need more study on this.
    pthread_mutex_lock(&lvgl_mutex);
    lv_obj_t * obj_status = lv_obj_get_child(btn, 3);
    pthread_mutex_unlock(&lvgl_mutex);
    // Based on the contact status passed through, change the color of the status indicator, green for near, 
    // light gray for out of range. You can modify at your will.
    if(in_range){
        // As the lvgl functions are not thread safe, we need to get exclusive access.
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
/// @brief This sends a json with a especific contact status to be updated on the contacts list on the client side.
/// @param id 
/// @param status 
void sendContactsStatusJson(const char * id, bool status){
    JsonDocument doc;
    std::string json;
    // Populate the doc.
    doc["command"] = "contact_status";
    doc["contact"]["id"] = id;
    doc["contact"]["status"] = status;
    // Convert to a json string.
    serializeJson(doc, json);
    while(sendingJson){
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    // Avoids parallel calls of sendJSON, memory corruption...
    pthread_mutex_lock(&send_json_mutex);
    sendJSON(json);
    pthread_mutex_unlock(&send_json_mutex);
}
/// @brief Loops through the contacts to analyse the statuses and sends them to the client side.
void check_contacts_in_range(){
    // Change the activity indicator to light blue.
    activity(lv_color_hex(0x0095ff));
    // Verify if someone got timeout.
    contacts_list.check_inrange();
    // Loop through the contacts.

    vector<Contact> * cl = contacts_list.getContactsList();
    for(uint32_t i = 0; i < (*cl).size(); i++){
        // Update the status indicator.
        update_frm_contacts_status(i, (*cl)[i].inrange);
        // For debug purposes.
        Serial.printf("check_contacts_in_range() - %s", (*cl)[i].getName());
        Serial.println((*cl)[i].inrange ? " is in range" : " is out of range");
        // Sends the current status to the client side.
        sendContactsStatusJson((*cl)[i].getID().c_str(), (*cl)[i].inrange);
    }
}
/// @brief Initialize the battery monitoring routine.
void initBat(){
    // Structure used to hold the characteristics related to ADC calibration.
    esp_adc_cal_characteristics_t adc_bat;
    // Calibrate the ADC (Analog-to-Digital Converter) for battery voltage measurement.
    // The esp32 has two adcs, ADC_UNIT_1 and ADC_UNIT_2.
    // ADC_ATTEN_DB_11 sets the attenuation level to 11dB, or voltage range 0-3.9v.
    // ADC_WIDTH_BIT_12 sets the resolution to 4096 voltage levels.
    // The reference voltage is 1.1v(1100).
    esp_adc_cal_value_t type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_bat);
    // Checks if we are using the reference voltage from eFuse, if not, 1.1 volts will be used.
    if (type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        vRef = adc_bat.vref;
    } else {
        vRef = 1100;
    }
}
/// @brief Same as the map function in arduino stuff.
/// @param x 
/// @param in_min 
/// @param in_max 
/// @param out_min 
/// @param out_max 
/// @return 
long mapv(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
/// @brief Gets the voltage readings from the adc1 and outputs as percentage. 4.2 volts means the battery is full, 3.3v is empty.
/// @return 
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
/// @brief This returns a string which represents the battery symbol, see lvgl's symbols table.
/// @param percentage 
/// @return 
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
/// @brief This task verify the battery level every 30 seconds.
/// @param param 
void update_bat(void * param){
    char icon[12] = {'\0'};
    uint32_t p = 0;
    char pc[4] = {'\0'};
    char msg[30]= {'\0'};
    string json;
    JsonDocument doc;

    while(true){
        p = read_bat();
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
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
/// @brief This event hides the wifi configuration dialog.
/// @param e 
void hide_wifi(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_wifi != NULL){
            lv_obj_add_flag(frm_wifi, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
/// @brief This event shows the wifi configuration dialog.
/// @param e 
void show_wifi(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_wifi != NULL){
            lv_obj_clear_flag(frm_wifi, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
/// @brief This event configures the wifi to connect to a network.
/// @param e 
void wifi_apply(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    // i is the index of the wifi_list where the selected network is.
    int i = (int)lv_event_get_user_data(e);
    uint32_t count = 0;
    char msg[100] = {'\0'};

    if(code == LV_EVENT_SHORT_CLICKED){
        // Check through some types of auth methods.
        if(wifi_list[i].auth_type == WIFI_AUTH_WPA2_PSK || 
           wifi_list[i].auth_type == WIFI_AUTH_WPA_WPA2_PSK || 
           wifi_list[i].auth_type == WIFI_AUTH_WEP){
            // Get the password.
            strcpy(wifi_list[i].pass, lv_textarea_get_text(frm_wifi_simple_ta_pass));
            if(strcmp(wifi_list[i].pass, "") == 0){
                Serial.println("Provide a password");
                return;
            }
            // Set the title.
            lv_label_set_text(frm_wifi_simple_title_lbl, "Connecting...");
            // Disconnect from previous network.
            WiFi.disconnect(true);
            // Set wifi mode to wireless client. 
            WiFi.mode(WIFI_STA);
            // Connect to the selected wifi AP.
            WiFi.begin(wifi_list[i].SSID, wifi_list[i].pass);
            // Try to connect within 30 seconds.
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
            // wpa enterprise require a login type auth. So get the password and login.
            strcpy(wifi_list[i].pass, lv_textarea_get_text(frm_wifi_login_ta_pass));
            strcpy(wifi_list[i].login, lv_textarea_get_text(frm_wifi_login_ta_login));
            // Continue only if the fields are not empty.
            if(strcmp(wifi_list[i].pass, "") == 0 || strcmp(wifi_list[i].login, "") == 0){
                Serial.println("Provide a login and passwod");
                return;
            }
            lv_label_set_text(frm_wifi_login_title_lbl, "Connecting...");
            // Disconnect from previous connection.
            WiFi.disconnect(true);
            // Set the mode to wifi client.
            WiFi.mode(WIFI_STA);
            // Set the wpa enterprise parameters.
            esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)wifi_list[i].login, strlen(wifi_list[i].login));
            esp_wifi_sta_wpa2_ent_set_username((uint8_t*)wifi_list[i].login, strlen(wifi_list[i].login));
            esp_wifi_sta_wpa2_ent_set_password((uint8_t*)wifi_list[i].pass, strlen(wifi_list[i].pass));
            // Enable the configuration.
            esp_wifi_sta_wpa2_ent_enable();
            // Connect.
            WiFi.begin(wifi_list[i].SSID, wifi_list[i].pass);
            // 30 seconds timeout.
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
            // Connect without auth.
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
            // Store the last wifi network connected index of the wifi_list.
            last_wifi_con = i;
            // Add to the list of once connected wifi APs.
            wifi_connected_nets.add(wifi_list[i]);
            // Save the list.
            wifi_connected_nets.save();
            Serial.println("wifi network saved");
            // In settings, on wifi tab there is a button with a wifi symbol, change the color to green.
            lv_obj_set_style_text_color(frm_settings_btn_wifi_lbl, lv_color_hex(0x00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
            Serial.println(WiFi.localIP().toString());
            // Update the status in wifi tab.
            strcpy(connected_to, wifi_list[i].SSID);
            strcat(connected_to, " ");
            strcat(connected_to, WiFi.localIP().toString().c_str());
            // Hide the auth dialogs for each type of connection.
            lv_obj_add_flag(frm_wifi_login, LV_OBJ_FLAG_HIDDEN); 
            lv_obj_add_flag(frm_wifi_simple, LV_OBJ_FLAG_HIDDEN);
            // Update the date and time through the internet.
            datetime();
        }else{
            Serial.println("Connection failed");
            lv_label_set_text(frm_wifi_connected_to_lbl, "Auth failed");
        }
    }
}
/// @brief Event used to disconnect from wifi and delete the connection from history.
/// @param e 
void forget_net(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    // Index of the conection on wifi_connected_nets.
    uint i = (int)lv_event_get_user_data(e);

    if(code == LV_EVENT_LONG_PRESSED){
        if(wifi_connected_nets.del(wifi_list[i].SSID)){ // If erased from the list.
            // Update the status.
            lv_label_set_text(frm_wifi_connected_to_lbl, "forgoten");
            Serial.println("SSID deleted from connected nets");
            // Disconnect from it if so.
            WiFi.disconnect(true);
            // Change the wifi icon color on settings to black.
            lv_obj_set_style_text_color(frm_settings_btn_wifi_lbl, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            // Save the list.
            if(wifi_connected_nets.save()){
                Serial.println("connected nets list save failed");
            }
        }else{
            Serial.println("SSID not found in connected nets");
        }
    }
}
/// @brief Selects the apropriate auth type dialog during a wifi configuration.
/// @param e 
void wifi_select(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    uint i = (int)lv_event_get_user_data(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        Serial.print("Connecting to ");
        Serial.println(wifi_list[i].SSID);
        Serial.println(wifi_list[i].RSSI);
        Serial.println(wifi_list[i].auth_type);
        // Change the connect button event accordingly to the auth type, this also changes the auth dialog type.
        switch(wifi_list[i].auth_type){
            case WIFI_AUTH_WPA2_PSK:
                // This is for password only dialog.
                // Remove the previous event.
                lv_obj_remove_event_cb(frm_wifi_simple_btn_connect, wifi_apply);
                // Sets a simple password dialog and auth event calling wifi_apply.
                lv_obj_add_event_cb(frm_wifi_simple_btn_connect, wifi_apply, LV_EVENT_SHORT_CLICKED, (void *)i);
                // Show the auth dialog.
                lv_obj_clear_flag(frm_wifi_simple, LV_OBJ_FLAG_HIDDEN);
                break;
            case WIFI_AUTH_WPA_WPA2_PSK:
                // Same as above.
                lv_obj_remove_event_cb(frm_wifi_simple_btn_connect, wifi_apply);
                lv_obj_add_event_cb(frm_wifi_simple_btn_connect, wifi_apply, LV_EVENT_SHORT_CLICKED, (void *)i);
                lv_obj_clear_flag(frm_wifi_simple, LV_OBJ_FLAG_HIDDEN);
                break;
            case WIFI_AUTH_WPA2_ENTERPRISE:
                // Same as above, however the auth dialog shown have login and password fields.
                lv_obj_remove_event_cb(frm_wifi_login_btn_connect, wifi_apply);
                lv_obj_add_event_cb(frm_wifi_login_btn_connect, wifi_apply, LV_EVENT_SHORT_CLICKED, (void *)i);
                lv_obj_clear_flag(frm_wifi_login, LV_OBJ_FLAG_HIDDEN);
                break;
            case WIFI_AUTH_OPEN:
                // This has no auth with password, connects directly.
                wifi_apply(e);
                break;
        }
    }if(code == LV_EVENT_LONG_PRESSED){
        // Long pressing on the name of the network makes a deletion from the history list.
        forget_net(e);
    }
}
/// @brief Task which scans for networks and populates a list used as reference to connect to.
/// @param param 
void wifi_scan_task(void * param){
    int n = 0;
    wifi_info wi;
    char ssid[100] = {'\0'};
    char rssi[5] = {'\0'};
    char ch[3] = {'\0'};
    lv_obj_t * btn = NULL;
    Serial.print("scanning...");
    
    lv_label_set_text(frm_wifi_connected_to_lbl, "Scanning...");
    
    // We can scan for other networks even if connected to one.
    // Get the number of networks discovered.
    n = WiFi.scanNetworks();
    if(n > 0){
        // Cleans the list of wifi networks object.
        lv_obj_clean(frm_wifi_list);
        // Empty the list that holds the wifi info struct.
        wifi_list.clear();
        // Iterates through the networks discovered.
        for(uint i = 0; i < n; i++){
            // Use a structure to hold the wifi info.
            // Get the type of auth.
            wi.auth_type = WiFi.encryptionType(i);
            // Get the signal strength.
            wi.RSSI = WiFi.RSSI(i);
            // Get the channel.
            wi.ch = WiFi.channel();
            // Get the SSID.
            strcpy(wi.SSID, WiFi.SSID(i).c_str());
            // Store on wifi_list. wifi_list is used to build a lvgl list with the names of the networks.
            wifi_list.push_back(wi);
            // Create a description of each wifi network.
            strcpy(ssid, WiFi.SSID(i).c_str());
            strcat(ssid, " rssi:");
            itoa(WiFi.RSSI(i), rssi, 10);
            strcat(ssid, rssi);
            strcat(ssid, "\n");
            strcat(ssid, wifi_auth_mode_to_str(WiFi.encryptionType(i)));
            strcat(ssid, " ch:");
            itoa(WiFi.channel(i), ch, 10);
            strcat(ssid, ch);
            // Create a button on the list with the description.
            btn = lv_list_add_btn(frm_wifi_list, LV_SYMBOL_WIFI, ssid);
            // Set two events for each button, one to connect, other to forget.
            lv_obj_add_event_cb(btn, wifi_select, LV_EVENT_SHORT_CLICKED, (void *)i);
            lv_obj_add_event_cb(btn, wifi_select, LV_EVENT_LONG_PRESSED, (void *)i);
            
        }
    }
    Serial.println("done");
    // Update the status message on scan.
    lv_label_set_text(frm_wifi_connected_to_lbl, "Scan complete");
    // Delete the task.
    vTaskDelete(task_wifi_scan);
}
/// @brief This event create a task which list the wifi networks.
/// @param e 
void wifi_scan(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        xTaskCreatePinnedToCore(wifi_scan_task, "wifi_scan_task", 10000, NULL, 1, &task_wifi_scan, 1);
    }
}
/// @brief Verifies if is conected to a wifi and sets or remove a icon on the home screen.
void update_wifi_icon(){
    if(WiFi.isConnected()){
        // Seti the icon.
        lv_label_set_text(frm_home_wifi_lbl, LV_SYMBOL_WIFI);
        // Set the name of the network in which its connected to.
        lv_label_set_text(frm_wifi_connected_to_lbl, connected_to);
        
    }
    else{
        // Undo the above.
        lv_label_set_text(frm_home_wifi_lbl, "");
        lv_label_set_text(frm_wifi_connected_to_lbl, "");
        
    }
    // Sets the wifi connection info on wifi tab in settings menu.
    wifi_con_info();
}
/// @brief Event to show the content of the password field as the user holds the eye icon.
/// @param e 
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
/// @brief Event to show the content of the password field as the user holds the eye icon.
/// @param e 
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
/// @brief Show a simple auth dialog with password.
/// @param e 
void show_simple(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_wifi_simple != NULL){
            lv_obj_clear_flag(frm_wifi_simple, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
/// @brief Hide the simple auth dialog.
/// @param e 
void hide_simple(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_wifi_simple != NULL){
            lv_obj_add_flag(frm_wifi_simple, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
/// @brief Show a auth dialog with login and password.
/// @param e 
void show_wifi_login(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_wifi_login != NULL){
            lv_obj_clear_flag(frm_wifi_login, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
/// @brief Hide the auth dialog with login and password.
/// @param e 
void hide_wifi_login(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(frm_wifi_login != NULL){
            lv_obj_add_flag(frm_wifi_login, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
/// @brief Event to toggle the connection on and off.
/// @param e 
void wifi_toggle(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    uint c = 0;

    if(code == LV_EVENT_SHORT_CLICKED){
        // This will only proceed if we have at least a connection saved on history.
        if(wifi_connected_nets.list.size() == 0)
            return;
        if(!WiFi.isConnected()){
            // Search for a wifi connection, if it exists on the history connect to it.
            wifi_auto_connect(NULL);
        }
        else{
            // Disconnect from the network.
            WiFi.disconnect(true);
            // Turn off the wifi transceiver.
            WiFi.mode(WIFI_OFF);
            // Sets the color of the wifi symbol to white.
            lv_obj_set_style_text_color(frm_settings_btn_wifi_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            Serial.println("wifi off");
        }
    }
}
/// @brief T-Deck's routine to setup the sd card.
/// @return 
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
/// @brief T-Deck's routine to enable sound and microfone.
/// @return 
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
/// @brief Task to connect to a internet radio station and plays.
/// @param param 
void song(void * param) {
    //BaseType_t xHigherPriorityTaskWoken;
    if(WiFi.isConnected()){
        //if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
            digitalWrite(BOARD_POWERON, HIGH);
            audio.setPinout(BOARD_I2S_BCK, BOARD_I2S_WS, BOARD_I2S_DOUT);
            audio.setVolume(10);
            audio.connecttohost("0n-80s.radionetz.de:8000/0n-70s.mp3");
            //audio.connecttospeech("Notification sent.", "us");
            while (audio.isRunning()) {
                audio.loop();
            }
            audio.stopSong();
        //    xSemaphoreGive(xSemaphore);
        //}
    }
    vTaskDelete(task_play_radio);
}
/// @brief Task to play a short mp3 sound from the sd card, used to notify.
/// @param p 
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
/// @brief Function to launch a task to play a notification sound.
void notify_snd(){
    //xTaskCreatePinnedToCore(taskplaySong, "play_not_snd", 10000, NULL, 2 | portPRIVILEGE_BIT, NULL, 0);
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
/// @brief Event that shows a small keyboard with latin chars on chat dialog.
/// @param e
void show_especial(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(lv_obj_has_flag(kb, LV_OBJ_FLAG_HIDDEN))
            lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}
/// @brief Change the color of the activity indicator.
/// @param color 
void activity(lv_color_t color){
    lv_obj_set_style_bg_color(frm_home_activity_led, color, LV_PART_MAIN | LV_STATE_DEFAULT);
}
/// @brief Array of latin chars.
const char * kbmap[] = {"á", "à", "ã", "â", "Á", "À", "Ã", "Â","\n",
                        "é", "ê", "í", "ó", "É", "Ê", "Í", "Ó","\n",
                        "ô", "õ", "ú", "ç", "Ô", "Õ", "Ú", "Ç",""};
/// @brief Array of state type for each button.
const lv_btnmatrix_ctrl_t kbctrl[] = {
LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, 
LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT,
LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT,
LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, 
LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT,
LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT, LV_BTNMATRIX_CTRL_NO_REPEAT};
/// @brief Array brightness levels to set on the roller object.
const char * brightness_levels = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10";
/// @brief Event to set the backlight intensity of the display.
/// @param e 
void setBrightness(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * roller = (lv_obj_t *)lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED){
        // Get the selected index on the roller.
        brightness = lv_roller_get_selected(roller);
        // Write a mapped value to the analog output pin where the backlight is connected to.
        analogWrite(BOARD_BL_PIN, mapv(brightness, 0, 9, 100, 255));
    }
}
/// @brief Enum used to configure a menu object.
enum {
    LV_MENU_ITEM_BUILDER_VARIANT_1,
    LV_MENU_ITEM_BUILDER_VARIANT_2
};
typedef uint8_t lv_menu_builder_variant_t;
/// @brief Function to create a menu item.
/// @param parent 
/// @param icon 
/// @param txt 
/// @param builder_variant 
/// @return lv_obj_t*
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
/// @brief Function to update the wifi info on wifi tab in settings.
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

void rssi_chart_zoom(lv_event_t * e){
    lv_obj_t * rssi_chart = (lv_obj_t *)lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED){
        if(!chart_zoomed){
            lv_obj_set_size(frm_home_rssi_chart, 295, 200);
            lv_obj_move_foreground(frm_home_rssi_chart);
            lv_chart_set_div_line_count(frm_home_rssi_chart, 5, 8);
            lv_obj_set_style_bg_opa(frm_home_rssi_chart, 150, LV_PART_MAIN);
            //lv_chart_set_series_color(frm_home_rssi_chart, frm_home_rssi_series, lv_palette_main(LV_PALETTE_LIGHT_GREEN));
            chart_zoomed = true;
        }
        else{
            lv_obj_set_size(frm_home_rssi_chart, rssi_chart_width, rssi_chart_height);
            lv_chart_set_div_line_count(frm_home_rssi_chart, 3, 5);
            lv_obj_move_background(frm_home_rssi_chart);
            lv_obj_set_style_bg_opa(frm_home_rssi_chart, 80, LV_PART_MAIN);
            //lv_chart_set_series_color(frm_home_rssi_chart, frm_home_rssi_series, lv_color_hex(0x00ff00));
            chart_zoomed = false;
        }
    }
}

void show_discovery_app(lv_event_t * e){
    discoveryApp.showUI();
}

/// @brief Function to initilize all lvgl objects.
void ui(){
    //style**************************************************************
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_color_hex(ui_primary_color), lv_palette_main(LV_PALETTE_RED), false, &lv_font_montserrat_16);
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

    //Notification icon
    frm_home_symbol_lbl = lv_label_create(frm_home);
    lv_obj_align(frm_home_symbol_lbl, LV_ALIGN_TOP_LEFT, -10, -10);
    lv_obj_set_style_text_color(frm_home_symbol_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);

    frm_home_title_lbl = lv_label_create(frm_home);
    lv_obj_set_style_text_font(frm_home_title_lbl, &ubuntu, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(frm_home_title_lbl, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(frm_home_title_lbl, LV_ALIGN_TOP_LEFT, 7, -10);
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

    // RSSI graph
    frm_home_rssi_chart = lv_chart_create(frm_home);
    lv_obj_set_size(frm_home_rssi_chart, 90, 55);
    lv_obj_align(frm_home_rssi_chart, LV_ALIGN_OUT_TOP_LEFT, 0, 15);
    lv_chart_set_type(frm_home_rssi_chart, LV_CHART_TYPE_LINE);
    frm_home_rssi_series = lv_chart_add_series(frm_home_rssi_chart, lv_color_hex(0x00ff00), LV_CHART_AXIS_PRIMARY_Y);
    frm_home_snr_series = lv_chart_add_series(frm_home_rssi_chart, lv_color_hex(0xff0000), LV_CHART_AXIS_SECONDARY_Y);
    lv_chart_set_range(frm_home_rssi_chart, LV_CHART_AXIS_PRIMARY_Y, -147, 0);
    lv_chart_set_range(frm_home_rssi_chart, LV_CHART_AXIS_SECONDARY_Y, 0, 20);
    lv_chart_set_next_value(frm_home_rssi_chart, frm_home_rssi_series, -147);
    lv_chart_set_next_value(frm_home_rssi_chart, frm_home_snr_series, 0);
    //lv_obj_set_style_bg_color(frm_home_rssi_chart, lv_color_hex(0x000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(frm_home_rssi_chart, 80, LV_PART_MAIN);
    //lv_obj_set_style_border_color(frm_home_rssi_chart, lv_color_hex(0x000), LV_PART_MAIN);
    lv_obj_set_style_border_opa(frm_home_rssi_chart, 80, LV_PART_MAIN);
    lv_chart_refresh(frm_home_rssi_chart);
    lv_obj_add_event_cb(frm_home_rssi_chart, rssi_chart_zoom, LV_EVENT_CLICKED, NULL);

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
    lv_obj_add_event_cb(btn_test, ping, LV_EVENT_SHORT_CLICKED, NULL);

    // Nodes button
    discoveryApp.initUI(lv_scr_act());
    //delay(2000);
    btn_nodes = lv_btn_create(frm_home);
    lv_obj_set_size(btn_nodes, 50, 20);
    lv_obj_align(btn_nodes, LV_ALIGN_BOTTOM_RIGHT, 0, -50);

    btn_nodes_lbl = lv_label_create(btn_nodes);
    lv_obj_set_style_text_font(btn_nodes_lbl, &ubuntu, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(btn_nodes_lbl, "Nodes");
    lv_obj_align(btn_nodes_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_nodes, show_discovery_app, LV_EVENT_SHORT_CLICKED, NULL);

    // Activity led 
    frm_home_activity_led = lv_obj_create(frm_home);
    lv_obj_set_size(frm_home_activity_led, 7, 15);
    lv_obj_align(frm_home_activity_led, LV_ALIGN_TOP_RIGHT, -75, -10);
    lv_obj_clear_flag(frm_home_activity_led, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(frm_home_activity_led, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(frm_home_activity_led, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    
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

    // Key text input
    frm_add_contact_textarea_key = lv_textarea_create(frm_add_contact);
    lv_obj_set_size(frm_add_contact_textarea_key, 240, 30);
    lv_obj_align(frm_add_contact_textarea_key, LV_ALIGN_TOP_LEFT, 0, 95);
    lv_textarea_set_placeholder_text(frm_add_contact_textarea_key, "KEY");

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

    // KEY text input
    frm_edit_text_key = lv_textarea_create(frm_edit_contacts);
    lv_obj_set_size(frm_edit_text_key, 240, 30);
    lv_obj_align(frm_edit_text_key, LV_ALIGN_TOP_LEFT, 0, 95);
    lv_textarea_set_placeholder_text(frm_edit_text_key, "KEY");

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
    lv_textarea_set_max_length(frm_chat_text_ans, 207);
    lv_textarea_set_placeholder_text(frm_chat_text_ans, "Answer");
    lv_obj_set_style_text_font(frm_chat_text_ans, &ubuntu, LV_PART_MAIN | LV_STATE_DEFAULT);

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

    //keyboard with special chars
    kb = lv_keyboard_create(frm_chat);
    lv_obj_set_style_text_font(kb, &ubuntu, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_SPECIAL);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_SPECIAL, kbmap, kbctrl);
    lv_keyboard_set_textarea(kb, frm_chat_text_ans);
    lv_obj_set_size(kb, 200, 100);
    lv_obj_align(kb, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0xcccccc), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

    //special chars keyboard button
    frm_chat_btn_kb = lv_btn_create(frm_chat);
    lv_obj_set_size(frm_chat_btn_kb, 30, 20);
    lv_obj_align(frm_chat_btn_kb, LV_ALIGN_TOP_RIGHT, -50, -15);
    lv_obj_add_event_cb(frm_chat_btn_kb, show_especial, LV_EVENT_SHORT_CLICKED, NULL);

    frm_chat_btn_kb_lbl = lv_label_create(frm_chat_btn_kb);
    //lv_obj_set_style_text_font(frm_chat_btn_kb_lbl, &ubuntu, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(frm_chat_btn_kb_lbl, LV_SYMBOL_KEYBOARD);
    lv_obj_set_align(frm_chat_btn_kb_lbl, LV_ALIGN_CENTER);

    lv_obj_add_flag(frm_chat, LV_OBJ_FLAG_HIDDEN);

    // Settings form**************************************************************
    
    frm_settings = lv_obj_create(lv_scr_act());
    lv_obj_set_size(frm_settings, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(frm_settings, LV_OBJ_FLAG_SCROLLABLE);

    //Menu
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

    // id section
    frm_settings_id_page = lv_menu_page_create(frm_settings_menu, NULL);
    lv_obj_set_style_pad_ver(frm_settings_id_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(frm_settings_menu), 0), 0);
    lv_menu_separator_create(frm_settings_id_page);
    frm_settings_id_section = lv_menu_section_create(frm_settings_id_page);
    lv_obj_clear_flag(frm_settings_id_page, LV_OBJ_FLAG_SCROLLABLE);

    frm_settings_obj_id = lv_obj_create(frm_settings_id_section);
    lv_obj_set_size(frm_settings_obj_id, 230, 230);

    //name
    frm_settings_name = lv_textarea_create(frm_settings_obj_id);
    lv_textarea_set_one_line(frm_settings_name, true);
    lv_obj_set_size(frm_settings_name, 180, 30);
    lv_textarea_set_placeholder_text(frm_settings_name, "Name");
    lv_obj_align(frm_settings_name, LV_ALIGN_OUT_TOP_LEFT, 0, -10);

    // ID
    frm_settings_id = lv_textarea_create(frm_settings_obj_id);
    lv_textarea_set_one_line(frm_settings_id, true);
    lv_obj_set_size(frm_settings_id, 90, 30);
    lv_textarea_set_max_length(frm_settings_id, 6);
    lv_textarea_set_placeholder_text(frm_settings_id, "ID");
    lv_obj_align(frm_settings_id, LV_ALIGN_TOP_LEFT, 0, 20);

    //Generate button
    frm_settings_btn_generate = lv_btn_create(frm_settings_obj_id);
    lv_obj_set_size(frm_settings_btn_generate, 80, 20);
    lv_obj_align(frm_settings_btn_generate, LV_ALIGN_TOP_LEFT, 100, 25);
    lv_obj_add_event_cb(frm_settings_btn_generate, generateID, LV_EVENT_SHORT_CLICKED, (void*)6);

    frm_settings_btn_generate_lbl = lv_label_create(frm_settings_btn_generate);
    lv_label_set_text(frm_settings_btn_generate_lbl, "Generate");
    lv_obj_set_align(frm_settings_btn_generate_lbl, LV_ALIGN_CENTER);

    // Key text area
    frm_settings_key = lv_textarea_create(frm_settings_obj_id);
    lv_textarea_set_one_line(frm_settings_key, true);
    lv_obj_set_size(frm_settings_key, 180, 30);
    lv_textarea_set_max_length(frm_settings_key, 16);
    lv_textarea_set_placeholder_text(frm_settings_key, "KEY");
    lv_obj_align(frm_settings_key, LV_ALIGN_TOP_LEFT, 0, 50);

    // Generate key button
    frm_settings_btn_generate_key = lv_btn_create(frm_settings_obj_id);
    lv_obj_set_size(frm_settings_btn_generate_key, 80, 20);
    lv_obj_align(frm_settings_btn_generate_key, LV_ALIGN_TOP_LEFT, 0, 85);
    lv_obj_add_event_cb(frm_settings_btn_generate_key, generateKEY, LV_EVENT_SHORT_CLICKED, (void*)16);

    frm_settings_btn_generate_key_lbl = lv_label_create(frm_settings_btn_generate_key);
    lv_label_set_text(frm_settings_btn_generate_key_lbl, "Generate");
    lv_obj_set_align(frm_settings_btn_generate_key_lbl, LV_ALIGN_CENTER);

    // Admin password
    frm_settings_admin_password = lv_textarea_create(frm_settings_obj_id);
    lv_textarea_set_placeholder_text(frm_settings_admin_password, "adm pass");
    lv_textarea_set_text(frm_settings_admin_password, http_admin_pass);
    lv_textarea_set_max_length(frm_settings_admin_password, 40);
    lv_obj_set_size(frm_settings_admin_password, 180, 30);
    lv_obj_align(frm_settings_admin_password, LV_ALIGN_TOP_LEFT, 0, 115);

    //dx section
    frm_settings_dx_section;
    frm_settings_dx_page = lv_menu_page_create(frm_settings_menu, NULL);
    lv_obj_set_style_pad_hor(frm_settings_dx_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(frm_settings_menu), 0), 0);
    lv_menu_separator_create(frm_settings_dx_page);
    frm_settings_dx_section = lv_menu_section_create(frm_settings_dx_page);

    frm_settings_obj_dx = lv_obj_create(frm_settings_dx_section);
    lv_obj_set_size(frm_settings_obj_dx, 230, 230);

    // dx switch
    frm_settings_switch_dx = lv_switch_create(frm_settings_obj_dx);
    lv_obj_align(frm_settings_switch_dx, LV_ALIGN_OUT_TOP_LEFT, 30, -10);
    lv_obj_add_event_cb(frm_settings_switch_dx, DX, LV_EVENT_VALUE_CHANGED, NULL);

    frm_settings_switch_dx_lbl = lv_label_create(frm_settings_obj_dx);
    lv_label_set_text(frm_settings_switch_dx_lbl, "DX");
    lv_obj_align(frm_settings_switch_dx_lbl, LV_ALIGN_TOP_LEFT, 0, -5);

    //wifi section
    lv_obj_t * frm_settings_wifi_section;
    lv_obj_t * frm_settings_wifi_page = lv_menu_page_create(frm_settings_menu, NULL);
    lv_obj_set_style_pad_hor(frm_settings_wifi_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(frm_settings_menu), 0), 0);
    lv_menu_separator_create(frm_settings_wifi_page);
    frm_settings_wifi_section = lv_menu_section_create(frm_settings_wifi_page);

    lv_obj_t * frm_settings_obj_wifi = lv_obj_create(frm_settings_wifi_section);
    lv_obj_set_size(frm_settings_obj_wifi, 230, 230);

    //wifi toggle button
    frm_settings_btn_wifi = lv_btn_create(frm_settings_obj_wifi);
    lv_obj_set_size(frm_settings_btn_wifi, 30, 30);
    lv_obj_align(frm_settings_btn_wifi, LV_ALIGN_TOP_LEFT, 0, -10);
    lv_obj_add_event_cb(frm_settings_btn_wifi, wifi_toggle, LV_EVENT_SHORT_CLICKED, NULL);

    frm_settings_btn_wifi_lbl = lv_label_create(frm_settings_btn_wifi);
    lv_label_set_text(frm_settings_btn_wifi_lbl, LV_SYMBOL_WIFI);
    lv_obj_set_align(frm_settings_btn_wifi_lbl, LV_ALIGN_CENTER);

    // wifi config button
    frm_settings_btn_wifi_conf = lv_btn_create(frm_settings_obj_wifi);
    lv_obj_set_size(frm_settings_btn_wifi_conf, 80, 30);
    lv_obj_align(frm_settings_btn_wifi_conf, LV_ALIGN_TOP_LEFT, 40, -10);
    lv_obj_add_event_cb(frm_settings_btn_wifi_conf, show_wifi, LV_EVENT_SHORT_CLICKED, NULL);

    frm_settings_btn_wifi_conf_lbl = lv_label_create(frm_settings_btn_wifi_conf);
    lv_label_set_text(frm_settings_btn_wifi_conf_lbl, "Configure");
    lv_obj_set_align(frm_settings_btn_wifi_conf_lbl, LV_ALIGN_CENTER);

    //Http server status button
    frm_settings_wifi_http_btn = lv_btn_create(frm_settings_obj_wifi);
    lv_obj_set_size(frm_settings_wifi_http_btn, 50, 30);
    lv_obj_align(frm_settings_wifi_http_btn, LV_ALIGN_TOP_LEFT, 130, -10);
    lv_obj_add_flag(frm_settings_wifi_http_btn, LV_OBJ_FLAG_HIDDEN);

    frm_settings_wifi_http_label = lv_label_create(frm_settings_wifi_http_btn);
    lv_label_set_text(frm_settings_wifi_http_label, "https");
    lv_obj_set_align(frm_settings_wifi_http_label, LV_ALIGN_CENTER);

    //wifi info
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

    //date section
    lv_obj_t * frm_settings_date_section;
    lv_obj_t * frm_settings_date_page = lv_menu_page_create(frm_settings_menu, NULL);
    lv_obj_set_style_pad_hor(frm_settings_date_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(frm_settings_menu), 0), 0);
    lv_menu_separator_create(frm_settings_date_page);
    frm_settings_date_section = lv_menu_section_create(frm_settings_date_page);

    lv_obj_t * frm_settings_obj_date = lv_obj_create(frm_settings_date_section);
    lv_obj_set_size(frm_settings_obj_date, 230, 230);
    lv_obj_clear_flag(frm_settings_obj_date, LV_OBJ_FLAG_SCROLLABLE);

    //date label
    frm_settings_date_lbl = lv_label_create(frm_settings_obj_date);
    lv_label_set_text(frm_settings_date_lbl, "Date");
    lv_obj_align(frm_settings_date_lbl, LV_ALIGN_TOP_LEFT, 0, -5);

    //day
    frm_settings_day = lv_textarea_create(frm_settings_obj_date);
    lv_obj_set_size(frm_settings_day, 60, 30);
    lv_obj_align(frm_settings_day, LV_ALIGN_TOP_LEFT, 0, 20);
    lv_textarea_set_accepted_chars(frm_settings_day, "1234567890");
    lv_textarea_set_max_length(frm_settings_day, 2);
    lv_textarea_set_placeholder_text(frm_settings_day, "dd");

    //month
    frm_settings_month = lv_textarea_create(frm_settings_obj_date);
    lv_obj_set_size(frm_settings_month, 60, 30);
    lv_obj_align(frm_settings_month, LV_ALIGN_TOP_LEFT, 60, 20);
    lv_textarea_set_accepted_chars(frm_settings_month, "1234567890");
    lv_textarea_set_max_length(frm_settings_month, 2);
    lv_textarea_set_placeholder_text(frm_settings_month, "mm");

    //year
    frm_settings_year = lv_textarea_create(frm_settings_obj_date);
    lv_obj_set_size(frm_settings_year, 60, 30);
    lv_obj_align(frm_settings_year, LV_ALIGN_TOP_LEFT, 120, 20);
    lv_textarea_set_accepted_chars(frm_settings_year, "1234567890");
    lv_textarea_set_max_length(frm_settings_year, 4);
    lv_textarea_set_placeholder_text(frm_settings_year, "yyyy");

    //time label
    frm_settings_time_lbl = lv_label_create(frm_settings_obj_date);
    lv_label_set_text(frm_settings_time_lbl, "Time");
    lv_obj_align(frm_settings_time_lbl, LV_ALIGN_TOP_LEFT, 0, 60);

    //hour
    frm_settings_hour = lv_textarea_create(frm_settings_obj_date);
    lv_obj_set_size(frm_settings_hour, 60, 30);
    lv_obj_align(frm_settings_hour, LV_ALIGN_TOP_LEFT, 0, 80);
    lv_textarea_set_accepted_chars(frm_settings_hour, "1234567890");
    lv_textarea_set_max_length(frm_settings_hour, 2);
    lv_textarea_set_placeholder_text(frm_settings_hour, "hh");

    //minute
    frm_settings_minute = lv_textarea_create(frm_settings_obj_date);
    lv_obj_set_size(frm_settings_minute, 60, 30);
    lv_obj_align(frm_settings_minute, LV_ALIGN_TOP_LEFT, 60, 80);
    lv_textarea_set_accepted_chars(frm_settings_minute, "1234567890");
    lv_textarea_set_max_length(frm_settings_minute, 2);
    lv_textarea_set_placeholder_text(frm_settings_minute, "mm");

    // setDate button
    frm_settings_btn_setDate = lv_btn_create(frm_settings_obj_date);
    lv_obj_set_size(frm_settings_btn_setDate, 50, 20);
    lv_obj_align(frm_settings_btn_setDate, LV_ALIGN_TOP_LEFT, 130, 85);
    lv_obj_add_event_cb(frm_settings_btn_setDate, applyDate, LV_EVENT_SHORT_CLICKED, NULL);

    //setDate label
    frm_settings_btn_setDate_lbl = lv_label_create(frm_settings_btn_setDate);
    lv_label_set_text(frm_settings_btn_setDate_lbl, "Set");
    lv_obj_set_align(frm_settings_btn_setDate_lbl, LV_ALIGN_CENTER);

    // Time zone list
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

    //ui custom section
    lv_obj_t * frm_settings_ui_section;
    lv_obj_t * frm_settings_ui_page = lv_menu_page_create(frm_settings_menu, NULL);
    lv_obj_set_style_pad_hor(frm_settings_ui_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(frm_settings_menu), 0), 0);
    lv_menu_separator_create(frm_settings_ui_page);
    frm_settings_ui_section = lv_menu_section_create(frm_settings_ui_page);

    frm_settings_obj_ui = lv_obj_create(frm_settings_ui_section);
    lv_obj_set_size(frm_settings_obj_ui, 230, 230);
    lv_obj_clear_flag(frm_settings_obj_ui, LV_OBJ_FLAG_SCROLLABLE);

    // color label
    frm_settings_btn_color_lbl = lv_label_create(frm_settings_obj_ui);
    lv_label_set_text(frm_settings_btn_color_lbl, "UI color");
    lv_obj_align(frm_settings_btn_color_lbl, LV_ALIGN_TOP_LEFT, 0, 5);
    
    //color 
    frm_settings_color = lv_textarea_create(frm_settings_obj_ui);
    lv_obj_set_size(frm_settings_color , 100, 30);
    lv_obj_align(frm_settings_color, LV_ALIGN_TOP_LEFT, 60, 0);
    lv_textarea_set_max_length(frm_settings_color, 6);
    lv_textarea_set_accepted_chars(frm_settings_color, "abcdefABCDEF1234567890");

    //apply color button
    frm_settings_btn_applycolor = lv_btn_create(frm_settings_obj_ui);
    lv_obj_set_size(frm_settings_btn_applycolor, 50, 20);
    lv_obj_align(frm_settings_btn_applycolor, LV_ALIGN_TOP_LEFT, 0, 30);
    lv_obj_add_event_cb(frm_settings_btn_applycolor, apply_color, LV_EVENT_SHORT_CLICKED, NULL);

    // apply color label
    frm_settings_btn_applycolor_lbl = lv_label_create(frm_settings_btn_applycolor);
    lv_label_set_text(frm_settings_btn_applycolor_lbl, "Apply");
    lv_obj_set_align(frm_settings_btn_applycolor_lbl, LV_ALIGN_CENTER);

    //brightness section
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

    //root page
    frm_settings_root_section;
    frm_settings_root_page = lv_menu_page_create(frm_settings_menu, (char*)"Settings");
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
    //traditional style
    //lv_menu_clear_history(frm_settings_menu);
    //lv_menu_set_page(frm_settings_menu, frm_settings_root_page);

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
    lv_obj_set_style_text_font(frm_not_lbl, &ubuntu, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(frm_not_lbl, LV_ALIGN_TOP_LEFT, 5, -10);
    lv_label_set_long_mode(frm_not_lbl, LV_LABEL_LONG_SCROLL);

    //symbol label
    frm_not_symbol_lbl = lv_label_create(frm_not);
    lv_obj_align(frm_not_symbol_lbl, LV_ALIGN_TOP_LEFT, -10, -10);

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
/// @brief Function to update the RTC with the intenet date and time and update the Date time widget.
void datetime(){
    // Hour and minute.
    char hm[6] = {'\0'};
    // More described date.
    char date[12] = {'\0'};
    // Only proceed with internet connection.
    if(WiFi.status() == WL_CONNECTED){
        Serial.print("RSSI ");
        Serial.println(WiFi.RSSI());
        Serial.println(WiFi.localIP());
        // Configure sntp connection.
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
/// @brief Function to return a string representation of the auth mode code used by wifi.
/// @param auth_mode 
/// @return const char *
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

/// @brief This task searches for a wifi AP on the hitory list and connect to it if available.
/// @param param 
void wifi_auto_connect(void * param){
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(WiFiDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    // Number of wifi network discovered.
    int n = 0;
    // Count how many seconds elapsed until timeout.
    int c = 0;
    // Holds the wifi characteristics.
    wifi_info wi;
    // List of characteristics of each wifi network found.
    vector<wifi_info>list;
    // String used to update statuses.
    char a[50] = {'\0'};
    // Only proceed if we have at least a network on history.
    if(wifi_connected_nets.list.size() != 0){
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        Serial.print("Searching for wifi connections...");
        // Shows on notify area on home screen.
        lv_label_set_text(frm_home_title_lbl, "Searching for wifi connections...");
        lv_label_set_text(frm_home_symbol_lbl, LV_SYMBOL_WIFI);
        // Disconnect from network if it is.
        WiFi.disconnect(true);
        // Change mode to stand alone client.
        WiFi.mode(WIFI_STA);
        // Get the number of networks discovered.
        n = WiFi.scanNetworks();
        if(n > 0)
            for(int i = 0; i < n; i++){
                // Save the network on the list.
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
        // Search in history for a wifi network connected previously.
        if(wifi_connected_nets.list.size() > 0){
            for(uint32_t i = 0; i < wifi_connected_nets.list.size(); i++){
                for(uint32_t j = 0; j < list.size(); j++){
                    if(strcmp(wifi_connected_nets.list[i].SSID, list[j].SSID) == 0){ // We found one.
                        // Notify the user the attempt connection.
                        Serial.print("Connecting to ");
                        Serial.print(wifi_connected_nets.list[i].SSID);
                        strcpy(a, "Connecting to ");
                        strcat(a, wifi_connected_nets.list[i].SSID);
                        lv_label_set_text(frm_home_title_lbl, a);
                        // Choose the apropriate auth method.
                        if(wifi_connected_nets.list[i].auth_type == WIFI_AUTH_WPA2_ENTERPRISE){
                            // This is for wpa enterprise, we need login and password.
                            esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)wifi_connected_nets.list[i].login, strlen(wifi_connected_nets.list[i].login));
                            esp_wifi_sta_wpa2_ent_set_username((uint8_t*)wifi_connected_nets.list[i].login, strlen(wifi_connected_nets.list[i].login));
                            esp_wifi_sta_wpa2_ent_set_password((uint8_t*)wifi_connected_nets.list[i].pass, strlen(wifi_connected_nets.list[i].pass));
                            esp_wifi_sta_wpa2_ent_enable();
                            // Disconnect from previous network.
                            WiFi.disconnect(true);
                            // Configures the wifi to stand alone client.
                            WiFi.mode(WIFI_STA);
                            // Begin a new connection.
                            WiFi.begin(wifi_connected_nets.list[i].SSID, wifi_connected_nets.list[i].pass);
                            // Set a 30 seconds timeout.
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
                            // Store the index of the network on history.
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
                // Update the notification area on home screen.
                lv_label_set_text(frm_home_title_lbl, "connected");
                lv_label_set_text(frm_home_symbol_lbl, LV_SYMBOL_WIFI);
                // Change the color of the wifi icon on settings to green.
                lv_obj_set_style_text_color(frm_settings_btn_wifi_lbl, lv_color_hex(0x00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                // For debug purposes.
                strcpy(connected_to, wifi_connected_nets.list[last_wifi_con].SSID);
                strcat(connected_to, " ");
                strcat(connected_to, WiFi.localIP().toString().c_str());
                Serial.println(" connected");
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                // Clear the notification area on home screen.
                lv_label_set_text(frm_home_title_lbl, "");
                lv_label_set_text(frm_home_symbol_lbl, "");
                // Sets the date using the internet.
                datetime();
                if(param != NULL){
                    // Launch the web server task.
                    //xTaskCreatePinnedToCore(setupServer, "server", 12000, (void*)"ok", 1, NULL, 1);
                    //setupServer(NULL);
                }
            }else{
                // If fails, notify the user and turn off the wifi transceiver.
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
    // To use this as function pass NULL as param. 
    if(param != NULL){
        vTaskDelete(task_wifi_auto);
        task_wifi_auto = NULL;
    }
}
/// @brief Sends a beacon as LoRa packet.
void announce(){
    lora_packet hi;

    //Serial.println("==============announcement=============");
    strcpy(hi.id, generate_ID(6).c_str());
    strcpy(hi.sender, user_id);
    strcpy(hi.status, "show");
    hi.type = LORA_PKT_ANNOUNCE;
    //Serial.printf("creating announcement packet ID %s\n", hi.id);
    if(!transmit_pkt_list.hasType(LORA_PKT_ANNOUNCE)){
        transmit_pkt_list.add(hi);
        if(!pkt_history.exists(hi.id))
            pkt_history.add(hi.id);
    }
    else{
        //Serial.printf("Announcement packet already on transmit queue\n");
    }
    //Serial.println("=======================================");
}

void task_beacon(void * param){
    uint32_t r = 0;
    while(true){
        r = transmit_pkt_list.genPktTimeout(10);
        vTaskDelay(r / portTICK_PERIOD_MS);
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
    //SPIFFS.end();
}

/// @brief T-Deck's initial setup function.
void setup(){
    // Seed to rand()
    struct timeval timea;
    gettimeofday(&timea, NULL);
    srand((timea.tv_sec * 1000) + (timea.tv_sec / 1000));

    if(!SPIFFS.begin(true)){
        Serial.println("failed mounting SPIFFS");
    }
    bool ret = false;
    Serial.begin(115200);
    //delay(3000);

    //time interval checking online contacts.
    contacts_list.setCheckPeriod(5);
    //Load contacts
    loadContacts();
    //SPIFFS.remove("/contacts");

    //load connected wifi networks.
    if(wifi_connected_nets.load())
        Serial.println("wifi networks loaded");
    else
        Serial.println("no wifi networks loaded");
    // Configure pinouts.
    pinMode(BOARD_POWERON, OUTPUT);
    digitalWrite(BOARD_POWERON, HIGH);
    // SPI chip selection for sd card, radio module and display. The selection is when the output goes low.
    pinMode(BOARD_SDCARD_CS, OUTPUT);
    pinMode(RADIO_CS_PIN, OUTPUT);
    pinMode(BOARD_TFT_CS, OUTPUT);

    digitalWrite(BOARD_SDCARD_CS, HIGH);
    digitalWrite(RADIO_CS_PIN, HIGH);
    digitalWrite(BOARD_TFT_CS, HIGH);

    pinMode(BOARD_SPI_MISO, INPUT_PULLUP);
    // Initialize the SPI bus.
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
    // Inicialize the radio module.
    setupRadio(NULL);
    
    // Initialize the i2c bus.
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    scanDevices(&Wire);
    // Initialize the touch screen.
    touch = new TouchLib(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, touchAddress);
    touch->init();
    Wire.beginTransmission(touchAddress);
    ret = Wire.endTransmission() == 0;
    touchDected = ret;
    if(touchDected)
        Serial.println("touch detected");
    else 
        Serial.println("touch not detected");
    // initialize the display.
    tft.begin();
    // Set to landscape mode.
    tft.setRotation( 1 );
    tft.fillScreen(TFT_BLUE);
    // Semaphore to control the SPI access to the modules.
    xSemaphore = xSemaphoreCreateBinary();
    assert(xSemaphore);
    xSemaphoreGive( xSemaphore );
    // Mutexes to restrict the access to some resources, avoiding memory corruption, catastrophic failures.
    pthread_mutexattr_t Attr;
    pthread_mutexattr_init(&Attr);
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&lvgl_mutex, &Attr);
    pthread_mutex_init(&messages_mutex, NULL);
    pthread_mutex_init(&send_json_mutex, NULL);
    pthread_mutex_init(&websocket_send, NULL);
    // Initialize the physical keyboard.
    Wire.beginTransmission(touchAddress);
    ret = Wire.endTransmission() == 0;
    touchDected = ret;
    kbDected = checkKb();
    // Initialize the graphical environment.
    setupLvgl();
    // Draw the user interface.
    ui();

    //SD card
    /*
    if(!setupSD())
        Serial.println("cannot configure SD card");
    else
        Serial.println("SD card detected");
    */
    //Load settings
    loadSettings();
    
    // set brightness.
    analogWrite(BOARD_BL_PIN, mapv(brightness, 0, 9, 100, 255));
    // Connect to a wifi network in history.
    if(wifi_connected_nets.list.size() > 0)
        xTaskCreatePinnedToCore(wifi_auto_connect, "wifi_auto", 10000, (void*)"ok", 1, &task_wifi_auto, 0);
    // Get the date from internet.
    if(wifi_connected)
        datetime();
    //Launch a date time task.
    xTaskCreatePinnedToCore(update_time, "update_time", 11000, (struct tm*)&timeinfo, 2, &task_date_time, 1);

    // Initial date, in case of no internet connection.
    setDate(2024, 1, 1, 0, 0, 0, 0);
    
    // Initialize the battery monitoring routine and lauch his task.
    initBat();
    xTaskCreatePinnedToCore(update_bat, "task_bat", 4000, NULL, 2, &task_bat, 1);

    // Launch the notification task.
    xTaskCreatePinnedToCore(notify, "notify", 4000, NULL, 1, &task_not, 1);
    // Launch de beacon task.
    xTaskCreatePinnedToCore(task_beacon, "beacon", 4000, NULL, 1, NULL, 1);
    // The ESP32S3 documentation says to avoid use the core 0 to run tasks or intensive routines. 
    xTaskCreatePinnedToCore(collectPackets, "collect_pkt", 11000, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(processPackets2, "process_pkt", 5000, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(processReceivedStats, "proc_stats_pkt", 3000, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(processTransmitingPackets, "proc_tx_pkt", 11000, NULL, 1, NULL, 1);
    // Turn off the display
    //delay(1000);
    //tft.writecommand(0x10);
    //delay(1000);
    // Turn on the display
    //tft.writecommand(0x11);
}

void loop(){
    pthread_mutex_lock(&lvgl_mutex);
    lv_task_handler();
    pthread_mutex_unlock(&lvgl_mutex);
    /*
    if(secureServer != NULL)
        if(secureServer->isRunning()){
            xSemaphoreTake(xSemaphore, portMAX_DELAY);
            secureServer->loop();
            xSemaphoreGive(xSemaphore);
        }*/
        
    delay(5);
}
