#pragma once
#include <RadioLib.h>
#include "utilities.h"

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