#include "lora_radio.hpp"
#include <RadioLib.h>

#define LORA_FREQ 915.0

bool lora_radio::initBasicConfig(){
    Serial.println("Initialising with standard configuration");

    digitalWrite(BOARD_SDCARD_CS, HIGH);
    digitalWrite(RADIO_CS_PIN, HIGH);
    digitalWrite(BOARD_TFT_CS, HIGH);
    SPI.end();
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); //SD

    int state = radio.begin(LORA_FREQ);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Start Radio success!");
    } else {
        Serial.print("Start Radio failed,code:");
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
    return true;
}

lora_radio::lora_radio(){
    Serial.println("initialising radio");
    radio.reset(false);
    if(initBasicConfig()){
        Serial.println("Radio initialisation complete");
        initialized = true;
    }else
        Serial.println("radio initialisation failed");
}

lora_radio::~lora_radio(){
    
}

SX1262 * lora_radio::getRadio(){
    return &radio;
}