/*
  aSysBus communication template

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

#ifndef ASB_COMM__H
#define ASB_COMM__H
    #include "asb_proto.h"
    #include "asb_comm.h"

    /**
     * Class defining the base functions for any communication interface
     * This is a template and can not be used by itself
     */
    class ASB_COMM {
        public:

            /**
             * Error code of the last action - 0 = no error
             */
            byte lastErr = 0;

            /**
             * Initialize Interface
             * @return error code, 0=OK
             */
            virtual byte begin(void) = 0;

            /**
             * Send message to the interface
             * @param type 2 bit message type (ASB_PKGTYPE_*)
             * @param target address between 0x0001 and 0x07FF/0xFFFF
             * @param source source address between 0x0001 and 0x07FF
             * @param port port address between 0x00 and 0x1F, Unicast only
             * @param len number of bytes to send (0-8)
             * @param data array of bytes to send
             */
            virtual bool asbSend(byte type, unsigned int target, unsigned int source, char port, byte len, const byte *data) = 0;

            /**
             * Receive a message from the interface
             *
             * The received message will be passed to &pkg, if no message
             * is available the function will return false.
             *
             * @param pkg asbPacket-Reference to store received packet
             * @return true if a message was received
             */
            virtual bool asbReceive(asbPacket &pkg)=0;
    };

#endif /* ASB_COMM__H */
