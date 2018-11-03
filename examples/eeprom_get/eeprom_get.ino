#include <EEPROM.h>

void setup() {
  byte check;
  
  Serial.begin(115200);
  for (unsigned int i = 0 ; i < EEPROM.length() ; i++) {
    check = EEPROM.read(i);
    Serial.print("0x");
    Serial.print(i, HEX);
    Serial.print(" => ");
    Serial.println(check, HEX);
  }
  Serial.print("Done.");
}

void loop() {}
