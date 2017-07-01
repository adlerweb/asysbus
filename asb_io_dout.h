/**
  aSysBus io module - digital outputs

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

#ifndef ASB_IO_DOUT__H
#define ASB_IO_DOUT__H

    #define ASB_IO_DOUT_DIO   0 //Digital IO
    #define ASB_IO_DOUT_PULSE 1 //Pulse for 1 second
    #define ASB_IO_DOUT_LIN   2 //Linear PWM
    #define ASB_IO_DOUT_LED   3 //LED PWM
    #define ASB_IO_DOUT_SER   4 //Servo PWM

    #include <Arduino.h>
    #include <inttypes.h>
    #include <asb.h>

    /**
     * Direct output struct
     * Contains pin numbers and configuration for outputs
     */
    typedef struct {

      /**
       * Affected pin
       */
      byte pin = 0xFF;

      /**
       * Target address
       * Unicast:             0x0001 - 0x07FF
       * Milticast/Broadcast: 0x0001 - 0xFFFF
       * 0x0 -> everything
       */
      unsigned int target = 0;

      /**
       * Is pin inverted?
       */
      boolean invert = false;

      /**
       * Initial pin state
       */
      boolean init = false;

      /**
       * Output mode
       */
      byte mode = ASB_IO_DIN_DIRECT;

      /**
       * Last received state
       */
      byte last = LOW;

  } asbIoDOut;

    /**
     * Digital output module
     * @see ASB_IO
     */
    class ASB_IO_DOUT : public ASB_IO {
        private:
            /**
             * Number of configured inputs
             */
             byte _items;

            /**
             * Array of configured inputs
             */
             asbIoDOut *_config;

             /**
              * LED dimming multiplier for 8 bit
              */
             const float _ledFactor = (100 * log10(2))/(log10(255));

        public:
            /**
             * Initialize
             * @param read configuration objects using id X, 1-15
             * @return bool true if successful
             */
            ASB_IO_DOUT(byte cfgId);

            /**
             * Read configuration block starting at provided address
             * @param read configuration object from address X
             * @return bool true if successful
             */
            bool cfgRead(unsigned int address);

            /**
             * Write configuration to EEPROM
             * @param asbIoDOut configuration struct reference
             * @return bool true if successful
             */
            bool cfgWrite(asbIoDOut &cfg);

            /**
             * Reset current configuration
             * @return bool true if successful
             */
            bool cfgReset(void);

            /**
             * Reserve memory for configuration
             * @param objects number of configuration objects
             * @return bool true if successful
             */
            bool cfgReserve(byte objects);

            /**
             * Process incoming packet
             * @param pkg Packet struct
             * @return bool true if successful
             */
            bool process(asbPacket &pkg);

            /**
             * Main loop call, e.G. for polling stuff
             * @return bool true if successful
             */
            bool loop(void);

            /**
             * Attach an input to a set of metadata
             *
             * If the supplied pins state changes an IO-message will be send
             *
             * @param target target address between 0x0001 and 0xFFFF
             * @param pin Pin number to monitor
             * @param mode ASB_IN_DIRECT = Direct output, _TOGGLE = Toggle output when HIGH
             * @param invert input
             * @param pullup use internal pull-up resistor
             * @return true if successfully added
             */
            bool attach(unsigned int target, byte pin, byte mode, bool invert, bool init);
    };

#endif /* ASB_IO_DOUT__H */
