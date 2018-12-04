/**
  aSysBus protocol definitions

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

#ifndef ASB_PROTO__H
#define ASB_PROTO__H

    #include <Arduino.h>
    #include <inttypes.h>

    #define ASB_PKGTYPE_BROADCAST 0x00
    #define ASB_PKGTYPE_MULTICAST 0x01
    #define ASB_PKGTYPE_UNICAST   0x02

    #define ASB_CMD_LEGACY_8B     0x02
    #define ASB_CMD_BOOT          0x21
    #define ASB_CMD_REQ           0x40
    #define ASB_CMD_0B            0x50 //0-Bit, generic pulse
    #define ASB_CMD_1B            0x51 //1-Bit, on/off
    #define ASB_CMD_PER           0x52 //%
    #define ASB_CMD_PING          0x70
    #define ASB_CMD_PONG          0x71
    #define ASB_CMD_CFG_READ      0x80 //2-byte address
    #define ASB_CMD_CFG_WRITE     0x81 //2-byte-address + data
    #define ASB_CMD_CFG_COMMIT    0x82 //2-byte-address
    #define ASB_CMD_IDENT         0x85 //Change local address, 2-byte-address
    #define ASB_CMD_S_TEMP        0xA0 //x*0.1°C, int
    #define ASB_CMD_S_HUM         0xA1 //x*0.1%RH, unsigned int
    #define ASB_CMD_S_PRS         0xA2 //x*0.1hPa, unsigned int
    #define ASB_CMD_S_LUX         0xA5 //unsigned long
    #define ASB_CMD_S_UV          0xA6 //*0.1, unsigned int
    #define ASB_CMD_S_IR          0xA7 //unsigned long
    #define ASB_CMD_S_PM25        0xB0 //TBD
    #define ASB_CMD_S_PM10        0xB1 //TBD
    #define ASB_CMD_S_VOLT        0xC0 //x*0.01V, int
    #define ASB_CMD_S_AMP         0xC1 //x*0.01A, int
    #define ASB_CMD_S_PWR         0xC2 //x*0.1W or VA, long
    #define ASB_CMD_S_PER         0xD0 //%, byte
    #define ASB_CMD_S_PML         0xD1 //‰, unsingned int
    #define ASB_CMD_S_PPM         0xD2 //Parts per million, unsingned int
    #define ASB_CMD_S_PY          0xD5 //smth per Year, unsinged int
    #define ASB_CMD_S_PMo         0xD6 //smth per Month, unsinged int
    #define ASB_CMD_S_PD          0xD7 //smth per Day, unsinged int
    #define ASB_CMD_S_PH          0xD8 //smth per Hour, unsinged int
    #define ASB_CMD_S_PM          0xD9 //smth per Minute, unsinged int (RPM, Pulse PM, etc)
    #define ASB_CMD_S_PS          0xDA //smth per Second, unsinged int

    /**
     * Packet metadata
     * Contains all data except things related to the actual payload
     */
    typedef struct {
      /**
       * Message type
       *  0 -> Broadcast
       *  1 -> Multicast
       *  2 -> Unicast
       */
      byte type = 0x00;

      /**
       * Port
       * 0x00 - 0x1F
       * Only used in Unicast Mode, otherwise -1
       */
      char port = -1;

      /**
       * Target address
       * Unicast:             0x0001 - 0x07FF
       * Milticast/Broadcast: 0x0001 - 0xFFFF
       * 0x0000 = invalid packet
       */
      unsigned int target = 0x0000;

      /**
       * Source address
       * 0x0001 - 0x07FF
       * 0x0000 = invalid packet
       */
      unsigned int source = 0x0000;

      /**
       * Interface the message originated from
       * @see ASB::busAttach
       */
      signed char busId = -1;
    } asbMeta;

    /**
     * Complete Packet
     * Contains metadata, a length indicator and the payload itself
     * @see asbMeta
     */
    typedef struct {
        /**
         * @see asbMeta
         */
        asbMeta meta;
        /**
         * length in bytes, 0-8. -1 indicates invalid packet
         */
        char len = -1;
        /**
         * payload
         */
        byte data[8];
    } asbPacket;

#endif /* ASB_PROTO__H */

