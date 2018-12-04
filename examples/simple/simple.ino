/**
 * aSysBus simple example
 * 
 * This example shows how a basic aSysBus-node can be made. Here we use a
 * CAN-Bus to send and receive messages. Inside the loop we send a message and 
 * output everything we received in a human readable format to UART
 */
 
#include "asb.h"

//Create new ASB node using ID 0x123 as local addess
//This is the "brain" and already handles routing between multiple interfaces and some messages like PING
ASB asb0(0x123);

//Start new CAN-Bus with 125KBps and 16MHz crystal using CS pin 10 and Interrupt pin 2
ASB_CAN asbCan0(10, CAN_125KBPS, MCP_16MHz, 2);

unsigned long lastSend;

void setup() {
  //Initialize Serial port
  Serial.begin(115200);
  Serial.println(F("ASB Test Node started"));

  //Attach the previously defined CAN-Bus to our controller
  Serial.print(F("Attach CAN..."));
  if(asb0.busAttach(&asbCan0) < 0) {
    Serial.println(F("Error!"));
  }else{
    Serial.println(F("done!"));
  }
}

void loop() {

  //Sending a message
  if(millis() > lastSend + 3000) { //every 3 seconds
    //Send a message to group 0x1234 instructing it to turn on
    byte data[2] = {ASB_CMD_1B, 1};
    byte sndStat = asb0.asbSend(ASB_PKGTYPE_MULTICAST, 0x1234, sizeof(data), data);
    if(sndStat == CAN_OK){
      Serial.println(F("Message sent successfully!"));
    } else {
      Serial.println(F("Error sending message..."));
    }
    lastSend = millis();
  }
  
  //loop() handles all recurring tasks like processing received messages and returns a packet
  asbPacket pkg = asb0.loop();
  if(pkg.meta.busId != -1) { //If bus is -1 there was no new packet received
    Serial.println(F("---"));
    Serial.print(F("Type: 0x"));
    Serial.println(pkg.meta.type, HEX);
    Serial.print(F("Target: 0x"));
    Serial.println(pkg.meta.target, HEX);
    Serial.print(F("Source: 0x"));
    Serial.println(pkg.meta.source, HEX);
    Serial.print(F("Port: 0x"));
    Serial.println(pkg.meta.port, HEX);
    Serial.print(F("Length: 0x"));
    Serial.println(pkg.len, HEX);
    for(byte i=0; i<pkg.len; i++) {
      Serial.print(F(" 0x"));
      Serial.print(pkg.data[i], HEX);
    }
    Serial.println();
  }
}
