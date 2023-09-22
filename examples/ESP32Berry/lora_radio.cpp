#include "lora_radio.hpp"
#include <RadioLib.h>

#define LORA_FREQ 915.0

bool lora_radio::initBasicConfig(){
    Serial.println("Starting basic configuration");

    digitalWrite(BOARD_SDCARD_CS, HIGH);
    digitalWrite(RADIO_CS_PIN, HIGH);
    digitalWrite(BOARD_TFT_CS, HIGH);
    SPI.end();
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); //SD

    int state = radio.begin(LORA_FREQ);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Radio start success!");
    } else {
        Serial.print("Radio start failed,code:");
        Serial.println(state);
        return false;
    }
    // set carrier frequency to 868.0 MHz
    if (radio.setFrequency(LORA_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Serial.println(F("Selected frequency is invalid for this module!"));
        return false;
    }else
    Serial.println("Carrier 915MHz");

    // set bandwidth to 250 kHz
    if (radio.setBandwidth(250.0) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        Serial.println(F("Selected bandwidth is invalid for this module!"));
        return false;
    }else
    Serial.println("Bandwidth 250KHz");

    // set spreading factor to 10
    if (radio.setSpreadingFactor(10) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        Serial.println(F("Selected spreading factor is invalid for this module!"));
        return false;
    }else
    Serial.println("Spread factor 10");

    // set coding rate to 6
    if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        Serial.println(F("Selected coding rate is invalid for this module!"));
        return false;
    }else 
    Serial.println("Coding rate 6");

    // set LoRa sync word to 0xAB
    if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
        Serial.println(F("Unable to set sync word!"));
        return false;
    }else
    Serial.println("Sync word 0xAB");

    // set output power to 10 dBm (accepted range is -17 - 22 dBm)
    if (radio.setOutputPower(17) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("Selected output power is invalid for this module!"));
        return false;
    }else
    Serial.println("Output power 10dBm");

    // set over current protection limit to 140 mA (accepted range is 45 - 140 mA)
    // NOTE: set value to 0 to disable overcurrent protection
    if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
        Serial.println(F("Selected current limit is invalid for this module!"));
        return false;
    }else
    Serial.println("Over current protection 140ma");

    // set LoRa preamble length to 15 symbols (accepted range is 0 - 65535)
    if (radio.setPreambleLength(15) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
        Serial.println(F("Selected preamble length is invalid for this module!"));
        return false;
    }else
    Serial.println("Preable 15 symbols");

    // disable CRC
    if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
        Serial.println(F("Selected CRC is invalid for this module!"));
        return false;
    }else
    Serial.println("No CRC check");
    return true;
    // disable CRC
    if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
        Serial.println(F("Selected CRC is invalid for this module!"));
        return false;
    }else
        Serial.println("No CRC check");
    Serial.println("Done");
    return true;
}

lora_radio::lora_radio(){
    radio.reset(false);
    if(initBasicConfig()){
        Serial.println("Radio configuration complete");
        initialized = true;
    }else{
        Serial.println("radio configuration failed");
        initialized = false;
    }
}

lora_radio::~lora_radio(){
    if(this != NULL){
        radio.reset();
        radio.sleep(false);
        initialized = false;
    }
}

SX1262 * lora_radio::getRadio(){
    return &radio;
}

lora_settings::lora_settings(){

}

lora_settings::~lora_settings(){
    
}

void lora_settings::setName(String name){
    this->conf.name = name;
}

void lora_settings::setId(String id){
    this->conf.id = id;
}

void lora_settings::setAddr(uint8_t addr){
    this->conf.addr = addr;
}

void lora_settings::setFreq(float freq){
    this->conf.freq = freq;
}

void lora_settings::setBandwidth(float bw){
    this->conf.bandwidth = bw;
}

void lora_settings::setSpreadFactor(uint8_t sf){
    this->conf.spread_factor = sf;
}

void lora_settings::setCodeRate(uint8_t cr){
    this->conf.coding_rate = cr;
}

void lora_settings::setSyncWord(uint32_t sw){
    this->conf.sync_word = sw;
}

void lora_settings::setOutputPower(int8_t power){
    this->conf.output_power = power;
}

void lora_settings::setCurrentLimit(uint8_t limit){
    this->conf.current_limit = limit;
}

void lora_settings::setPreamble(uint16_t preamble){
    this->conf.preamble = preamble;
}

String lora_settings::getName(){
    return this->conf.name;
}

String lora_settings::getID(){
    return this->conf.id;
}

uint8_t lora_settings::getAddr(){
    return this->conf.addr;
}

float lora_settings::getFreq(){
    return this->conf.freq;
}
float lora_settings::getBandwidth(){
    return this->conf.bandwidth;
}

uint8_t lora_settings::getSpreadFactor(){
    return this->conf.spread_factor;
}

uint8_t lora_settings::getCodingRate(){
    return this->conf.coding_rate;
}

uint32_t lora_settings::getSyncWord(){
    return this->conf.sync_word;
}

int8_t lora_settings::getOutputPower(){
    return this->conf.output_power;
}

uint8_t lora_settings::getCurrentLimit(){
    return this->conf.current_limit;
}

uint16_t lora_settings::getPreamble(){
    return this->conf.preamble;
}

void lora_settings::load(){

}

void lora_settings::save(){
    if(conf.name.isEmpty())
        return;
    Serial.println("===============LoRa Config================");
    Serial.print("Name: ");
    Serial.println(conf.name);
    Serial.print("ID: ");
    Serial.println(conf.id);
    Serial.print("Address: ");
    Serial.println(conf.addr);
    Serial.print("Frequency: ");
    Serial.println(conf.freq);
    Serial.print("Bandwidth: ");
    Serial.println(conf.bandwidth);
    Serial.print("Spread factor: ");
    Serial.println(conf.spread_factor);
    Serial.print("Coding rate: ");
    Serial.println(conf.coding_rate);
    Serial.print("Sync word: ");
    Serial.println(conf.sync_word);
    Serial.print("Output power: ");
    Serial.println(conf.output_power);
    Serial.print("Current limit: ");
    Serial.println(conf.current_limit);
    Serial.print("Preamble: ");
    Serial.println(conf.preamble);
    Serial.print("CRC check: ");
    Serial.println(conf.crc ? "True" : "False");
    Serial.println("==========================================");
}
