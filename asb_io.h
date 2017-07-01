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

#ifndef ASB_IO__H
#define ASB_IO__H

    #include <Arduino.h>
    #include <inttypes.h>
    #include <EEPROM.h>
    #include <asb.h>

    class ASB; //ASB has not been defined yet

    /**
     * IO Module
     */
    class ASB_IO {
        public:
            /**
             * Read configuration block starting at provided address
             */
            byte _cfgId=255;

            /**
             * Pointer to our Controller
             */
            ASB *_control;

            /**
             * Read configuration block starting at provided address
             * @param read configuration object from address X
             * @return bool true if successful
             */
            virtual bool cfgRead(unsigned int address)=0;

            /**
             * Write current configuration
             * @param module specific configuration struct
             * @return bool true if successful
             */
            //virtual bool cfgWrite()=0;

            /**
             * Reset current configuration and reserve memory
             * for x objects
             * @return bool true if successful
             */
            virtual bool cfgReset(void)=0;

            /**
             * Reserve memory for configuration
             * @param objects number of configuration objects
             * @return bool true if successful
             */
            virtual bool cfgReserve(byte objects)=0;

            /**
             * Process incoming packet
             * @param pkg Packet struct
             * @param ASB controller reference
             * @return bool true if successful
             */
            virtual bool process(asbPacket &pkg)=0;

            /**
             * Main loop call, e.G. for polling stuff
             * @param ASB controller reference
             * @return bool true if successful
             */
            virtual bool loop(void)=0;


    };

#endif /* ASB_IO__H */
