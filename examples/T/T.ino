#include <Arduino.h>
#include "utilities.h"
#include <Wire.h>
#include <TFT_eSPI.h>
#include <RadioLib.h>
#include <ui.hpp>


struct lora_packet{
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
TaskHandle_t thproc_recv_pkt = NULL;

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
    lora_packet dummy;
    if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE){
        if(hasRadio){
            int state = radio.startTransmit((uint8_t *)&my_packet, sizeof(lora_packet));
            if(state != RADIOLIB_ERR_NONE){
                Serial.print("transmission failed ");
                Serial.println(state);
            }else
                Serial.println("transmitted");
            int state = radio.startTransmit((uint8_t *)&dummy, sizeof(lora_packet));
        }
        xSemaphoreGive(xSemaphore);
    }
}

void hide_contacts_frm(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED){
        if(contacts_form != NULL){
            lv_obj_add_flag(contacts_form, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void show_contacts_form(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_SHORT_CLICKED){
        if(contacts_form != NULL){
            lv_obj_clear_flag(contacts_form, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui(){
    // Home screen**************************************************************
    init_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(init_screen, LV_HOR_RES, LV_VER_RES);
    
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
    contacts_form = lv_obj_create(lv_scr_act());
    lv_obj_set_size(contacts_form, LV_HOR_RES, LV_VER_RES);

    lv_obj_t * list = lv_list_create(contacts_form);
    lv_obj_set_size(list, LV_HOR_RES - 20, 210);
    lv_obj_align(list, LV_ALIGN_LEFT_MID, -15, 0);
    lv_obj_set_style_border_opa(list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_list_add_btn(list, LV_SYMBOL_WARNING, "Broadcast");

    // Add contact button
    lv_obj_t * btn_frm_contatcs_add = lv_btn_create(contacts_form);
    lv_obj_set_size(btn_frm_contatcs_add, 60, 25);
    lv_obj_set_align(btn_frm_contatcs_add, LV_ALIGN_BOTTOM_LEFT);
    //lv_obj_set_pos(btn_contacts, 10, -10);
    lv_obj_t * lbl_btn_frm_contacts_add = lv_label_create(btn_frm_contatcs_add);
    lv_label_set_text(lbl_btn_frm_contacts_add, "Add");
    lv_obj_align(lbl_btn_frm_contacts_add, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_frm_contatcs_add, hide_contacts_frm, LV_EVENT_SHORT_CLICKED, NULL);

    // Close button
    btn_frm_contatcs_close = lv_btn_create(contacts_form);
    lv_obj_set_size(btn_frm_contatcs_close, 60, 25);
    lv_obj_set_align(btn_frm_contatcs_close, LV_ALIGN_BOTTOM_RIGHT);
    //lv_obj_set_pos(btn_contacts, 10, -10);
    lbl_btn_frm_contacts_close = lv_label_create(btn_frm_contatcs_close);
    lv_label_set_text(lbl_btn_frm_contacts_close, "Close");
    lv_obj_align(lbl_btn_frm_contacts_close, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_frm_contatcs_close, hide_contacts_frm, LV_EVENT_SHORT_CLICKED, NULL);

    

    // Hide contacts form
    lv_obj_add_flag(contacts_form, LV_OBJ_FLAG_HIDDEN);

    // Add contacts form**************************************************************

    // Edit contacts form**************************************************************

    // Chat form**************************************************************
}

void setup(){
    bool ret = false;
    Serial.begin(115200);
    delay(2000);
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
}

void loop(){
    lv_task_handler();
    delay(5);
}