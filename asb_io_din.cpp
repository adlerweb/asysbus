/**
  aSysBus io module - digital inputs

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

#ifndef ASB_IO_DIN__C
    #define ASB_IO_DIN__C

    #include <Arduino.h>
    #include <inttypes.h>
    #include <asb.h>

    ASB_IO_DIN::ASB_IO_DIN(byte cfgId) {
        _cfgId=cfgId;
    }

    bool ASB_IO_DIN::cfgRead(unsigned int address) {
        if(_control == NULL) return false;
        byte temp;

        byte check = EEPROM.read(address);
        if(check == 0xFF || check == 0x00) {
            #ifdef ASB_DEBUG
                Serial.print(F("Header empty")); Serial.println(); Serial.flush();
            #endif
            return false;
        }
        if(((check & 0xF0) >> 4) != _cfgId) {
            #ifdef ASB_DEBUG
                Serial.print(F("Wrong ID")); Serial.println(); Serial.flush();
            #endif
            return false;
        }

        for(temp=0; temp<_items; temp++) {
            if(_config[temp].pin == 0xFF || _config[temp].pin == 0x00) { //Slot is free
                EEPROM.get(address+1, _config[temp]);

                if(_config[temp].pin == 0xFF || _config[temp].pin == 0x00) return false;

                if(_config[temp].pullup) {
                    ::pinMode(_config[temp].pin, INPUT_PULLUP);
                    delay(1);
                }else{
                    ::pinMode(_config[temp].pin, INPUT);
                }

                _config[temp].last = ::digitalRead(_config[temp].pin);
                if(_config[temp].invert) {
                    _config[temp].last ^= 1;
                }

                //Poll other nodes for last state
                byte data[1] = {ASB_CMD_REQ};
                _control->asbSend(ASB_PKGTYPE_MULTICAST, _config[temp].target, sizeof(data), data);
                
                return true;
            }
        }
        return false;
    }

    bool ASB_IO_DIN::cfgWrite(asbIoDIn &cfg) {
        if(_control == NULL) {
            #ifdef ASB_DEBUG
                Serial.println(F("No controller...")); Serial.flush();
            #endif
            return false;
        }

        unsigned int address;

        if(cfg.pin == 0xFF) {
            #ifdef ASB_DEBUG
                Serial.println(F("No pin...")); Serial.flush();
            #endif
            return false;
        }
        
        address = _control->cfgFindFreeblock(sizeof(cfg)+1, _cfgId);
        if(address == 0) {
            #ifdef ASB_DEBUG
                Serial.println(F("Got no address...")); Serial.flush();
            #endif
            return false;
        }
        address++; //Skip header

        EEPROM.put(address, cfg);
        
        return true;
    }

    bool ASB_IO_DIN::cfgReset(void) {
        if(_config == NULL) return true;
        free(_config);
        _config = NULL;
        _items = 0;
        return true;
    }

    bool ASB_IO_DIN::cfgReserve(byte objects) {
        cfgReset();
        _config = (asbIoDIn *) calloc(objects, sizeof(asbIoDIn));
        if(_config==NULL) {
            _items = 0;
            return false;
        }else{
            _items = objects;
            return true;
        }
    }

    bool ASB_IO_DIN::process(asbPacket &pkg) {
        if(_control == NULL) return false;
        byte i;
        if(pkg.len >= 1) {
            switch(pkg.data[0]) {
                case ASB_CMD_1B:
                    if(pkg.meta.type != ASB_PKGTYPE_MULTICAST || pkg.len != 2) break;
                    for(i=0; i<_items; i++) {
                        if(_config[i].pin != 0xFF && _config[i].pin != 0x00 && _config[i].target == pkg.meta.target) {
                            switch(_config[i].mode) {
                                case ASB_IO_DIN_DIRECT:
                                    _config[i].last = pkg.data[1];
                                break;
                                case ASB_IO_DIN_BTOGGLE:
                                case ASB_IO_DIN_STOGGLE:
                                    _config[i].last ^= (-(pkg.data[1] << 1) ^ _config[i].last) & (1 << 1);
                                break;
                            }
                        }
                    }
                break;

                case ASB_CMD_PER:
                    if(pkg.meta.type != ASB_PKGTYPE_MULTICAST || pkg.len != 2) break;
                    for(i=0; i<_items; i++) {
                        if(_config[i].pin != 0xFF && _config[i].pin != 0x00 && _config[i].target == pkg.meta.target) {
                            switch(_config[i].mode) {
                                case ASB_IO_DIN_DIRECT:
                                    _config[i].last = pkg.data[1];
                                break;
                                case ASB_IO_DIN_BTOGGLE:
                                case ASB_IO_DIN_STOGGLE:
                                    if(pkg.data[1] > 0) {
                                        _config[i].last |= 0x02;
                                    }else{
                                        _config[i].last &= ~(0x02);
                                    }
                                break;
                            }
                        }
                    }
                break;
            }
        }
        return true;
    }

    bool ASB_IO_DIN::loop(void) {
        if(_control == NULL) return false;
        byte temp;
        byte data[2] = {ASB_CMD_1B, 0};
        asbPacket pkg;
        bool t=false;

        for(byte i=0; i<_items; i++) {
            if(_config[i].pin != 0xFF && _config[i].pin != 0x00) {
                temp = ::digitalRead(_config[i].pin);

                if(_config[i].invert) {
                    temp ^= 1;
                }

                if((_config[i].last & 1) != temp) {
                    t=true;
                    switch(_config[i].mode) {
                        case ASB_IO_DIN_DIRECT:
                            data[1] = temp;
                            _control->asbSend(ASB_PKGTYPE_MULTICAST, _config[i].target, sizeof(data), data);
                            _config[i].last = temp;
                        break;
                        case ASB_IO_DIN_BTOGGLE:
                            //we use bit 1 of last as current bus state
                            //we use bit 2 of last as indicator for detecting button release
                            if((_config[i].last & 0x04) == 0 && temp) {
                                _config[i].last ^= 0x02;
                                data[1] = ((_config[i].last & 0x02) >> 1);
                                _control->asbSend(ASB_PKGTYPE_MULTICAST, _config[i].target, sizeof(data), data);
                                _config[i].last |= 0x04;
                            }else if((_config[i].last & 0x04) > 0 && !temp) {
                                _config[i].last &= ~0x04;
                            }
                            _config[i].last ^= (-temp ^ _config[i].last) & 1;
                        break;
                        case ASB_IO_DIN_STOGGLE:
                            //we use bit 1 of last as current bus state
                            _config[i].last ^= (1<<1);
                            data[1] = ((_config[i].last & 0x02) >> 1);
                            _control->asbSend(ASB_PKGTYPE_MULTICAST, _config[i].target, sizeof(data), data);
                            _config[i].last ^= (-temp ^ _config[i].last) & 1;
                        break;
                    }
                }
            }
        }
        
        if(t) delay(100); //TODO Replace this very ugly line fooling people who dont read code there would be proper debounce ;)
        return true;
    }

    bool ASB_IO_DIN::attach(unsigned int target, byte pin, byte mode, bool invert, bool pullup) {
        if(_control == NULL) return false;
        asbIoDIn cfg;

        cfg.target = target;
        cfg.pin = pin;
        cfg.invert = invert;
        cfg.pullup = pullup;
        cfg.mode = mode;
        if(!cfgWrite(cfg)) return false;
        _control->hookDetachModule(_cfgId);
        return _control->hookAttachModule(this);
    }

#endif /* ASB_IO_DIN__C */
