/*
  aSysBus CAN definitions

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

#ifndef ASB_CAN__H
#define ASB_CAN__H
    #include "asb.h"
    #include <mcp_can.h>
    #include <SPI.h>

    /**
     * Global variable indicating a CAN-bus got a message
     * This is currently shared amongst all CAN-interfaces!
     */
    extern volatile bool asb_CANIntReq;

    /**
     * Global interrupt function called on CAN interrupts
     * This is currently shared amongst all CAN-interfaces!
     */
    void asb_CANInt(void);

    /**
     * CAN Communication Interface
     * @see ASB_COMM
     */
    class ASB_CAN : public ASB_COMM {
        private:
            /**
             * CAN-Bus object
             * @see MCP_CAN
             * @see https://github.com/Seeed-Studio/CAN_BUS_Shield
             */
            MCP_CAN _interface;

            /**
             * Interrupt pin to be used
             */
            byte _intPin = 0;

            /**
             * CAN Bus Speed
             */
            byte _speed = 0;

            /**
             * CAN Crystal Frequency
             */
            byte _clockspd = 0;

        public:
            /**
             * Constructor for NonInterrupt operation
             * @TODO This is not implemented yet…
             * @param cs pin used for CHIP_SELECT
             * @param speed CAN bus speed definition from mcp_can_dfs.h (bottom)
             * @param clockspd MCP crystal frequency from mcp_can_dfs.h (bootom)
             * @see https://github.com/Seeed-Studio/CAN_BUS_Shield/blob/master/mcp_can_dfs.h
             */
            ASB_CAN(byte cs, byte speed, byte clockspd);

            /**
             * Constructor for Interrupt based operation
             * @param cs pin used for CHIP_SELECT
             * @param speed CAN bus speed definition from mcp_can_dfs.h (bottom)
             * @param clockspd MCP crystal frequency from mcp_can_dfs.h (bootom)
             * @param interrupt pin used for interrupt. Must have hardware INT support
             * @see https://github.com/Seeed-Studio/CAN_BUS_Shield/blob/master/mcp_can_dfs.h
             */
            ASB_CAN(byte cs, byte speed, byte clockspd, byte interrupt);

            /**
             * Initialize CAN controller
             * @return byte error code
             * @see https://github.com/Seeed-Studio/CAN_BUS_Shield/blob/master/mcp_can_dfs.h
             */
            byte begin(void);

            /**
             * Parse CAN-address into our metadata format
             * @param canAddr CAN-address
             * @return asbMeta object containing decoded metadata, targst/source==0x00 on errors
             */
            asbMeta asbCanAddrParse(unsigned long canAddr);

            /**
             * Assemble a CAN-address based on our adressing format
             * @param meta asbMeta object
             * @return unsigned long CAN-address
             */
            unsigned long asbCanAddrAssemble(asbMeta meta);
            /**
             * Assemble a CAN-address based on our adressing format
             * @param type 2 bit message type (ASB_PKGTYPE_*)
             * @param target address between 0x0001 and 0x07FF/0xFFFF
             * @param source source address between 0x0001 and 0x07FF
             * @return unsigned long CAN-address
             */
            unsigned long asbCanAddrAssemble(byte type, unsigned int target, unsigned int source);
            /**
             * Assemble a CAN-address based on our adressing format
             * @param type 2 bit message type (ASB_PKGTYPE_*)
             * @param target address between 0x0001 and 0x07FF/0xFFFF
             * @param source source address between 0x0001 and 0x07FF
             * @param port port address between 0x00 and 0x1F, Unicast only
             * @return unsigned long CAN-address
             */
            unsigned long asbCanAddrAssemble(byte type, unsigned int target, unsigned int source, char port);

            /**
             * Send message to CAN-bus
             * @param type 2 bit message type (ASB_PKGTYPE_*)
             * @param target address between 0x0001 and 0x07FF/0xFFFF
             * @param source source address between 0x0001 and 0x07FF
             * @param port port address between 0x00 and 0x1F, Unicast only
             * @param len number of bytes to send (0-8)
             * @param data array of bytes to send
             */
            bool asbSend(byte type, unsigned int target, unsigned int source, char port, byte len, const byte *data);

            /**
             * Receive a message from the CAN-bus
             *
             * This polls the CAN-Controller for new messages.
             * The received message will be passed to &pkg, if no message
             * is available the function will return false.
             *
             * @param pkg asbPacket-Reference to store received packet
             * @return true if a message was received
             */
            bool asbReceive(asbPacket &pkg);
    };

#endif /* ASB_CAN__H */
