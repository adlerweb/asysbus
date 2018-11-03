#include <EEPROM.h>

void setup() {
  Serial.begin(115200);
  Serial.print("Erasing...");
  for (unsigned int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0xFF);
  }
  Serial.print("Done.");
}

void loop() {}
