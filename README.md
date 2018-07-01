# aSysBus - The Arduino System Bus

aSysBus is hard- and software to build a network of arduino nodes using a CAN-bus or other interfaces. It was build as a replacement for [iSysBus<sup>(DE)</sup>](https://www.mikrocontroller.net/articles/Hausbus_Diskussion), which used native AVR code and a java based configuration framework instead of Arduino. It is mostly used for home automation and other control communication.

For CAN communication [Seeed-Studio/CAN_BUS_Shield](https://github.com/Seeed-Studio/CAN_BUS_Shield) is required. Be aware the library frequently breaks backwards compatibility. As time of writing master should work, if problems occur look for a commit close to a commit of this library and file a bug here.

Take a look around the wiki to learn more about the protocol, the included examples should help to get you started. If you speak german there are several videos over at [YouTube](https://www.youtube.com/user/adlerweb/search?query=aSysBus).
 
