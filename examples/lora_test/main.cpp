#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <utilities.h>

#define RADIO_FREQ          915.0
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
TaskHandle_t radioHandle = NULL;
bool transmissionFlag = true;
bool enableInterrupt = true;
int transmissionState ;
bool sender = false;
uint32_t sendCount = 0;
SemaphoreHandle_t xSemaphore = NULL;
uint32_t runningMillis = 0;

void setFlag(void)
{
    // check if the interrupt is enabled
    if (!enableInterrupt) {
        return;
    }
    // we got a packet, set the flag
    transmissionFlag = true;
}

bool setupRadio()
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
        return false;
    }


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

    // set the function that will be called
    // when new packet is received
    radio.setDio1Action(setFlag);
    return true;

}

void radioLoop(){
        digitalWrite(BOARD_SDCARD_CS, HIGH);
        digitalWrite(RADIO_CS_PIN, HIGH);
        digitalWrite(BOARD_TFT_CS, HIGH);

        char buf[256];

        

        String recv;

            int state = radio.readData(recv);
            if (state == RADIOLIB_ERR_NONE) {
                // packet was successfully received
                Serial.print(F("[RADIO] Received packet!"));

                // print data of the packet
                Serial.print(F(" Data:"));
                Serial.println(recv.isEmpty() ? " no data" : recv);

                // print RSSI (Received Signal Strength Indicator)
                Serial.print(F(" RSSI:"));
                Serial.print(radio.getRSSI());
                Serial.println(F(" dBm"));
                // snprintf(dispRecvicerBuff[1], sizeof(dispRecvicerBuff[1]), "RSSI:%.2f dBm", radio.getRSSI());

                // print SNR (Signal-to-Noise Ratio)
                Serial.print(F("  SNR:"));
                Serial.print(radio.getSNR());
                Serial.println(F(" dB"));


                snprintf(buf, 256, "RX:%s RSSI:%.2f SNR:%.2f\n", recv.c_str(), radio.getRSSI(), radio.getSNR());
                Serial.println(buf);

            } else if (state ==  RADIOLIB_ERR_CRC_MISMATCH) {
                // packet was received, but is malformed
                Serial.println(F("CRC error!"));

            } else {
                // some other error occurred
                Serial.print(F("failed, code "));
                Serial.println(state);
            }
            // put module back to listen mode
            state = radio.startReceive();
            if(state == RADIOLIB_ERR_NONE)
                Serial.println("Listening...");
            else{
                Serial.print("Failed to put in receive mode - code ");
                Serial.println(state);
            }
            // we're ready to receive more packets,
            // enable interrupt service routine
            enableInterrupt = true;
        
}



void setup(){
    Serial.begin(115200);
    setupRadio();
    delay(1000);

        int state = radio.startReceive();
        if (state == RADIOLIB_ERR_NONE) {
            Serial.println(F("success!"));
        } else {
            Serial.print(F("failed, code "));
            Serial.println(state);
        }


}

void loop(){
    radioLoop();
    delay(5000);
}