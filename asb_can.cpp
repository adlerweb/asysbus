/**
  aSysBus CAN interface

  @copyright 2015-2017 Florian Knodt, www.adlerweb.info

  Based on iSysBus - 2010 Patrick Amrhein, www.isysbus.org

  This interface depends on the CAN_BUS_Shield library.
    Download: https://github.com/Seeed-Studio/CAN_BUS_Shield

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ASB_CAN__C
#define ASB_CAN__C
    #include "asb_can.h"
    #include "asb_proto.h"
    #include <mcp_can.h>
    #include <SPI.h>

    volatile bool asb_CANIntReq=false;

    void asb_CANInt() {
      asb_CANIntReq=true;
    }

    /*
     * @TODO communication without INT
     */
    /*ASB_CAN::ASB_CAN(byte cs, byte speed, byte clock) {
        ASB_CAN::interface = MCP_CAN(cs);
        lastErr = ASB_CAN::interface.begin(MCP_EXT, speed, clock);
        if(lastErr == CAN_OK) ASB_CAN::interface.setMode(MODE_NORMAL);
        wakelock = 1;
        #error Communication without interrups isn't implemented yet
    }*/


    ASB_CAN::ASB_CAN(byte cs, byte speed, byte clockspd, byte interrupt) :
    _interface(cs) {
        pinMode(interrupt, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(interrupt), asb_CANInt, FALLING);
        _intPin = interrupt;
        _speed = speed;
        _clockspd = clockspd;
        //Begin doesn't work here!
    }

    byte ASB_CAN::begin() {
        lastErr = _interface.begin(_speed, _clockspd);
        return lastErr;
    }

    asbMeta ASB_CAN::asbCanAddrParse(unsigned long canAddr) {
        asbMeta temp;

        temp.target = 0;
        temp.source = 0;
        temp.port = -1;

        temp.type = ((canAddr >> 28) & 0x03);
        temp.source = (canAddr & 0x7FF);
        temp.target = ((canAddr >> 11) & 0xFFFF);

        if(temp.type == 0x03) { //Unicast
            temp.port = ((canAddr >> 23) & 0x1F);
            temp.target &= 0x7FF;
        }

        return temp;
    }

    unsigned long ASB_CAN::asbCanAddrAssemble(asbMeta meta) {
        return asbCanAddrAssemble(meta.type, meta.target, meta.source, meta.port);
    }

    unsigned long ASB_CAN::asbCanAddrAssemble(byte type, unsigned int target, unsigned int source) {
        return asbCanAddrAssemble(type, target, source, -1);
    }

    unsigned long ASB_CAN::asbCanAddrAssemble(byte type, unsigned int target, unsigned int source, char port) {
          unsigned long addr = 0x80000000;

          if(type > 0x03) return 0;
          addr |= ((unsigned long)type << 28);

          if(type == ASB_PKGTYPE_UNICAST) {
            if(target > 0x7FF) return 0;
            if(port < 0 ||port > 0x1F) return 0;

            addr |= ((unsigned long)port << 23);
          }else{
            if(target > 0xFFFF) return 0;
          }

          addr |= ((unsigned long)target << 11);

          if(source > 0x7FF) return 0;
          addr |= source;

          return addr;
    }

    bool ASB_CAN::asbSend(byte type, unsigned int target, unsigned int source, char port, byte len, const byte *data) {
        unsigned long addr = asbCanAddrAssemble(type, target, source, port);
        if(addr == 0) return false;

        lastErr = _interface.sendMsgBuf(addr, 1, len, data);
        if(lastErr != CAN_OK) return false;
        return true;
    }

    bool ASB_CAN::asbReceive(asbPacket &pkg) {

        unsigned long rxId;
        byte len = 0;
        byte rxBuf[8];

        if(_interface.checkReceive() != CAN_MSGAVAIL) return false;

        byte state = _interface.readMsgBufID(&rxId, &len, rxBuf);

        if(state != CAN_OK) return false;

        pkg.meta = asbCanAddrParse(rxId);
        pkg.len = len;

        for(byte i=0; i<len; i++) pkg.data[i] = rxBuf[i];

        return true;
    }


#endif /* ASB_CAN__C */
