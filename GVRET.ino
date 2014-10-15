/*
 GEV-RET.ino

 Created: 7/2/2014 10:10:14 PM
 Author: Collin Kidder

Copyright (c) 2014 Collin Kidder, Michael Neuweiler, Charles Galpin

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

 */

#include "GVRET.h"
#include "config.h"
#include <due_can.h>
#include <SDFat.h>
#include <SdFatUtil.h>
#include <due_wire.h>
#include <Wire_EEPROM.h>
#include "SerialConsole.h"

/*
Notes on project:
This code should be autonomous after being set up. That is, you should be able to set it up
then disconnect it and move over to a car or other device to monitor, plug it in, and have everything
use the settings you set up without any external input.
*/

byte i = 0;

bool binaryComm = true;

uint8_t buf[BUF_SIZE];
EEPROMSettings settings;
SystemSettings SysSettings;

// file system on sdcard
SdFat sd;
 
SdFile file; //allow to open a file

SerialConsole console;

//initializes all the system EEPROM values. Chances are this should be broken out a bit but
//there is only one checksum check for all of them so it's simple to do it all here.
void loadSettings()
{
	EEPROM.read(EEPROM_PAGE, settings);

	if (settings.version != EEPROM_VER) //if settings are not the current version then erase them and set defaults
	{
		Logger::console("Resetting to factory defaults");
		settings.version = EEPROM_VER;
		settings.appendFile = false;
		settings.CAN0Speed = 250000;
		settings.CAN0_Enabled = false;
		settings.CAN1Speed = 250000;
		settings.CAN1_Enabled = false;
		sprintf((char *)settings.fileNameBase, "CANBUS");
		sprintf((char *)settings.fileNameExt, "TXT");
		settings.fileNum = 1;
		for (int i = 0; i < 3; i++) 
		{
			settings.CAN0Filters[i].enabled = true;
			settings.CAN0Filters[i].extended = true;
			settings.CAN0Filters[i].id = 0;
			settings.CAN0Filters[i].mask = 0;
			settings.CAN1Filters[i].enabled = true;
			settings.CAN1Filters[i].extended = true;
			settings.CAN1Filters[i].id = 0;
			settings.CAN1Filters[i].mask = 0;
		}
		for (int j = 3; i < 8; i++)
		{
			settings.CAN0Filters[i].enabled = true;
			settings.CAN0Filters[i].extended = false;
			settings.CAN0Filters[i].id = 0;
			settings.CAN0Filters[i].mask = 0;
			settings.CAN1Filters[i].enabled = true;
			settings.CAN1Filters[i].extended = false;
			settings.CAN1Filters[i].id = 0;
			settings.CAN1Filters[i].mask = 0;
		}
		settings.useBinaryFile = false;
		settings.useBinarySerialComm = false;
		settings.logLevel = 1; //info
		settings.sysType = 0; //CANDUE as default
		settings.valid = 0; //not used right now
		EEPROM.write(EEPROM_PAGE, settings);
	}
	else {
		Logger::console("Using stored values from EEPROM");
	}

	Logger::setLoglevel((Logger::LogLevel)settings.logLevel);

	if (settings.sysType == 1) { //GEVCU
		Logger::console("Running on GEVCU hardware");
		SysSettings.eepromWPPin = GEVCU_EEPROM_WP_PIN;
		SysSettings.CAN0EnablePin = GEVCU_CAN0_EN_PIN;
		SysSettings.CAN1EnablePin = GEVCU_CAN1_EN_PIN;
		SysSettings.useSD = false;
		SysSettings.SDCardSelPin = GEVCU_SDCARD_SEL;
	}
	else //CANDUE
	{
		Logger::console("Running on CANDue hardware");
		SysSettings.eepromWPPin = CANDUE_EEPROM_WP_PIN;
		SysSettings.CAN0EnablePin = CANDUE_CAN0_EN_PIN;
		SysSettings.CAN1EnablePin = CANDUE_CAN1_EN_PIN;
		SysSettings.useSD = true;
		SysSettings.SDCardSelPin = CANDUE_SDCARD_SEL;
	}
}

void setup()
{
	delay(5000); //just for testing. Don't use in production
    pinMode(BLINK_LED, OUTPUT);
    digitalWrite(BLINK_LED, LOW);

	Wire.begin();
	EEPROM.setWPPin(18); // a guess...

	loadSettings();

	EEPROM.setWPPin(SysSettings.eepromWPPin);

    if (SysSettings.useSD) {	
		if (!sd.begin(SysSettings.SDCardSelPin, SPI_FULL_SPEED)) sd.initErrorHalt(); //init at 42MHz
	}

    SerialUSB.print("Build number: ");
    SerialUSB.println(CFG_BUILD_NUM);

    sys_early_setup();
	setup_sys_io();

	if (settings.CAN0_Enabled)
	{
		Can0.begin(settings.CAN0Speed, SysSettings.CAN0EnablePin);
	}
	if (settings.CAN1_Enabled)
	{
		Can1.begin(settings.CAN1Speed, SysSettings.CAN1EnablePin);
	}

	for (int i = 0; i < 7; i++) 
	{
		if (settings.CAN0Filters[i].enabled) 
		{
			Can0.setRXFilter(i, settings.CAN0Filters[i].id,
				settings.CAN0Filters[i].mask, settings.CAN0Filters[i].extended);
		}
		if (settings.CAN1Filters[i].enabled)
		{
			Can1.setRXFilter(i, settings.CAN1Filters[i].id,
				settings.CAN1Filters[i].mask, settings.CAN1Filters[i].extended);
		}
	}

	SerialUSB.print("Done with init\n");
}

void setPromiscuousMode() {
   //By default there are 7 mailboxes for each device that are RX boxes
  //This sets each mailbox to have an open filter that will accept extended
  //or standard frames
  int filter;
  //extended
  for (filter = 0; filter < 3; filter++) {
	Can0.setRXFilter(filter, 0, 0, true);
	Can1.setRXFilter(filter, 0, 0, true);
  }  
  //standard
  for (int filter = 3; filter < 7; filter++) {
	Can0.setRXFilter(filter, 0, 0, false);
	Can1.setRXFilter(filter, 0, 0, false);
  }  
}

//Get the value of XOR'ing all the bytes together. This creates a reasonable checksum that can be used
//to make sure nothing too stupid has happened on the comm.
uint8_t checksumCalc(uint8_t *buffer, int length) 
{
	uint8_t valu = 0;
	for (int c = 0; c < length; c++) {
		valu ^= buffer[c];
	}
	return valu;
}

void sendFrameToUSB(CAN_FRAME &frame, int whichBus) 
{
	uint8_t buff[18];
	uint8_t temp;
	if (settings.useBinarySerialComm) {
		if (frame.extended) frame.id |= 1 << 31;
		buff[0] = 0xF1;
		buff[1] = 0; //0 = canbus frame sending
		buff[2] = (uint8_t)(frame.id & 0xFF);
		buff[3] = (uint8_t)(frame.id >> 8);
		buff[4] = (uint8_t)(frame.id >> 16);
		buff[5] = (uint8_t)(frame.id >> 24);
		buff[6] = frame.length + (uint8_t)(whichBus << 4);
		for (int c = 0; c < frame.length; c++)
		{
			buff[7 + c] = frame.data.bytes[c];
		}
		temp = checksumCalc(buff, 7 + frame.length);
		buff[7 + frame.length] = temp;
		SerialUSB.write(buff, 8 + frame.length);
	}
	else 
	{
		SerialUSB.print(frame.id, HEX);
		if (frame.extended) SerialUSB.print(" X ");
		else SerialUSB.print(" S ");
		SerialUSB.print(whichBus);
		SerialUSB.print(" ");
		SerialUSB.print(frame.length);
		for (int c = 0; c < frame.length; c++)
		{
			SerialUSB.print(" ");
			SerialUSB.print(frame.data.bytes[c], HEX);
		}
		SerialUSB.println();
	}
}

/*
Loop executes as often as possible all the while interrupts fire in the background.
The serial comm protocol is as follows:
All commands start with 0xF1 this helps to synchronize if there were comm issues
Then the next byte specifies which command this is. 
Then the command data bytes which are specific to the command
Lastly, there is a checksum byte just to be sure there are no missed or duped bytes
Any bytes between checksum and 0xF1 are thrown away

Yes, this should probably have been done more neatly but this way is likely to be the
fastest and safest with limited function calls
*/
void loop()
{
	static int loops = 0;
	CAN_FRAME incoming;
	CAN_FRAME build_out_frame;
	int in_byte;
	static byte buff[20];
	static int step = 0;
	static STATE state = IDLE;
	static int build_int;
	uint8_t temp8;
	uint16_t temp16;
	static bool markToggle = false;

	//there is no switch debouncing here at the moment
	//if mark triggering causes bounce then debounce this later on.
	if (getDigital(0)) {
		if (!markToggle) {
			markToggle = true;
			if (!binaryComm) SerialUSB.println("MARK TRIGGERED");
			else 
			{ //figure out some sort of binary comm for the mark.
			}
		}
	}
	else markToggle = false;
	

  loops++;
  if (loops > 200000ul) {
	  loops = 0;
	  //SerialUSB.print(".");
  }

  if (Can0.available() > 0) {
	Can0.read(incoming);
	sendFrameToUSB(incoming, 0);
  }

  if (Can1.available()) {
	Can1.read(incoming); 
	sendFrameToUSB(incoming, 1);
  }

  if (SerialUSB.available()) {
	in_byte = SerialUSB.read();
	if (in_byte != -1) { //false alarm....
	   switch (state) {
	   case IDLE:
		   if (in_byte == 0xF1) state = GET_COMMAND;
		   else if (in_byte == 0xE7) settings.useBinarySerialComm = true;
		   else 
		   {
			   console.rcvCharacter((uint8_t)in_byte);
		   }
		   break;
	   case GET_COMMAND:
		   switch (in_byte) {
		   case 0:
			   state = BUILD_CAN_FRAME;
			   buff[0] = 0xF1;
			   step = 0;
			   break;
		   case 1:
			   state = TIME_SYNC;
			   step = 0;
			   break;
		   case 2:
			   //immediately return the data for digital inputs
			   temp8 = getDigital(0) + (getDigital(1) << 1) + (getDigital(2) << 2) + (getDigital(3) << 3);
			   buff[0] = 0xF1;
			   buff[1] = temp8;
			   temp8 = checksumCalc(buff, 2);
			   buff[2] = temp8;
			   SerialUSB.write(buff, 3);
			   state = IDLE;
			   break;
		   case 3:
			   //immediately return data on analog inputs
			   temp16 = getAnalog(0);
			   buff[0] = 0xF1;
			   buff[1] = temp16 & 0xFF;
			   buff[2] = uint8_t(temp16 >> 8);
			   temp16 = getAnalog(1);
			   buff[3] = temp16 & 0xFF;
			   buff[4] = uint8_t(temp16 >> 8);
			   temp16 = getAnalog(2);
			   buff[5] = temp16 & 0xFF;
			   buff[6] = uint8_t(temp16 >> 8);
			   temp16 = getAnalog(3);
			   buff[7] = temp16 & 0xFF;
			   buff[8] = uint8_t(temp16 >> 8);
			   temp8 = checksumCalc(buff, 9);
			   buff[9] = temp8;
			   SerialUSB.write(buff, 10);
			   state = IDLE;
			   break;
		   case 4:
			   state = SET_DIG_OUTPUTS;
			   buff[0] = 0xF1;
			   break;
		   case 5:
			   state = SETUP_CANBUS;
			   step = 0;
			   buff[0] = 0xF1;
			   break;
		   }
		   break;
	   case BUILD_CAN_FRAME:
		   buff[1 + step] = in_byte;
		   switch (step) {
		   case 0:
			   build_out_frame.id = in_byte;
			   break;
		   case 1:
			   build_out_frame.id |= in_byte << 8;
			   break;
		   case 2:
			   build_out_frame.id |= in_byte << 16;
			   break;
		   case 3:
			   build_out_frame.id |= in_byte << 24;
			   if (build_out_frame.id & 1 << 31) 
			   {
				   build_out_frame.id &= 0x7FFFFFFF;
				   build_out_frame.extended = true;
			   }
			   else build_out_frame.extended = false;
			   break;
		   case 4:
			   build_out_frame.length = in_byte & 0xF;
			   if (build_out_frame.length > 8) build_out_frame.length = 8;
			   break;
		   default:
			   if (step < build_out_frame.length + 5)
			   {
			      build_out_frame.data.bytes[step - 5] = in_byte;
			   }
			   else 
			   {
				   state = IDLE;
				   //this would be the checksum byte. Compute and compare.
				   temp8 = checksumCalc(buff, step);
				   if (temp8 == in_byte) 
				   {
						Can0.sendFrame(build_out_frame);
				   }
			   }
			   break;
		   }
		   step++;
		   break;
	   case TIME_SYNC:
		   break;
	   case SET_DIG_OUTPUTS: //todo: validate the XOR byte
		   buff[1] = in_byte;
		   //temp8 = checksumCalc(buff, 2);
		   for (int c = 0; c < 8; c++) 
		   {
			   if (in_byte & (1 << c)) setOutput(c, true);
			   else setOutput(c, false);
		   }
		   state = IDLE;
		   break;
	   case SETUP_CANBUS: //todo: validate checksum
		   switch (step)
		   {
		   case 0:
			   build_int = in_byte;
			   break;
		   case 1:
			   build_int |= in_byte << 8;
			   break;
		   case 2:
			   build_int |= in_byte << 16;
			   break;
		   case 3:
			   build_int |= in_byte << 24;
			   if (build_int > 0) 
			   {
				   if (build_int > 1000000) build_int = 1000000;
				   Can0.begin(build_int, SysSettings.CAN0EnablePin);
			   }
			   else //disable first canbus
			   {
				   Can0.disable();
			   }
			   break;
		   case 4:
			   build_int = in_byte;
			   break;
		   case 5:
			   build_int |= in_byte << 8;
			   break;
		   case 6:
			   build_int |= in_byte << 16;
			   break;
		   case 7:
			   build_int |= in_byte << 24;
			   if (build_int > 0) 
			   {
				   if (build_int > 1000000) build_int = 1000000;
				   Can1.begin(build_int, SysSettings.CAN1EnablePin);
			   }
			   else //disable first canbus
			   {
				   Can1.disable();
			   }
			   state = IDLE;
			   break;
		   }
		   setPromiscuousMode();
		   step++;
		   break;
	   }
	}
  }
   //this should still be here. It checks for a flag set during an interrupt
   sys_io_adc_poll();
}
