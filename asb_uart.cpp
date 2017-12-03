/**
  aSysBus UART interface

  @copyright 2015-2017 Florian Knodt, www.adlerweb.info

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

#ifndef ASB_UART__C
#define ASB_UART__C
    #include "asb_uart.h"
    #include "asb_proto.h"

    ASB_UART::ASB_UART(Stream &serial) {
        ASB_UART::_interface = &serial;
        ASB_UART::_buf[0] = 0;
    }

    byte ASB_UART::begin() {
        return 0;
    }

    bool ASB_UART::asbSend(byte type, unsigned int target, unsigned int source, char port, byte len, const byte *data) {
        byte tlen = 0;
        _interface->write(0x01);
        _interface->print(type,HEX);
        _interface->write(0x1F);
        _interface->print(target,HEX);
        _interface->write(0x1F);
        _interface->print(source,HEX);
        _interface->write(0x1F);
        if(port > 0) {
            _interface->print(port,HEX);
        }else{
            //Arduino print internally casts to double :(
            _interface->print(F("FF"));
        }
        _interface->write(0x1F);
        _interface->print(len,HEX);

        _interface->write(0x02);
        if(len > 0) {
            for(tlen = 0; tlen < len; tlen++) {
                _interface->print(data[tlen], HEX);
                _interface->write(0x1F);
            }
        }
        _interface->write(0x04);
        _interface->println();
        return true;
    }

    bool ASB_UART::bufShift(byte len) {
        if(_buf[0] == 0) return false;
        byte i;

        while (len > 0) {
            for(i = (_buf[0]-1); i > 0; i--) {
                _buf[i]     = _buf[(i+1)];
                _buf[(i+1)] = 0;
            }
            len--;
            _buf[0]--;
        }

        return true;
    }


    bool ASB_UART::bufShift() {
        if(_buf[0] == 0) return false;

        byte i;

        do {
            for(i = (_buf[0]-1); i > 0; i--) {
                _buf[i]     = _buf[(i+1)];
                _buf[(i+1)] = 0;
            }
            _buf[0]--;
        }while(_buf[1] != 0x01 && _buf[0] > 0);
        return true;
    }

    bool ASB_UART::asbReceive(asbPacket &pkg) {
        byte read;
        byte state;
        byte curwrite;
        bool retry = false;
        bool break2 = false;

        if(!_interface->available()) return false;

        while(_interface->available()) {
            read = _interface->read();

            if(read == 0x01) bufShift(_buf[0]);

            do {
                if(_buf[0] == 0) {  //No active RX, ignore everything until SOH
                    if(read == 0x01) {
                        _buf[0] = 2;
                        _buf[1] = read;
                    }
                }else{
                    _buf[_buf[0]] = read;

                    _buf[0]++;
                    if(_buf[(_buf[0]-1)] == 0x04 && _buf[0] >= 10) {
                        state = 0;
                        break2 = false;
                        for(read = 2; read <= _buf[0] && !break2; read++) {
                            switch(state) {
                                case 0:
                                    pkg.meta.type = asbHexToByte(_buf[read]);
                                    state++;
                                    break;
                                case 1:
                                    if(_buf[read] != 0x1F) {
                                        retry = bufShift();
                                        break2 = true;
                                        break;
                                    }else{
                                        state++;
                                    }
                                    break;
                                case 2:
                                    if(_buf[read] == 0x1F) {
                                        state++;
                                    }else{
                                        pkg.meta.target <<=4;
                                        pkg.meta.target |= asbHexToByte(_buf[read]);
                                    }
                                    break;
                                case 3:
                                    if(_buf[read] == 0x1F) {
                                        state++;
                                        pkg.meta.port = 0;
                                    }else{
                                        pkg.meta.source <<=4;
                                        pkg.meta.source |= asbHexToByte(_buf[read]);
                                    }
                                    break;
                                case 4:
                                    if(_buf[read] == 0x1F) {
                                        state++;
                                    }else{
                                        pkg.meta.port <<=4;
                                        pkg.meta.port |= asbHexToByte(_buf[read]);
                                    }
                                    break;
                                case 5:
                                    pkg.len = asbHexToByte(_buf[read]);
                                    state++;
                                    break;
                                case 6:
                                    if(_buf[read] != 0x02) {
                                        retry = bufShift();
                                        break2 = true;
                                        break;
                                    }else{
                                        curwrite=0;
                                        state++;
                                    }
                                    break;
                                case 7:
                                    if(_buf[read] == 0x1F) {
                                        curwrite++;
                                        pkg.data[curwrite] = 0;
                                    }else if(_buf[read] == 0x04) {
                                        state++;
                                    }else{
                                        pkg.data[curwrite] <<=4;
                                        pkg.data[curwrite] |= asbHexToByte(_buf[read]);
                                    }
                                    break;
                                case 8:
                                    bufShift(read);
                                    return true;
                            }
                        }
                    }else if(_buf[0] > 23) {
                        retry = bufShift();
                    }
                }
            }while(retry);
        }
        return false;
    }

    byte ASB_UART::asbHexToByte(byte hex) {
        if(hex >= '0' && hex <= '9') return hex-'0';
        if(hex >= 'a' && hex <= 'f') return hex-'a'+10;
        if(hex >= 'A' && hex <= 'F') return hex-'A'+10;
        return 0;
    }

#endif /* ASB_UART__C */
