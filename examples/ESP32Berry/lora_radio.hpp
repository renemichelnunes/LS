#pragma once
#include <RadioLib.h>
#include "utilities.h"
#include <string>
#include <Preferences.h>

struct lora_conf{
    String name;
    String id;
    uint8_t addr;
    uint16_t freq;
    uint16_t bandwidth;
    uint16_t spread_factor;
    uint16_t coding_rate;
    uint32_t sync_word;
    uint16_t tx_power;
    uint16_t current_limit;
    uint16_t preamble;
    bool crc;
};

class lora_settings{
    private:
        lora_conf conf;
    public:
        void setName(String name);
        void setId(String id);
        void setAddr(uint8_t addr);
        void setFreq(uint16_t freq);
        void setBandwidth(uint16_t bw);
        void setSpreadFactor(uint16_t sf);
        void setCodeRate(uint16_t cr);
        void setSyncWord(uint32_t sw);
        void setTXPower(uint16_t power);
        void setCurrentLimit(uint16_t limit);
        void setPreamble(uint16_t preamble);
        void setCRC(bool check);
        String getName();
        String getID();
        uint8_t getAddr();
        uint16_t getFreq();
        uint16_t getBandwidth();
        uint16_t getSpreadFactor();
        uint16_t getCodingRate();
        uint32_t getSyncWord();
        uint16_t getTXPower();
        uint16_t getCurrentLimit();
        uint16_t getPreamble();
        bool getCRC();
        void load();
        void save();
        lora_settings();
        ~lora_settings();
};

class lora_radio{
    private:
        SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
        bool initBasicConfig();
        bool initFSKBasicConfig();
    public:
        uint8_t vcurrent_limit[12] = {45, 60, 80, 100, 120, 140, 160, 180, 200, 220, 240, 0};
        float vbandwidth[10] = {7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0, 500.0};
        uint8_t vspread_factor[8] = {5, 6, 7, 8, 9, 10, 11, 12};
        uint8_t vcoding_rate[4] = {5, 6, 7, 8};
        int8_t vtx_power[9] = {-17, -10, -5, 0, 5, 10, 15, 20, 22};
        float vlora_freq[3] = {433.0, 868.0, 915.0};
        bool initialized = false;
        lora_settings settings;
        lora_radio();
        ~lora_radio();
        SX1262 * getRadio();
};  

