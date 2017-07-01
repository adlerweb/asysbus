/**
  aSysBus hook definitions

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

#ifndef ASB_HOOK__H
#define ASB_HOOK__H

    #include <Arduino.h>
    #include <inttypes.h>

    /**
     * Hook struct
     * Contains address and function to call on RX
     */
    typedef struct {
      /**
       * Message type
       *  0x00 -> Broadcast
       *  0x01 -> Multicast
       *  0x02 -> Unicast
       *  0xFF -> Everything
       */
      byte type = 0x01;

      /**
       * Port
       * 0x00 - 0x1F
       * Only used in Unicast Mode
       * -1 -> Everything
       */
      char port = -1;

      /**
       * Target address
       * Unicast:             0x0001 - 0x07FF
       * Milticast/Broadcast: 0x0001 - 0xFFFF
       * 0x0 -> everything
       */
      unsigned int target = 0;

      /**
       * First message byte, usually ASB_CMD_*
       * 0xFF = everything
       */
      byte firstByte = 0xFF;

      /**
       * Function to call
       */
      void (*execute)(asbPacket &pkg);

    } asbHook;

#endif /* ASB_HOOK__H */

