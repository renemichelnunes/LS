#pragma once
#include <RadioLib.h>
#include "utilities.h"
#include <string>

class lora_radio{
    private:
        SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
        bool initBasicConfig();
    public:
        bool initialized = false;
        lora_radio();
        ~lora_radio();
        SX1262 * getRadio();
};  

struct lora_conf{
    String name;
    String id;
    uint8_t addr;
    float freq;
    float bandwidth;
    uint8_t spread_factor;
    uint8_t coding_rate;
    uint32_t sync_word;
    int8_t output_power;
    uint8_t current_limit;
    uint16_t preamble;
    bool crc;
};

class lora_settings{
    private:
        lora_conf settings;
    public:
        void setName(String name);
        void setId(String id);
        void setAddr(uint8_t addr);
        void setFreq(float freq);
        void setBandwidth(float bw);
        void setSpreadFactor(uint8_t sf);
        void setCodeRate(uint8_t cr);
        void setSyncWord(uint32_t sw);
        void setOutputPower(int8_t power);
        void setCurrentLimit(uint8_t limit);
        void setPreamble(uint16_t preamble);
        String getName();
        String getID();
        uint8_t getAddr();
        float getFreq();
        float getBandwidth();
        uint8_t getSpreadFactor();
        uint8_t getCodingRate();
        uint32_t getSyncWord();
        int8_t getOutputPower();
        uint8_t getCurrentLimit();
        uint16_t getPreamble();
        void load();
        void save();
        lora_settings();
        ~lora_settings();
};
