#include "lora_radio.hpp"
#include <RadioLib.h>

#define basic_config 1

#define LORA_FREQ 915.0
int count = 1;

bool lora_radio::initFSKBasicConfig()
{   
    if(count > 1)
        return true;
    count++;
    digitalWrite(BOARD_SDCARD_CS, HIGH);
    digitalWrite(RADIO_CS_PIN, HIGH);
    digitalWrite(BOARD_TFT_CS, HIGH);
    SPI.end();
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);
    //delay(5000);
    int16_t state;


    state = radio.beginFSK();
    if(state == RADIOLIB_ERR_NONE)
        Serial.println(F("FSK mode done"));
    else{
        Serial.print(F("failed to set FSK mode - code "));
        Serial.println(state);
        return false;
    }

    state = radio.setTCXO(2.4);
    if(state == RADIOLIB_ERR_NONE)
        Serial.println(F("TCXO done"));
    else{
        Serial.print(F("failed to set TCXO - code "));
        Serial.println(state);
        return false;
    }
    
    state = radio.setFrequency(LORA_FREQ);
    if(state == RADIOLIB_ERR_NONE)
        Serial.println(F("LoRa frequency 915.0"));
    else{
        Serial.print(F("failed to set chip carrier - code "));
        Serial.println(state);
        return false;
    }

    state = radio.setBitRate(100.0);
    if(state == RADIOLIB_ERR_NONE)
        Serial.println(F("bit rate 100"));
    else{
        Serial.print(F("failed to set bit rate - code "));
        Serial.println(state);
        return false;
    }

    state = radio.setFrequencyDeviation(10.0);
    if(state == RADIOLIB_ERR_NONE)
        Serial.println(F("frequency deviation 10"));
    else{
        Serial.print(F("failed to set frequency deviation - code "));
        Serial.println(state);
        return false;
    }

    state = radio.setRxBandwidth(234.3);
    if(state == RADIOLIB_ERR_NONE)
        Serial.println(F("RX bandwidth 234.3"));
    else{
        Serial.print(F("failed to set RX bandwidth - code "));
        Serial.println(state);
        return false;
    }

    state = radio.setOutputPower(10.0);
    if(state == RADIOLIB_ERR_NONE)
        Serial.println(F("Output power 10"));
    else{
        Serial.print(F("failed to set output power - code "));
        Serial.println(state);
        return false;
    }

    state = radio.setCurrentLimit(100.0);
    if(state == RADIOLIB_ERR_NONE)
        Serial.println(F("Current limit 100mah"));
    else{
        Serial.print(F("failed to set current limit - code "));
        Serial.println(state);
        return false;
    }

    state = radio.setDataShaping(RADIOLIB_SHAPING_1_0);
    if(state == RADIOLIB_ERR_NONE)
        Serial.println(F("Data shaping set to 1_0"));
    else{
        Serial.print(F("failed to set data shaping - code "));
        Serial.println(state);
        return false;
    }

    uint8_t syncWord[] = {0x01, 0x23, 0x45, 0x67,
                        0x89, 0xAB, 0xCD, 0xEF};
    state = radio.setSyncWord(syncWord, 8);
    if(state == RADIOLIB_ERR_NONE)
        Serial.println(F("sync word applied"));
    else{
        Serial.print(F("failed to set sync work - code "));
        Serial.println(state);
        return false;
    } 

    state = radio.setSyncBits(syncWord, 12);
    if(state == RADIOLIB_ERR_NONE)
        Serial.println(F("sync bits applied 12"));
    else{
        Serial.print(F("failed to set sync bits 12 - code "));
        Serial.println(state);
        return false;
    }

    state = radio.setCRC(2, 0xFFFF, 0x8005, false);
    if(state == RADIOLIB_ERR_NONE)
        Serial.println(F("CRC applied"));
    else{
        Serial.print(F("failed to set CRC - code "));
        Serial.println(state);
        return false;
    }

    state = radio.disableAddressFiltering();
    if(state == RADIOLIB_ERR_NONE)
        Serial.println(F("Address filtering disabled"));
    else{
        Serial.print(F("failed to disable address filtering - code "));
        Serial.println(state);
        return false;
    }

    return true;
}

bool lora_radio::initBasicConfig(){
    if(count > 1)
        return true;
    Serial.print("count ");
    Serial.println(count);
    count++;
    Serial.println(F("Starting basic configuration"));

    digitalWrite(BOARD_SDCARD_CS, HIGH);
    digitalWrite(RADIO_CS_PIN, HIGH);
    digitalWrite(BOARD_TFT_CS, HIGH);
    SPI.end();
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); //SD
    delay(5000);
    int state = radio.begin();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("Radio start success!"));
    } else {
        Serial.print(F("Radio start failed,code:"));
        Serial.println(state);
        return false;
    }

    // set carrier frequency to 868.0 MHz
    if (radio.setFrequency(LORA_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Serial.println(F("Selected frequency is invalid for this module!"));
        return false;
    }else
    Serial.println(F("Carrier 915MHz"));
    
    //radio.setTCXO(2.4);

    #if basic_config == 1
    /*Disable address filtering*/
    //radio.disableAddressFiltering();

    // set bandwidth to 250 kHz
    if (radio.setBandwidth(250.0) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        Serial.println(F("Selected bandwidth is invalid for this module!"));
        return false;
    }else
    Serial.println(F("Bandwidth 250KHz"));

    // set spreading factor to 10
    if (radio.setSpreadingFactor(10) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        Serial.println(F("Selected spreading factor is invalid for this module!"));
        return false;
    }else
    Serial.println(F("Spread factor 10"));

    // set coding rate to 6
    if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        Serial.println(F("Selected coding rate is invalid for this module!"));
        return false;
    }else 
    Serial.println(F("Coding rate 6"));

    // set LoRa sync word to 0xAB
    if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
        Serial.println(F("Unable to set sync word!"));
        return false;
    }else
    Serial.println(F("Sync word 0xAB"));

    // set output power to 10 dBm (accepted range is -17 - 22 dBm)
    if (radio.setOutputPower(17) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("Selected output power is invalid for this module!"));
        return false;
    }else
    Serial.println(F("Output power 17dBm"));

    // set over current protection limit to 140 mA (accepted range is 45 - 140 mA)
    // NOTE: set value to 0 to disable overcurrent protection
    if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
        Serial.println(F("Selected current limit is invalid for this module!"));
        return false;
    }else
    Serial.println(F("Over current protection 140ma"));

    // set LoRa preamble length to 15 symbols (accepted range is 0 - 65535)
    if (radio.setPreambleLength(16) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
        Serial.println(F("Selected preamble length is invalid for this module!"));
        return false;
    }else
    Serial.println(F("Preable 15 symbols"));

    // disable CRC
    if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
        Serial.println(F("Selected CRC is invalid for this module!"));
        return false;
    }else
    Serial.println(F("No CRC check"));
    return true;
    // disable CRC
    if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
        Serial.println(F("Selected CRC is invalid for this module!"));
        return false;
    }else
        Serial.println(F("No CRC check"));
    #endif
    Serial.println(F("Done"));
    return true;
}

lora_radio::lora_radio(){
    radio.reset(false);
    if(initBasicConfig()){
        Serial.println(F("Radio configuration complete"));
        initialized = true;
    }else{
        Serial.println(F("Radio configuration failed"));
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
    conf.name = "";
    conf.id = "";
    conf.addr = 0;
    conf.current_limit = 5;
    conf.bandwidth = 8;
    conf.spread_factor = 5;
    conf.coding_rate = 1;
    conf.sync_word = 0xAB;
    conf.tx_power = 5;
    conf.preamble = 15;
    conf.freq = 2;
    conf.crc = false;
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

void lora_settings::setFreq(uint16_t freq){
    this->conf.freq = freq;
}

void lora_settings::setBandwidth(uint16_t bw){
    this->conf.bandwidth = bw;
}

void lora_settings::setSpreadFactor(uint16_t sf){
    this->conf.spread_factor = sf;
}

void lora_settings::setCodeRate(uint16_t cr){
    this->conf.coding_rate = cr;
}

void lora_settings::setSyncWord(uint32_t sw){
    this->conf.sync_word = sw;
}

void lora_settings::setTXPower(uint16_t power){
    this->conf.tx_power = power;
}

void lora_settings::setCurrentLimit(uint16_t limit){
    this->conf.current_limit = limit;
}

void lora_settings::setPreamble(uint16_t preamble){
    this->conf.preamble = preamble;
}

void lora_settings::setCRC(bool check)
{
    this->conf.crc = check;
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

uint16_t lora_settings::getFreq(){
    return this->conf.freq;
}
uint16_t lora_settings::getBandwidth(){
    return this->conf.bandwidth;
}

uint16_t lora_settings::getSpreadFactor(){
    return this->conf.spread_factor;
}

uint16_t lora_settings::getCodingRate(){
    return this->conf.coding_rate;
}

uint32_t lora_settings::getSyncWord(){
    return this->conf.sync_word;
}

uint16_t lora_settings::getTXPower(){
    return this->conf.tx_power;
}

uint16_t lora_settings::getCurrentLimit(){
    return this->conf.current_limit;
}

uint16_t lora_settings::getPreamble(){
    return this->conf.preamble;
}

void lora_settings::load(){

}

bool lora_settings::getCRC()
{
    return this->conf.crc;
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
    Serial.println(conf.tx_power);
    Serial.print("Current limit: ");
    Serial.println(conf.current_limit);
    Serial.print("Preamble: ");
    Serial.println(conf.preamble);
    Serial.print("CRC check: ");
    Serial.println(conf.crc ? "True" : "False");
    Serial.println("==========================================");
}
