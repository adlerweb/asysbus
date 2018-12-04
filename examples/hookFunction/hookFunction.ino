/**
 * aSysBus hook function example
 * 
 * This example shows how to link a function to a set of metadata. This is especially
 * useful if you want to implement your own actors. Here we also use two interfaces,
 * namely a CAN-bus and UART which both can be used to send or receive packets. The
 * node will route incoming packets to the other interface.
 */
 
 #include "asb.h"

//Create new ASB node using ID 0x123 as local addess
//This is the "brain"
ASB asb0(0x123);

//Start new CAN-Bus with 125KBps and 8MHz crystal using CS pin 10 and Interrupt pin 2
ASB_CAN asbCan0(10, CAN_125KBPS, MCP_8MHz, 2);

//Start new UART-Bus using "Serial". Keep in mind to initialize it as usual in setup();
ASB_UART asbUart0(Serial);

//This is a custom actor, we link it later to a bus event
void testLight(asbPacket &pkg) {
    Serial.print(F("Light was switched "));
    if(pkg.data[1] == 1) {
      Serial.println("on");
      digitalWrite(9, HIGH);
    }else{
      Serial.println("off");
      digitalWrite(9, LOW);
    }
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
  
  //Execute local funktion testLight for messages matching this filter:
  //Type: every type (0xFF)
  //Target: 0x1001
  //Port: every port (-1)
  //First data byte: 0x51 (ASB_CMD_1B)
  Serial.print(F("Attach Test-Hook..."));
  asb0.hookAttach(0xFF, 0x1001, -1, ASB_CMD_1B, testLight);
  Serial.println(F("OK")); 

  //We use the LED in our function above. Note that pin 13 (usually LED_BUILTIN)
  //is already used for SCK
  pinMode(9, OUTPUT);
}

void loop() {
  //This handles all recurring tasks like processing received messages
  asb0.loop();
}
