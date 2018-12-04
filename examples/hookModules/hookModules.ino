/**
 * aSysBus hook module example
 * 
 * There are several modules to handle generic functions. This example uses two 
 * modules designed to handle the pins of an Arduino as inputs or outputs 
 * respectively.
 * When using modules their configuration is stored in EEPROM which itself will
 * get initialized on first boot. Later you can use bus messages to change the 
 * EEPROM and as such reconfigure the node remotely.
 */

#include "asb.h"

//Create new ASB node, use EEPROM address 0 - MAX for configuration
//This is the "brain"
ASB asb0(0, EEPROM.length());

//Start new CAN-Bus with 125KBps and 16MHz crystal using CS pin 10 and Interrupt pin 2
ASB_CAN asbCan0(10, CAN_125KBPS, MCP_16MHz, 2);

//Start new UART-Bus using "Serial". Keep in mind to initialize it as usual in setup();
ASB_UART asbUart0(Serial);

//Load new module for digital inputs, use ID 1 for configuration items
ASB_IO_DIN asbDIn0(1);

//Load new module for digital outputs use ID 2 for configuration items
ASB_IO_DOUT asbDOut0(2);

void firstboot() {
  //This is only executed if no configuration was stored in EEPROM. If you used your arduino
  //before don't forget to clear the EEPROM using the eeprom_clear example. Once configured
  //the node will ignore this function and use the stored values. These also can be canged
  //using bus-messages allowing for reconfiguration without the need to reflash the module.

  //Set local address to 0x123
  asb0.setNodeId(0x123);

  Serial.print(F("Attach Test-Output..."));
  //Output requests targeted at group 0x1234 to pin 9 using LED-dimming for %-messages
  //output is not inverted (0) and turned off after boot (0)
  //This also handles pinMode etc, so you don't need to do it manually
  asbDOut0.attach(0x1001, 9, ASB_IO_DOUT_LED, 0, 0);
  Serial.println(F("OK"));

  Serial.print(F("Attach Test-Input..."));
  //Send changes on pin 7 to group 0x1234, toggle on every high-pulse
  //Button is inverted (1) and internal pull-up active (1)
  //This also handles pinMode etc, so you don't need to do it manually
  asbDIn0.attach(0x1001, 7, ASB_IO_DIN_BTOGGLE, 1, 1);
  Serial.println(F("OK"));
}

void setup() {
  //Initialize Serial port
  Serial.begin(115200);

  //We can still use Serial for TX even if ASB is also using it as a bus
  //RX is not possible in this case
  Serial.println(F("ASB Test Node started"));

  //Attach the previously defined CAN-Bus to our controller
  Serial.print(F("Attach CAN..."));
  if(asb0.busAttach(&asbCan0) < 0) {
    Serial.println(F("Error!"));
  }else{
    Serial.println(F("done!"));
  }
  //Same for the UART-Bus
  Serial.print(F("Attach UART..."));
  if(asb0.busAttach(&asbUart0) < 0) {
    Serial.println(F("Error!"));
  }else{
    Serial.println(F("done!"));
  }

  //Bind the previously defined input module to our controller
  Serial.print(F("Attach Input-Module to ASB..."));
  asb0.hookAttachModule(&asbDIn0);
  Serial.println(F("OK")); 

  //Bind the previously defined output module to our controller
  Serial.print(F("Attach Output-Module to ASB..."));
  asb0.hookAttachModule(&asbDOut0);
  Serial.println(F("OK")); 

  //Execute firstboot if neccesary
  asb0.firstboot(firstboot);
}

void loop() {
  //Everything is handled in the asb0-object, not much to do here
  asb0.loop();
}
