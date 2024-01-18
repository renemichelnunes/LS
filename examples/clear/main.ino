#include "Arduino.h"
#include <nvs_flash.h>
void setup(){
    Serial.begin(115200);
    delay(5000);
    Serial.print("erasing...");
    nvs_flash_erase();
    Serial.print("done");
}

void loop(){
    
}
