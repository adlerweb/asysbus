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

#ifndef ASB__H
    #define ASB__H
    #include "asb_comm.h"
    #include "asb_proto.h"
    #include "asb_hook.h"

    #include "asb_comm.h"
    #include "asb_can.h"
    #include "asb_uart.h"

    #include "asb_io.h"
    #include "asb_io_din.h"
    #include "asb_io_dout.h"

    /**
     * Maximum number of parallel communication interfaces
     *
     * ASB_BUSNUM sets the maximum number of active communication interfaces
     * in one instance. You can set it to a integer between 1 and 120. Keep
     * in mind higher numbers will increase RAM and CPU usage. Default is 6.
     */
    #ifndef ASB_BUSNUM
        #define ASB_BUSNUM 6 //<120!
    #endif

    /**
     * Activate debugging on Serial
     *
     */
    #ifndef ASB_DEBUG
        //#define ASB_DEBUG
    #endif

    /**
     * Maximum number of active hooks
     *
     * ASB_HOOKNUM sets the maximum number of active hooks in one instance.
     * You can set it to a integer between 1 and 120. Keep
     * in mind higher numbers will increase RAM and CPU usage. Default is 16.
     */
    #ifndef ASB_HOOKNUM
        #define ASB_HOOKNUM 16 //<120!
    #endif

    /**
     * Maximum number of active modules
     *
     * ASB_MODNUM sets the maximum number of active modules in one instance.
     * You can set it to a integer between 1 and 120. Keep in mind higher
     * numbers will increase RAM and CPU usage. Default is 16.
     */
    #ifndef ASB_MODNUM
        #define ASB_MODNUM 16 //<120!
    #endif

    /**
     * ASB main controller class
     *
     * This class stores all node parameters, handles packet routing between
     * different communication interfaces, implements basic functions like
     * sending boot messages, answering to PING and allows for attaching your
     * own functions to various messages
     */
    class ASB {
        private:
            /**
             * Array of pointers to active communication interfaces
             */
            ASB_COMM *_busAddr[ASB_BUSNUM];

            /**
             * Array of structs containing hooks
             */
            asbHook _hooks[ASB_HOOKNUM];

            /**
             * Array of pointers to active modules
             */
            ASB_IO *_module[ASB_MODNUM];

            /**
             * Bus address of this node
             */
            unsigned int _nodeId=0;

            /**
             * First EEPROM address to use
             */
            unsigned int _cfgAddrStart=0;

            /**
             * Last EEPROM address to use
             */
            unsigned int _cfgAddrStop=511;


        public:
            /**
             * Constructor reads configuration
             * @param EEPROM start address, usually 0
             * @param EEPROM stop address, usually 511
             */
            ASB(unsigned int start, unsigned int stop);
            
            /**
             * "Light" Constructor without EEPROM-read
             * @param Node-ID between 0x0001 and 0x07FF
             */
            ASB(unsigned int id);

            /**
             * Initialize node if no configuration found in EEPROM
             * @param function to execute
             * @return true if initialized
             */
            bool firstboot(void (*function)());

            /**
             * Change Node-ID
             * @param id Node-ID between 0x0001 and 0x07FF
             */
            bool setNodeId(unsigned int id);

            /**
             * Attach a bus-object to this controller
             * @param bus Bus object, derived from ASB_COMM
             * @return bus-ID, -1 on errors
             * @see busDetach()
             */
            char busAttach(ASB_COMM* bus);

            /**
             * Detach a bus-object from this controller
             * @param busId ID of the bus-object as given by busAttach
             * @return true if successful
             * @see busAttach()
             */
            bool busDetach(signed char busId);

            /**
             * First EEPROM address to use
             */
            unsigned int cfsAddrStart=0;

            /**
             * Last EEPROM address to use
             */
            unsigned int cfsAddrStop=511;


            /**
             * Find next free config block with >=size bytes
             * @param lenth of requested block in bytes
             * @param id configuration ID of requesting module
             * @return unsigned int address, 0 for error
             */
            unsigned int cfgFindFreeblock(byte bytes, byte id);

            /**
             * Send a message to the bus
             * @param meta asbMeta object containing message metadata
             * @param len number of data bytes to send
             * @param data array of data bytes to send
             * @return number of errors
             */
            byte asbSend(asbMeta meta, byte len, byte *data);
            /**
             * Send a message to the bus
             * @param type 2 bit message type (ASB_PKGTYPE_*)
             * @param target target address between 0x0001 and 0x07FF/0xFFFF
             * @param len number of data bytes to send
             * @param data array of data bytes to send
             * @return number of errors
             */
            byte asbSend(byte type, unsigned int target, byte len, byte *data);
            /**
             * Send a message to the bus
             * @param type 2 bit message type (ASB_PKGTYPE_*)
             * @param target target address between 0x0001 and 0x07FF/0xFFFF
             * @param port port address between 0x00 and 0x1F, Unicast only
             * @param len number of data bytes to send
             * @param data array of data bytes to send
             * @return number of errors
             */
            byte asbSend(byte type, unsigned int target, char port, byte len, byte *data);
            /**
             * Send a message to the bus
             * @param type 2 bit message type (ASB_PKGTYPE_*)
             * @param target target address between 0x0001 and 0x07FF/0xFFFF
             * @param source source address between 0x0001 and 0x07FF, 0=self
             * @param port port address between 0x00 and 0x1F, Unicast only
             * @param len number of data bytes to send
             * @param data array of data bytes to send
             * @param skip busID as per busAttach to skip when sending
             * @return number of errors
             * @see busAttach()
             */
            byte asbSend(byte type, unsigned int target, unsigned int source, char port, byte len, byte *data, signed char skip);

            /**
             * Receive a message from the bus
             *
             * This polls all attached communication interfaces for new messages
             * The first received message will be passed to &pkg, if no messages
             * are available the function will return false. If a message is
             * received you are adviced to call the function again until no
             * more messages are availabe on any interface. All received packets
             * will be redistributed to all other attached interfaces
             *
             * @param pkg asbPacket-Reference to store received packet
             * @return true if a message was received
             */
            bool asbReceive(asbPacket &pkg);

            /**
             * Receive a message from the bus
             *
             * Same as above but doesn't return anything. Useful if you just use hooks
             *
             * @see asbReceive(asbPacket &pkg)
             */
            void asbReceive(void);

            /**
             * Receive a message from the bus
             *
             * This polls all attached communication interfaces for new messages
             * The first received message will be passed to &pkg, if no messages
             * are available the function will return false. If a message is
             * received you are adviced to call the function again until no
             * more messages are availabe on any interface. All received packets
             * will be redistributed to all other attached interfaces if routing
             * is true
             *
             * @param pkg asbPacket-Reference to store received packet
             * @param routing If false packet will not be redistributed
             * @return true if a message was received
             */
            bool asbReceive(asbPacket &pkg, bool routing);

            /**
             * Process incoming packet
             * @param pkg Packet struct
             */
            void asbProcess(asbPacket &pkg);

            /**
             * Attach a hook to a set of metadata
             *
             * If a received packet matches the type, target and port the supplied function
             * will be called. This allows for custom actors. Note: There is no detach (yet?).
             *
             * @param type 2 bit message type (ASB_PKGTYPE_*), 0xFF = everything
             * @param target target address between 0x0001 and 0x07FF/0xFFFF, 0x0 = everything
             * @param port port address between 0x00 and 0x1F, Unicast only, -1 = everything
             * @param First data byte (usually ASB_CMD_*), 0xFF = everything
             * @param function to call when matched
             * @return true if successfully added
             */
            bool hookAttach(byte type, unsigned int target, char port, byte firstByte, void (*function)(asbPacket&));

            /**
             * Attach a module to this controller
             *
             * Attached modules will get initialized and later called for loop and received Packets
             *
             * @param ASB_IO based module
             * @return true if successfully added
             */
            bool hookAttachModule(ASB_IO *module);

            /**
             * Detach a module from this controller
             *
             * @param ASB_IO based module-ID
             * @return true if successfully added
             */
            bool hookDetachModule(byte id);

            /**
             * Main processing loop
             *
             * Receives and routes packets, checks inputs, etc
             *
             * @return asbPacket last received packet
             */
            asbPacket loop(void);
    };

#endif /* ASB__H */
