/*
 * config.h
 *
 * Defines the components to be used in the GEVCU and allows the user to configure
 * static parameters.
 *
 * Note: Make sure with all pin defintions of your hardware that each pin number is
 *       only defined once.

 Copyright (c) 2013-2016 Collin Kidder, Michael Neuweiler, Charles Galpin

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
	boolean extended;
	boolean enabled;
};

enum FILEOUTPUTTYPE
{
	NONE = 0,
	BINARYFILE = 1,
	GVRET = 2,
	CRTD = 3
};

struct EEPROMSettings { //Must stay under 256 - currently somewhere around 222
	uint8_t version;
	
	uint32_t CAN0Speed;
	uint32_t CAN1Speed;
	boolean CAN0_Enabled;
	boolean CAN1_Enabled;
	FILTER CAN0Filters[8]; // filters for our 8 mailboxes - 10*8 = 80 bytes
	FILTER CAN1Filters[8]; // filters for our 8 mailboxes - 10*8 = 80 bytes

	boolean useBinarySerialComm; //use a binary protocol on the serial link or human readable format?
	FILEOUTPUTTYPE fileOutputType; //what format should we use for file output?

	char fileNameBase[30]; //Base filename to use
	char fileNameExt[4]; //extension to use
	uint16_t fileNum; //incrementing value to append to filename if we create a new file each time
	boolean appendFile; //start a new file every power up or append to current?
	boolean autoStartLogging; //should logging start immediately on start up?

	uint8_t logLevel; //Level of logging to output on serial line
	uint8_t sysType; //0 = CANDUE, 1 = GEVCU

	uint16_t valid; //stores a validity token to make sure EEPROM is not corrupt

	uint8_t singleWireMode; //anything other than 1 means normal mode. 1 means use single wire mode where we strobe the enable line to go into HV mode
	boolean CAN0ListenOnly; //if true we don't allow any messing with the bus but rather just passively monitor.
	boolean CAN1ListenOnly;
};

struct SystemSettings 
{
	uint8_t eepromWPPin;
	uint8_t CAN0EnablePin;
	uint8_t CAN1EnablePin;
	uint8_t SWCANMode0Pin;
	uint8_t SWCANMode1Pin; 
	boolean useSD; //should we attempt to use the SDCard? (No logging possible otherwise)
	boolean logToFile; //are we currently supposed to be logging to file?
	uint8_t SDCardSelPin;
	boolean SDCardInserted;
	uint8_t LED_CANTX;
	uint8_t LED_CANRX;
	uint8_t LED_LOGGING;
	boolean txToggle; //LED toggle values
	boolean rxToggle;
	boolean logToggle;
	boolean lawicelMode;
	boolean lawicelAutoPoll;
	boolean lawicelTimestamping;
	int lawicelPollCounter;
};

extern EEPROMSettings settings;
extern SystemSettings SysSettings;

//buffer size for SDCard - Sending canbus data to the card. Still allocated even for GEVCU but unused in that case
//This is a large buffer but the sketch may as well use up a lot of RAM. It's there.
//This value is picked up by the SD card library and not directly used in the GVRET code.
#define	BUF_SIZE	8192 

//size to use for buffering writes to the USB bulk endpoint
//This is, however, directly used.
#define SER_BUFF_SIZE		4096

//maximum number of microseconds between flushes to the USB port. 
//The host should be polling every 1ms or so and so this time should be a small multiple of that
#define SER_BUFF_FLUSH_INTERVAL	2000   

#define CFG_BUILD_NUM	332
#define CFG_VERSION "GVRET alpha 2016-03-28"
#define EEPROM_PAGE		275 //this is within an eeprom space currently unused on GEVCU so it's safe
#define EEPROM_VER		0x15

#define CANDUE_EEPROM_WP_PIN	18
#define CANDUE_CAN0_EN_PIN		50
#define CANDUE_CAN1_EN_PIN		48
#define CANDUE_USE_SD			1
#define CANDUE_SDCARD_SEL		10
#define CANDUE_SWCAN_MODE0		46
#define CANDUE_SWCAN_MODE1		44

#define GEVCU_EEPROM_WP_PIN		19
#define GEVCU_CAN0_EN_PIN		255  //GEVCU has a different transceiver with no enable pin
#define GEVCU_CAN1_EN_PIN		255
#define GEVCU_USE_SD			0
#define GEVCU_SDCARD_SEL		10
#define GEVCU_SWCAN_MODE0		255
#define GEVCU_SWCAN_MODE1		255

#define BLINK_LED          73 //13 is L, 73 is TX, 72 is RX

#define NUM_ANALOG	4
#define NUM_DIGITAL	4
#define NUM_OUTPUT	8

#endif /* CONFIG_H_ */

