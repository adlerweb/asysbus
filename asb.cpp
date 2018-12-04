/**
  aSysBus bus interface

  @copyright 2015-2017 Florian Knodt, www.adlerweb.info

  Based on iSysBus - 2010 Patrick Amrhein, www.isysbus.org

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

#ifndef ASB__C
#define ASB__C
    #include "asb.h"
    #include "asb_proto.h"

    ASB::ASB(unsigned int start, unsigned int stop) {
        if(_cfgAddrStop < _cfgAddrStart+2) _cfgAddrStop = _cfgAddrStart;

        //Read NodeID from EEPROM
        unsigned int cfg = 0xFFFF;
        EEPROM.get(_cfgAddrStart, cfg);
        setNodeId(cfg);

        _cfgAddrStop = stop;
        _cfgAddrStart = start;
    }

    ASB::ASB(unsigned int id) {
        setNodeId(id);

        _cfgAddrStop = 0;
        _cfgAddrStart = 0;
    }

    bool ASB::firstboot(void (*function)()) {
        if(function == NULL) return false;
        if(_nodeId < 0x0001 || _nodeId > 0x07FF) {
            #ifdef ASB_DEBUG
                Serial.print(F("Running initial configuration")); Serial.println(); Serial.flush();
            #endif
            function();
            return true;
        }else{
            return false;
        }
    }

    bool ASB::setNodeId(unsigned int id) {
        if(id < 0x0001 || id > 0x07FF) return false;
        _nodeId = id;
        EEPROM.put(_cfgAddrStart, id);
        return true;
    }

    char ASB::busAttach(ASB_COMM *bus) {
        for(signed char busId=0; busId<ASB_BUSNUM; busId++) {
            if(_busAddr[busId] == 0x00) {
                _busAddr[busId] = bus;
                byte err = bus->begin();
                if(err == 0) {
                    //Boot message
                    byte data[1] = {ASB_CMD_BOOT};
                    bus->asbSend(ASB_PKGTYPE_BROADCAST, 0x00, _nodeId, -1, sizeof(data), data);

                    return busId;
                }else{
                    _busAddr[busId] = 0x00;
                }
                return -2;
            }
        }
        return -1;
    }

    bool ASB::busDetach(signed char busId) {
        if(busId < 0 || busId >= ASB_BUSNUM) return false;
        if(_busAddr[busId] == 0x00) return false;
        _busAddr[busId] = 0x00;
        return true;
    }

    unsigned int ASB::cfgFindFreeblock(byte bytes, byte id) {
        if(_cfgAddrStart == _cfgAddrStop)  {
            #ifdef ASB_DEBUG
                Serial.print(F("EEPROM space 0")); Serial.println(); Serial.flush();
            #endif
            return 0;
        }
        
        bytes++; //Header
        
        #ifdef ASB_DEBUG
            Serial.print(F("ID is ")); Serial.println(id, HEX); Serial.flush();
            Serial.print(F("size is ")); Serial.println(bytes, HEX); Serial.flush();
        #endif

        unsigned int address = _cfgAddrStart+2; //bytes 1+2 are our ID
        byte check,len;

        if(((int)_cfgAddrStop-_cfgAddrStart-bytes) < 0) {
            #ifdef ASB_DEBUG
                Serial.print(F("Space < length")); Serial.println(); Serial.flush();
            #endif
            return 0;
        }

        do {
            check = EEPROM.read(address);
            #ifdef ASB_DEBUG
                Serial.print(F("check addr ")); Serial.println(address); Serial.flush();
                Serial.print(F(" = ")); Serial.println(check, HEX); Serial.flush();
            #endif
            if(check == 0xFF || check == 0) { //Nothing saved yet
                len = 0;
                while(((1 << len) + 5) < bytes) {
                    #ifdef ASB_DEBUG
                        Serial.print(F("Testing length")); Serial.println(len); Serial.flush();
                    #endif
                    len++;
                    if(len > 0x0F) {
                        #ifdef ASB_DEBUG
                            Serial.print(F("length exceeded")); Serial.println(); Serial.flush();
                        #endif
                        return 0;
                    }
                }
                #ifdef ASB_DEBUG
                    Serial.print(F("final length")); Serial.println(len); Serial.flush();
                #endif

                check  = (id << 4) | len;
                
                #ifdef ASB_DEBUG
                    Serial.print(F("ID is now ")); Serial.println(check, HEX); Serial.flush();
                #endif
                EEPROM.write(address, check);

                return address;
            }

            len = ((1 << (check & 0x0F)) + 5);
            if((check & 0xF0) == 0x00 && len >= bytes) { //block is marked as free and has enough free space
                //@TODO try to split into multiple blocks
                return address;
            //@TODO check if adjacent block is also free
            }else{
                address += len;
            }
        }while(address+bytes < _cfgAddrStop);
        return 0;
    }

    byte ASB::asbSend(asbMeta meta, byte len, byte *data) {
        return asbSend(meta.type, meta.target, meta.source, meta.port, len, data, meta.busId);
    }

    byte ASB::asbSend(byte type, unsigned int target, byte len, byte *data) {
        return asbSend(type, target, _nodeId, -1, len, data, -1);
    }

    byte ASB::asbSend(byte type, unsigned int target, char port, byte len, byte *data) {
        return asbSend(type, target, _nodeId, port, len, data, -1);
    }

    byte ASB::asbSend(byte type, unsigned int target, unsigned int source, char port, byte len, byte *data, signed char skip) {
        bool state;
        byte errors=0,i;

        if(source == 0) source = _nodeId;

        for(signed char busId=0; busId<ASB_BUSNUM; busId++) {
            if(_busAddr[busId] != NULL && busId != skip) {
                state = _busAddr[busId]->asbSend(type, target, source, port, len, data);
                if(!state) errors++;
            }
        }

        if(skip < 0) {
            //Local source, check for actions
            asbPacket pkg;
            pkg.meta.type = type;
            pkg.meta.target = target;
            pkg.meta.source = source;
            pkg.meta.port = port;
            pkg.len = len;
            for(i=0; i<len; i++) pkg.data[i] = data[i];

            asbProcess(pkg);
        }
        return errors;
    }

    bool ASB::asbReceive(asbPacket &pkg) {
        return asbReceive(pkg, true);
    }

    void ASB::asbReceive(void) {
        asbPacket pkg;
        asbReceive(pkg, true);
    }

    bool ASB::asbReceive(asbPacket &pkg, bool routing) {
        bool check = false;

        for(signed char busId=0; busId<ASB_BUSNUM; busId++) {
            if(_busAddr[busId] != NULL) {
                check = _busAddr[busId]->asbReceive(pkg);
                if(check) {
                    pkg.meta.busId = busId;

                    asbProcess(pkg);

                    if(routing) {
                        //Resend to every interface except the one we received it on
                        asbSend(pkg.meta.type, pkg.meta.target, pkg.meta.source, pkg.meta.port, pkg.len, pkg.data, pkg.meta.busId);
                    }
                    return true;
                }
            }
        }

        return false;
    }

    void ASB::asbProcess(asbPacket &pkg) {
        byte i;
        byte data[8];
        
        //Internal logic
        if(pkg.len >= 1) {
            switch(pkg.data[0]) {
                case ASB_CMD_PING:
                    if(pkg.meta.type != ASB_PKGTYPE_UNICAST || pkg.meta.target != _nodeId) break;
                    data[0] = ASB_CMD_PONG;
                    _busAddr[pkg.meta.busId]->asbSend(ASB_PKGTYPE_UNICAST, pkg.meta.source, _nodeId, pkg.meta.port, 1, data);
                break;
                //@todo config
                //@todo nodeid
            }
        }

        //modules
        for(i=0; i<ASB_MODNUM; i++) {
            if(_module[i] != NULL) {
                Serial.print(F("exec")); Serial.println(); Serial.flush();
                _module[i]->process(pkg);
            }
        }

        //Hooked functions
        for(i = 0; i<ASB_HOOKNUM; i++) {
            if(_hooks[i].execute != 0) {
                if(
                    (_hooks[i].type == 0xFF || _hooks[i].type == pkg.meta.type) &&
                    (_hooks[i].target == 0  || _hooks[i].target == pkg.meta.target) &&
                    (_hooks[i].port == -1   || _hooks[i].port == pkg.meta.port) &&
                    (_hooks[i].firstByte == 0xFF || (pkg.len > 0 && _hooks[i].firstByte == pkg.data[0]))
                ) {
                    _hooks[i].execute(pkg);
                }
            }
        }
    }

    bool ASB::hookAttach(byte type, unsigned int target, char port, byte firstByte, void (*function)(asbPacket&)) {
        for(byte i=0; i<ASB_HOOKNUM; i++) {
            if(_hooks[i].execute == 0) {
                _hooks[i].type = type;
                _hooks[i].target = target;
                _hooks[i].port = port;
                _hooks[i].firstByte = firstByte;
                _hooks[i].execute = function;
                return true;
            }
        }
        return false;
    }

    bool ASB::hookAttachModule(ASB_IO *module) {
        for(byte i=0; i<ASB_MODNUM; i++) {
            if(_module[i] == NULL) {
                module->_control = this;
                _module[i] = (ASB_IO *)module;

                byte id = module->_cfgId;

                //Read configuration
                if(_cfgAddrStart < _cfgAddrStop && id < 128) {
                    id <<= 4;
                    unsigned int address = _cfgAddrStart+2; //bytes 1+2 are our ID
                    byte check,len,num=0;

                    //Round 1 - count objects
                    do {
                        check = EEPROM.read(address);
                        len = ((1 << (check & 0x0F)) + 5);
                        if((check & 0xF0) == id) { //this is probably related to our module
                            num++;
                        }
                        address += len;
                    }while(address < _cfgAddrStop && check != 0xFF && check != 0x00);

                    if(
                        num > 0 && 
                        module->cfgReset() && 
                        module->cfgReserve(num)
                    ) {
                        //Round 2 - read objects
                        address = _cfgAddrStart+2;
                        do {
                            check = EEPROM.read(address);
                            len = ((1 << (check & 0x0F)) + 5);
                            if((check & 0xF0) == id) { //this is probably related to our module
                                module->cfgRead(address);
                            }
                            address += len;
                        }while(address < _cfgAddrStop && check != 0xFF && check != 0x00);
                        return true;
                    }else{
                        #ifdef ASB_DEBUG
                            Serial.print(F("ERR RES ")); 
                        #endif
                        return false;
                    }
                }
            }
        }
        #ifdef ASB_DEBUG
            Serial.print(F("No free module slot to attach")); Serial.println(); Serial.flush();
        #endif
        return false;
    }

    bool ASB::hookDetachModule(byte id) {
        for(byte i=0; i<ASB_MODNUM; i++) {
            if(_module[i] != NULL && _module[i]->_cfgId == id && _module[i]->cfgReset()) {
                _module[i] = NULL;
                return true;
            }
        }
        return false;
    }

    asbPacket ASB::loop(void) {
        byte i;
        asbPacket pkg;

        //Packet handling
        //This only receives a single packet. We could loop here, but doing it this way allows the user code to still somewhat execute in environments with a lot of messages…
        asbReceive(pkg);

        //Modules
        for(i=0; i<ASB_MODNUM; i++) {
            if(_module[i] != NULL) {
                _module[i]->loop();
            }
        }

        return pkg;
    }

#endif /* ASB__C */
