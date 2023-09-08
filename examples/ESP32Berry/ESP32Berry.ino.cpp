# 1 "/tmp/tmpb93rjdos"
#include <Arduino.h>
# 1 "/home/rene/Documentos/T-Deck/examples/ESP32Berry/ESP32Berry.ino"
# 9 "/home/rene/Documentos/T-Deck/examples/ESP32Berry/ESP32Berry.ino"
#include "ESP32Berry.hpp"

ESP32Berry *_ESP32Berry;
void setup();
void loop();
void initBoard();
#line 13 "/home/rene/Documentos/T-Deck/examples/ESP32Berry/ESP32Berry.ino"
void setup() {
  Serial.begin(115200);
  initBoard();
  _ESP32Berry = new ESP32Berry();
  _ESP32Berry->begin();
}

void loop() {}

void initBoard(){
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
}