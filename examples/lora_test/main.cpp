/*
   RadioLib SX126x FSK Modem Example

   This example shows how to use FSK modem in SX126x chips.

   NOTE: The sketch below is just a guide on how to use
         FSK modem, so this code should not be run directly!
         Instead, modify the other examples to use FSK
         modem and use the appropriate configuration
         methods.

   For default module settings, see the wiki page
   https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---fsk-modem

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>
#include <utilities.h>
#include <Arduino.h>
#include <SPI.h>
// SX1262 has the following connections:
// NSS pin:   10
// DIO1 pin:  2
// NRST pin:  3
// BUSY pin:  9
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
SemaphoreHandle_t xSemaphore = NULL;
bool        transmissionFlag = true;
bool        enableInterrupt = true;
bool hasRadio = false;
int         transmissionState ;
bool        sender = true;
uint32_t    sendCount = 0;
// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1262 radio = RadioShield.ModuleA;

// or using CubeCell
//SX1262 radio = new Module(RADIOLIB_BUILTIN_MODULE);
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

    int state = radio.begin(915.0);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Start Radio success!");
    } else {
        Serial.print("Start Radio failed,code:");
        Serial.println(state);
        return false;
    }

    hasRadio = true;

    // set carrier frequency to 868.0 MHz
    if (radio.setFrequency(915.0) == RADIOLIB_ERR_INVALID_FREQUENCY) {
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

void loopRadio(){
  if ( xSemaphoreTake( xSemaphore, portMAX_DELAY ) == pdTRUE ) {
    String recv;

        // check if the flag is set
    Serial.println(transmissionFlag);
    transmissionFlag = true;
    if (transmissionFlag) {
        // disable the interrupt service routine while
        // processing the data
        enableInterrupt = false;

        // reset flag
        transmissionFlag = false;

        // you can read received data as an Arduino String
        // int state = radio.readData(recv);

        // you can also read received data as byte array
        /*
        */
        int state = radio.readData(recv);
        if (state == RADIOLIB_ERR_NONE) {


            // packet was successfully received
            Serial.print(F("[RADIO] Received packet!"));

            // print data of the packet
            Serial.print(F(" Data:"));
            Serial.print(recv);

            // print RSSI (Received Signal Strength Indicator)
            Serial.print(F(" RSSI:"));
            Serial.print(radio.getRSSI());
            Serial.print(F(" dBm"));
            // snprintf(dispRecvicerBuff[1], sizeof(dispRecvicerBuff[1]), "RSSI:%.2f dBm", radio.getRSSI());

            // print SNR (Signal-to-Noise Ratio)
            Serial.print(F("  SNR:"));
            Serial.print(radio.getSNR());
            Serial.println(F(" dB"));



        } else if (state ==  RADIOLIB_ERR_CRC_MISMATCH) {
            // packet was received, but is malformed
            Serial.println(F("CRC error!"));

        } else {
            // some other error occurred
            Serial.print(F("failed, code "));
            Serial.println(state);
        }
        // put module back to listen mode
        radio.startReceive();

        // we're ready to receive more packets,
        // enable interrupt service routine
        enableInterrupt = true;
    }
    xSemaphoreGive( xSemaphore );
  }
}

void initBoard()
{
    bool ret = 0;

    Serial.begin(115200);
    Serial.println("T-DECK factory");

    //! The board peripheral power control pin needs to be set to HIGH when using the peripheral
    pinMode(BOARD_POWERON, OUTPUT);
    digitalWrite(BOARD_POWERON, HIGH);

    //! Set CS on all SPI buses to high level during initialization
    pinMode(BOARD_SDCARD_CS, OUTPUT);
    pinMode(RADIO_CS_PIN, OUTPUT);
    pinMode(BOARD_TFT_CS, OUTPUT);

    digitalWrite(BOARD_SDCARD_CS, HIGH);
    digitalWrite(RADIO_CS_PIN, HIGH);
    digitalWrite(BOARD_TFT_CS, HIGH);

    pinMode(BOARD_SPI_MISO, INPUT_PULLUP);
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); //SD

    pinMode(BOARD_BOOT_PIN, INPUT_PULLUP);
    pinMode(BOARD_TBOX_G02, INPUT_PULLUP);
    pinMode(BOARD_TBOX_G01, INPUT_PULLUP);
    pinMode(BOARD_TBOX_G04, INPUT_PULLUP);
    pinMode(BOARD_TBOX_G03, INPUT_PULLUP);

    //Wakeup touch chip
    pinMode(BOARD_TOUCH_INT, OUTPUT);
    digitalWrite(BOARD_TOUCH_INT, HIGH);

    //Add mutex to allow multitasking access
    xSemaphore = xSemaphoreCreateBinary();
    assert(xSemaphore);
    xSemaphoreGive( xSemaphore );

    // Set touch int input
    pinMode(BOARD_TOUCH_INT, INPUT); delay(20);




    if ( xSemaphoreTake( xSemaphore, portMAX_DELAY ) == pdTRUE ) {
        if (hasRadio) {
            if (sender) {
                transmissionState = radio.startTransmit("0");
                sendCount = 0;
                Serial.println("startTransmit!!!!");
            } else {
                int state = radio.startReceive();
                if (state == RADIOLIB_ERR_NONE) {
                    Serial.println(F("success!"));
                } else {
                    Serial.print(F("failed, code "));
                    Serial.println(state);
                }
            }
        }
        xSemaphoreGive( xSemaphore );
    }


}

void setup() {
  Serial.begin(9600);
    delay(5000);
  // initialize SX1262 FSK modem with default settings
    digitalWrite(BOARD_SDCARD_CS, HIGH);
    digitalWrite(RADIO_CS_PIN, HIGH);
    digitalWrite(BOARD_TFT_CS, HIGH);
    SPI.end();
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); //SD
    initBoard();
  setupRadio();

  radio.startReceive();
}

void loop() {
  loopRadio();
  delay(5);
}
