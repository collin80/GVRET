/*
 * config.h
 *
 * Defines the components to be used in the GEVCU and allows the user to configure
 * static parameters.
 *
 * Note: Make sure with all pin defintions of your hardware that each pin number is
 *       only defined once.

 Copyright (c) 2013 Collin Kidder, Michael Neuweiler, Charles Galpin

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *      Author: Michael Neuweiler
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "due_can.h"

struct FILTER {  //should be 10 bytes
	uint32_t id;
	uint32_t mask;
	uint8_t extended;
	uint8_t enabled;
};

struct EEPROMSettings { //222 bytes right now. Must stay under 256
	uint8_t version;
	
	uint32_t CAN0Speed;
	uint32_t CAN1Speed;
	uint8_t CAN0_Enabled;
	uint8_t CAN1_Enabled;
	FILTER CAN0Filters[8]; // filters for our 8 mailboxes - 10*8 = 80 bytes
	FILTER CAN1Filters[8]; // filters for our 8 mailboxes - 10*8 = 80 bytes

	uint8_t useBinarySerialComm; //use a binary protocol on the serial link or human readable format?
	uint8_t useBinaryFile; //store data in file in binary or text? Binary is more compact and thus faster/more efficient

	char fileNameBase[30]; //Base filename to use
	char fileNameExt[4]; //extension to use
	uint16_t fileNum; //incrementing value to append to filename if we create a new file each time
	uint8_t appendFile; //start a new file every power up or append to current?

	uint8_t logLevel; //Level of logging to output on serial line
	uint8_t sysType; //0 = CANDUE, 1 = GEVCU

	uint16_t valid; //stores a validity token to make sure EEPROM is not corrupt
};

struct SystemSettings 
{
	uint8_t eepromWPPin;
	uint8_t CAN0EnablePin;
	uint8_t CAN1EnablePin;
	boolean useSD;
	uint8_t SDCardSelPin;
};

extern EEPROMSettings settings;
extern SystemSettings SysSettings;

#define	BUF_SIZE	8192 //buffer size for SDCard - Sending canbus data to the card. Still allocated even for GEVCU but unused in that case

#define CFG_BUILD_NUM	301
#define CFG_VERSION "GVRET alpha 2014-10-14"
#define EEPROM_PAGE		275 //this is within an eeprom space currently unused on GEVCU so it's safe
#define EEPROM_VER		0x14

#define CANDUE_EEPROM_WP_PIN	18
#define CANDUE_CAN0_EN_PIN		50
#define CANDUE_CAN1_EN_PIN		48
#define CANDUE_USE_SD			1
#define CANDUE_SDCARD_SEL		10

#define GEVCU_EEPROM_WP_PIN		19
#define GEVCU_CAN0_EN_PIN		255  //GEVCU has a different transceiver with no enable pin
#define GEVCU_CAN1_EN_PIN		255
#define GEVCU_USE_SD			0
#define GEVCU_SDCARD_SEL		10

/*
 * SERIAL CONFIGURATION
 */
#define CFG_SERIAL_SPEED 115200
//#define SerialUSB Serial // re-route serial-usb output to programming port ;) comment if output should go to std usb

#define BLINK_LED          73 //13 is L, 73 is TX, 72 is RX

#define NUM_ANALOG	4
#define NUM_DIGITAL	4
#define NUM_OUTPUT	8

#endif /* CONFIG_H_ */
