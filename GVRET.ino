/*
 GEV-RET.ino

 Created: 7/2/2014 10:10:14 PM
 Author: Collin Kidder

Copyright (c) 2014-2015 Collin Kidder, Michael Neuweiler, Charles Galpin

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
#include <SdFat.h>
#include <SdFatUtil.h>
#include <due_wire.h>
#include <Wire_EEPROM.h>
#include <DueFlashStorage.h>
#include "SerialConsole.h"

/*
Notes on project:
This code should be autonomous after being set up. That is, you should be able to set it up
then disconnect it and move over to a car or other device to monitor, plug it in, and have everything
use the settings you set up without any external input.
*/

byte i = 0;

byte serialBuffer[SER_BUFF_SIZE];
int serialBufferLength = 0; //not creating a ring buffer. The buffer should be large enough to never overflow
uint32_t lastFlushMicros = 0;

EEPROMSettings settings;
SystemSettings SysSettings;

// file system on sdcard
SdFat sd;

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
		settings.CAN0Speed = 500000;
		settings.CAN0_Enabled = true;
		settings.CAN1Speed = 500000;
		settings.CAN1_Enabled = true;
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
		for (int j = 3; j < 8; j++)
		{
			settings.CAN0Filters[j].enabled = true;
			settings.CAN0Filters[j].extended = false;
			settings.CAN0Filters[j].id = 0;
			settings.CAN0Filters[j].mask = 0;
			settings.CAN1Filters[j].enabled = true;
			settings.CAN1Filters[j].extended = false;
			settings.CAN1Filters[j].id = 0;
			settings.CAN1Filters[j].mask = 0;
		}
		settings.fileOutputType = CRTD;
		settings.useBinarySerialComm = false;
		settings.autoStartLogging = false;
		settings.logLevel = 1; //info
		settings.sysType = 0; //CANDUE as default
		settings.valid = 0; //not used right now
		settings.singleWireMode = 0; //normal mode
		settings.CAN0ListenOnly = false;
		settings.CAN1ListenOnly = false;
		EEPROM.write(EEPROM_PAGE, settings);
	}
	else {
		Logger::console("Using stored values from EEPROM");
	}

	Logger::setLoglevel((Logger::LogLevel)settings.logLevel);

	SysSettings.SDCardInserted = false;

	switch (settings.sysType) {
		case 1:  //GEVCU
			Logger::console("Running on GEVCU hardware");
			SysSettings.eepromWPPin = GEVCU_EEPROM_WP_PIN;
			SysSettings.CAN0EnablePin = GEVCU_CAN0_EN_PIN;
			SysSettings.CAN1EnablePin = GEVCU_CAN1_EN_PIN;
			SysSettings.SWCANMode0Pin = GEVCU_SWCAN_MODE0;
			SysSettings.SWCANMode1Pin = GEVCU_SWCAN_MODE1;
			SysSettings.useSD = false;
			SysSettings.SDCardSelPin = GEVCU_SDCARD_SEL;
			SysSettings.LED_CANTX = 13; //We do have an LED at pin 13. Use it for both
			SysSettings.LED_CANRX = 13; //RX and TX.
			SysSettings.LED_LOGGING = 255; //we just don't have an LED to use for this.
			pinMode(13, OUTPUT);
			digitalWrite(13, LOW);
			break;
		case 2: //CANDUE13
			Logger::console("Running on CANDue13 hardware");
			SysSettings.eepromWPPin = CANDUE_EEPROM_WP_PIN;
			SysSettings.CAN0EnablePin = CANDUE_CAN0_EN_PIN;
			SysSettings.CAN1EnablePin = CANDUE_CAN1_EN_PIN;
			SysSettings.SWCANMode0Pin = CANDUE_SWCAN_MODE0;
			SysSettings.SWCANMode1Pin = CANDUE_SWCAN_MODE1;
			SysSettings.useSD = true;
			SysSettings.SDCardSelPin = CANDUE_SDCARD_SEL;
			SysSettings.LED_CANTX = 13; //The Arduino Due has three LEDs 
			SysSettings.LED_CANRX = 13; //so we can use them all
			SysSettings.LED_LOGGING = 13; //The above two are active low. This is active high.
			SysSettings.logToggle = false;
			SysSettings.txToggle = true;
			SysSettings.rxToggle = true;
			pinMode(13, OUTPUT); //just to be sure it's an output
			digitalWrite(13, LOW);
			break;
		default: //CANDUE
			Logger::console("Running on CANDue hardware");
			SysSettings.eepromWPPin = CANDUE_EEPROM_WP_PIN;
			SysSettings.CAN0EnablePin = CANDUE_CAN0_EN_PIN;
			SysSettings.CAN1EnablePin = CANDUE_CAN1_EN_PIN;
			SysSettings.SWCANMode0Pin = CANDUE_SWCAN_MODE0;
			SysSettings.SWCANMode1Pin = CANDUE_SWCAN_MODE1;
			SysSettings.useSD = true;
			SysSettings.SDCardSelPin = CANDUE_SDCARD_SEL;
			SysSettings.LED_CANTX = 73; //The Arduino Due has three LEDs 
			SysSettings.LED_CANRX = 72; //so we can use them all
			SysSettings.LED_LOGGING = 13; //The above two are active low. This is active high.
			SysSettings.logToggle = false;
			SysSettings.txToggle = true;
			SysSettings.rxToggle = true;
			pinMode(13, OUTPUT); //just to be sure they're outputs
			pinMode(73, OUTPUT);
			pinMode(72, OUTPUT);
			//And set all lights to be off.
			digitalWrite(13, LOW);
			digitalWrite(72, HIGH);
			digitalWrite(73, HIGH);
			break;
	}
	if (SysSettings.SWCANMode0Pin != 255) pinMode(SysSettings.SWCANMode0Pin, OUTPUT);
	if (SysSettings.SWCANMode1Pin != 255) pinMode(SysSettings.SWCANMode1Pin, OUTPUT);
	
	if (SysSettings.CAN0EnablePin != 255) pinMode(SysSettings.CAN0EnablePin, OUTPUT);
	if (SysSettings.CAN1EnablePin != 255) pinMode(SysSettings.CAN1EnablePin, OUTPUT);
		
	if (settings.singleWireMode && settings.CAN1_Enabled) setSWCANEnabled();
	else setSWCANSleep(); //start out setting single wire to sleep. 
}

void setSWCANSleep()
{
	if (SysSettings.SWCANMode0Pin != 255) digitalWrite(SysSettings.SWCANMode0Pin, LOW);
	if (SysSettings.SWCANMode1Pin != 255) digitalWrite(SysSettings.SWCANMode1Pin, LOW);
	if (settings.CAN1_Enabled && SysSettings.CAN1EnablePin != 255) digitalWrite(SysSettings.CAN1EnablePin, HIGH);
}

void setSWCANEnabled()
{
	if (SysSettings.SWCANMode0Pin != 255) digitalWrite(SysSettings.SWCANMode0Pin, HIGH);
	if (SysSettings.SWCANMode1Pin != 255) digitalWrite(SysSettings.SWCANMode1Pin, HIGH);
	if (settings.CAN1_Enabled && SysSettings.CAN1EnablePin != 255) digitalWrite(SysSettings.CAN1EnablePin, LOW);
}

void setSWCANWakeup()
{
	if (SysSettings.SWCANMode0Pin != 255) digitalWrite(SysSettings.SWCANMode0Pin, LOW);
	if (SysSettings.SWCANMode1Pin != 255) digitalWrite(SysSettings.SWCANMode1Pin, HIGH);
}

void setup()
{
	//delay(5000); //just for testing. Don't use in production
    pinMode(BLINK_LED, OUTPUT);
    digitalWrite(ENABLE_PASS_0TO1_PIN, HIGH); // enable pull-up resistor
    digitalWrite(ENABLE_PASS_1TO0_PIN, HIGH); // enable pull-up resistor
    digitalWrite(BLINK_LED, LOW);

	Wire.begin();
	EEPROM.setWPPin(18); // a guess...

	loadSettings();

	EEPROM.setWPPin(SysSettings.eepromWPPin);

    if (SysSettings.useSD) {	
		if (!sd.begin(SysSettings.SDCardSelPin, SPI_FULL_SPEED)) 
		{
			Logger::error("Could not initialize SDCard! No file logging will be possible!");
		}
		else SysSettings.SDCardInserted = true;
		if (settings.autoStartLogging) {
			SysSettings.logToFile = true;
			Logger::info("Automatically logging to file.");
		}
	}

    SerialUSB.print("Build number: ");
    SerialUSB.println(CFG_BUILD_NUM);

    sys_early_setup();
    setup_sys_io();

	if (settings.CAN0_Enabled)
	{
		if (settings.CAN0ListenOnly)
		{
			Can0.enable_autobaud_listen_mode();
		}
		else
		{
			Can0.disable_autobaud_listen_mode();
		}
		Can0.enable();
		Can0.begin(settings.CAN0Speed, SysSettings.CAN0EnablePin);
	}
	else Can0.disable();

	if (settings.CAN1_Enabled)
	{
		if (settings.CAN1ListenOnly)
		{
			Can1.enable_autobaud_listen_mode();
		}
		else
		{
			Can1.disable_autobaud_listen_mode();
		}
		Can1.enable();
		Can1.begin(settings.CAN1Speed, SysSettings.CAN1EnablePin);
		if (settings.singleWireMode)
		{
			setSWCANEnabled();
		}
		else
		{
			setSWCANSleep();
		}
	}
	else Can1.disable();

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

	
	SysSettings.lawicelMode = false;
	SysSettings.lawicelAutoPoll = false;
	SysSettings.lawicelTimestamping = false;
	SysSettings.lawicelPollCounter = 0;

	SerialUSB.print("Done with init\n");
	digitalWrite(BLINK_LED, HIGH);
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
  for (filter = 3; filter < 7; filter++) {
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

void toggleRXLED()
{
	SysSettings.rxToggle = !SysSettings.rxToggle;
	setLED(SysSettings.LED_CANRX, SysSettings.rxToggle);
}

void sendFrameToUSB(CAN_FRAME &frame, int whichBus) 
{
	uint8_t buff[22];
	uint8_t temp;
	uint32_t now = micros();
	
	if (SysSettings.lawicelMode)
	{
		if (frame.extended)
		{
			SerialUSB.print("T");
			sprintf((char *)buff, "%08x", frame.id);
			SerialUSB.print((char *)buff);
		}
		else
		{
			SerialUSB.print("t");
			sprintf((char *)buff, "%03x", frame.id);
			SerialUSB.print((char *)buff);
		}
		SerialUSB.print(frame.length);
		for (int i = 0; i < frame.length; i++)
		{
			sprintf((char *)buff, "%02x", frame.data.byte[i]);
			SerialUSB.print((char *)buff);
		}
		if (SysSettings.lawicelTimestamping)
		{
			uint16_t timestamp = (uint16_t)millis();
			sprintf((char *)buff, "%04x", timestamp);
			SerialUSB.print((char *)buff);
		}
		SerialUSB.write(13);
	}
	else
	{
		if (settings.useBinarySerialComm) {
			if (frame.extended) frame.id |= 1 << 31;
			serialBuffer[serialBufferLength++] = 0xF1;
			serialBuffer[serialBufferLength++] = 0; //0 = canbus frame sending
			serialBuffer[serialBufferLength++] = (uint8_t)(now & 0xFF);
			serialBuffer[serialBufferLength++] = (uint8_t)(now >> 8);
			serialBuffer[serialBufferLength++] = (uint8_t)(now >> 16);
			serialBuffer[serialBufferLength++] = (uint8_t)(now >> 24);
			serialBuffer[serialBufferLength++] = (uint8_t)(frame.id & 0xFF);
			serialBuffer[serialBufferLength++] = (uint8_t)(frame.id >> 8);
			serialBuffer[serialBufferLength++] = (uint8_t)(frame.id >> 16);
			serialBuffer[serialBufferLength++] = (uint8_t)(frame.id >> 24);
			serialBuffer[serialBufferLength++] = frame.length + (uint8_t)(whichBus << 4);
			for (int c = 0; c < frame.length; c++)
			{
				serialBuffer[serialBufferLength++] = frame.data.bytes[c];
			}
			//temp = checksumCalc(buff, 11 + frame.length);
			temp = 0;
			serialBuffer[serialBufferLength++] = temp;
			//SerialUSB.write(buff, 12 + frame.length);
		}
		else 
		{
			SerialUSB.print(micros());
			SerialUSB.print(" - ");
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
}

void sendFrameToFile(CAN_FRAME &frame, int whichBus)
{
	uint8_t buff[40];
	uint8_t temp;
	uint32_t timestamp;
	if (settings.fileOutputType == BINARYFILE) {
		if (frame.extended) frame.id |= 1 << 31;
		timestamp = micros();
		buff[0] = (uint8_t)(timestamp & 0xFF);
		buff[1] = (uint8_t)(timestamp >> 8);
		buff[2] = (uint8_t)(timestamp >> 16);
		buff[3] = (uint8_t)(timestamp >> 24);
		buff[4] = (uint8_t)(frame.id & 0xFF);
		buff[5] = (uint8_t)(frame.id >> 8);
		buff[6] = (uint8_t)(frame.id >> 16);
		buff[7] = (uint8_t)(frame.id >> 24);
		buff[8] = frame.length + (uint8_t)(whichBus << 4);
		for (int c = 0; c < frame.length; c++)
		{
			buff[9 + c] = frame.data.bytes[c];
		}
		Logger::fileRaw(buff, 9 + frame.length);
	}
	else if (settings.fileOutputType == GVRET)
	{
		sprintf((char *)buff, "%i,%x,%i,%i,%i", millis(), frame.id, frame.extended, whichBus, frame.length);
		Logger::fileRaw(buff, strlen((char *)buff));

		for (int c = 0; c < frame.length; c++)
		{
			sprintf((char *) buff, ",%x", frame.data.bytes[c]);
			Logger::fileRaw(buff, strlen((char *)buff));
		}
		buff[0] = '\r';
		buff[1] = '\n';
		Logger::fileRaw(buff, 2);
	}
	else if (settings.fileOutputType == CRTD)
	{
		int idBits = 11;
		if (frame.extended) idBits = 29;
		sprintf((char *)buff, "%f R%i %x", millis() / 1000.0f, idBits, frame.id);
		Logger::fileRaw(buff, strlen((char *)buff));

		for (int c = 0; c < frame.length; c++)
		{
			sprintf((char *) buff, " %x", frame.data.bytes[c]);
			Logger::fileRaw(buff, strlen((char *)buff));
		}
		buff[0] = '\r';
		buff[1] = '\n';
		Logger::fileRaw(buff, 2);
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
	static CAN_FRAME build_out_frame;
	static int out_bus;
	int in_byte;
	static byte buff[20];
	static int step = 0;
	static STATE state = IDLE;
	static uint32_t build_int;
	uint8_t temp8;
	uint16_t temp16;
	static bool markToggle = false;
	bool isConnected = false;
	int serialCnt;
	uint32_t now = micros();

	/*if (SerialUSB)*/ isConnected = true;

	//there is no switch debouncing here at the moment
	//if mark triggering causes bounce then debounce this later on.
	/*
	if (getDigital(0)) {
		if (!markToggle) {
			markToggle = true;
			if (!settings.useBinarySerialComm) SerialUSB.println("MARK TRIGGERED");
			else 
			{ //figure out some sort of binary comm for the mark.
			}
		}
	}
	else markToggle = false;
	*/

	//if (!SysSettings.lawicelMode || SysSettings.lawicelAutoPoll || SysSettings.lawicelPollCounter > 0)
	//{
		if (Can0.available()) {
			Can0.read(incoming);
			if (!digitalRead(ENABLE_PASS_0TO1_PIN)) Can1.sendFrame(incoming); // if pin is shorted to GND
			toggleRXLED();
			if (isConnected) sendFrameToUSB(incoming, 0);
			if (SysSettings.logToFile) sendFrameToFile(incoming, 0);
			//fwReceiver->gotFrame(&incoming);
		}

		if (Can1.available()) {
			Can1.read(incoming); 
			if (!digitalRead(ENABLE_PASS_1TO0_PIN)) Can0.sendFrame(incoming); // if pin is shorted to GND
			toggleRXLED();
			if (isConnected) sendFrameToUSB(incoming, 1);
			if (SysSettings.logToFile) sendFrameToFile(incoming, 1);
		}
		if (SysSettings.lawicelPollCounter > 0) SysSettings.lawicelPollCounter--;
	//}

  if (micros() - lastFlushMicros > SER_BUFF_FLUSH_INTERVAL)
  {
	if (serialBufferLength > 0)
	{
		SerialUSB.write(serialBuffer, serialBufferLength);
        	serialBufferLength = 0;
		lastFlushMicros = micros();
	}
  }

  serialCnt = 0;
  while (isConnected && (SerialUSB.available() > 0) && serialCnt < 128) {
	serialCnt++;
	in_byte = SerialUSB.read();
	   switch (state) {
	   case IDLE:
		   if (in_byte == 0xF1) state = GET_COMMAND;
		   else if (in_byte == 0xE7) 
		  {
			settings.useBinarySerialComm = true;
			SysSettings.lawicelMode = false;
		  }
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
			   buff[0] = 0xF1;
			   buff[1] = 1; //time sync
			   buff[2] = (uint8_t)(now & 0xFF);
			   buff[3] = (uint8_t)(now >> 8);
			   buff[4] = (uint8_t)(now >> 16);
			   buff[5] = (uint8_t)(now >> 24);
			   SerialUSB.write(buff, 6);
			   break;
		   case 2:
			   //immediately return the data for digital inputs
			   temp8 = getDigital(0) + (getDigital(1) << 1) + (getDigital(2) << 2) + (getDigital(3) << 3);
			   buff[0] = 0xF1;
			   buff[1] = 2; //digital inputs
			   buff[2] = temp8;
			   temp8 = checksumCalc(buff, 2);
			   buff[3] = temp8;
			   SerialUSB.write(buff, 4);
			   state = IDLE;
			   break;
		   case 3:
			   //immediately return data on analog inputs
			   temp16 = getAnalog(0);
			   buff[0] = 0xF1;
			   buff[1] = 3;
			   buff[2] = temp16 & 0xFF;
			   buff[3] = uint8_t(temp16 >> 8);
			   temp16 = getAnalog(1);
			   buff[4] = temp16 & 0xFF;
			   buff[5] = uint8_t(temp16 >> 8);
			   temp16 = getAnalog(2);
			   buff[6] = temp16 & 0xFF;
			   buff[7] = uint8_t(temp16 >> 8);
			   temp16 = getAnalog(3);
			   buff[8] = temp16 & 0xFF;
			   buff[9] = uint8_t(temp16 >> 8);
			   temp8 = checksumCalc(buff, 9);
			   buff[10] = temp8;
			   SerialUSB.write(buff, 11);
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
		   case 6:
			   //immediately return data on canbus params
			   buff[0] = 0xF1;
			   buff[1] = 6;
			   buff[2] = settings.CAN0_Enabled + ((unsigned char)settings.CAN0ListenOnly << 4);
			   buff[3] = settings.CAN0Speed;
			   buff[4] = settings.CAN0Speed >> 8;
			   buff[5] = settings.CAN0Speed >> 16;
			   buff[6] = settings.CAN0Speed >> 24;
			   buff[7] = settings.CAN1_Enabled + ((unsigned char)settings.CAN1ListenOnly << 4) + (unsigned char)settings.singleWireMode << 6;
			   buff[8] = settings.CAN1Speed;
			   buff[9] = settings.CAN1Speed >> 8;
			   buff[10] = settings.CAN1Speed >> 16;
			   buff[11] = settings.CAN1Speed >> 24;
			   SerialUSB.write(buff, 12);
			   state = IDLE;
			   break;
		   case 7:
			   //immediately return device information
			   buff[0] = 0xF1;
			   buff[1] = 7;
			   buff[2] = CFG_BUILD_NUM & 0xFF;
			   buff[3] = (CFG_BUILD_NUM >> 8);
			   buff[4] = EEPROM_VER;
			   buff[5] = (unsigned char)settings.fileOutputType;
			   buff[6] = (unsigned char)settings.autoStartLogging;
			   buff[7] = settings.singleWireMode;
			   SerialUSB.write(buff, 8);
			   state = IDLE;
			   break; 
		   case 8:
			   buff[0] = 0xF1;
			   state = SET_SINGLEWIRE_MODE;
			   step = 0;
			   break;
		   case 9:
			   buff[0] = 0xF1;
			   buff[1] = 0x09;
			   buff[2] = 0xDE;
			   buff[3] = 0xAD;
			   SerialUSB.write(buff, 4);
			   state = IDLE;
			   break;
		   case 10:
			   buff[0] = 0xF1;
			   state = SET_SYSTYPE;
			   step = 0;
			   break;
		   case 11:
			   state = ECHO_CAN_FRAME;
			   buff[0] = 0xF1;
			   step = 0;
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
		       out_bus = in_byte & 1;
		       break;
		   case 5:
			   build_out_frame.length = in_byte & 0xF;
			   if (build_out_frame.length > 8) build_out_frame.length = 8;
			   break;
		   default:
			   if (step < build_out_frame.length + 6)
			   {
			      build_out_frame.data.bytes[step - 6] = in_byte;
			   }
			   else 
			   {
				   state = IDLE;
				   //this would be the checksum byte. Compute and compare.
				   temp8 = checksumCalc(buff, step);
				   //if (temp8 == in_byte) 
				   //{
				   if (settings.singleWireMode == 1)
				   {
					   if (build_out_frame.id == 0x100)
					   {
						   if (out_bus == 1)
						   {
							   setSWCANWakeup();
							   delay(5);
						   }
					   }
				   }
				   if (out_bus == 0) Can0.sendFrame(build_out_frame);
				   if (out_bus == 1) Can1.sendFrame(build_out_frame);

				   if (settings.singleWireMode == 1)
				   {
					   if (build_out_frame.id == 0x100)
					   {
						   if (out_bus == 1)
						   {
							   delay(5);
							   setSWCANEnabled();
						   }
					   }
				   }
				   //}
			   }
			   break;
		   }
		   step++;
		   break;
	   case TIME_SYNC:
		   state = IDLE;
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
				   if (build_int & 0x80000000) //signals that enabled and listen only status are also being passed
				   {
					   if (build_int & 0x40000000)
					   {
						   settings.CAN0_Enabled = true;
						   Can0.enable();
					   }
					   else
					   {
						   settings.CAN0_Enabled = false;
						   Can0.disable();
					   }
					   if (build_int & 0x20000000)
					   {
						   settings.CAN0ListenOnly = true;
						   Can0.enable_autobaud_listen_mode();
					   }
					   else
					   {
						   settings.CAN0ListenOnly = false;
						   Can0.disable_autobaud_listen_mode();
					   }
				   }
				   else
				   {
					   Can0.enable(); //if not using extended status mode then just default to enabling - this was old behavior
					   settings.CAN0_Enabled = true;
				   }
				   build_int = build_int & 0xFFFFF;
				   if (build_int > 1000000) build_int = 1000000;				   
				   Can0.begin(build_int, SysSettings.CAN0EnablePin);
				   //Can0.set_baudrate(build_int);
				   settings.CAN0Speed = build_int;				   
			   }
			   else //disable first canbus
			   {
				   Can0.disable();
				   settings.CAN0_Enabled = false;
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
				   if (build_int & 0x80000000) //signals that enabled and listen only status are also being passed
				   {
					   if (build_int & 0x40000000)
					   {
						   settings.CAN1_Enabled = true;
						   Can1.enable();
					   }
					   else
					   {
						   settings.CAN1_Enabled = false;
						   Can1.disable();
					   }
					   if (build_int & 0x20000000)
					   {
						   settings.CAN1ListenOnly = true;
						   Can1.enable_autobaud_listen_mode();
					   }
					   else
					   {
						   settings.CAN1ListenOnly = false;
						   Can1.disable_autobaud_listen_mode();
					   }
				   }
				   else
				   {
					   Can1.enable(); //if not using extended status mode then just default to enabling - this was old behavior
					   settings.CAN1_Enabled = true;
				   }
				   build_int = build_int & 0xFFFFF;
				   if (build_int > 1000000) build_int = 1000000;
				   Can1.begin(build_int, SysSettings.CAN1EnablePin);
				   if (settings.singleWireMode) setSWCANEnabled();
				   else setSWCANSleep();
				   //Can1.set_baudrate(build_int);

				   settings.CAN1Speed = build_int;				   
			   }
			   else //disable second canbus
			   {
				   setSWCANSleep();
				   Can1.disable();
				   settings.CAN1_Enabled = false;
			   }
			   state = IDLE;
			    //now, write out the new canbus settings to EEPROM
				EEPROM.write(EEPROM_PAGE, settings);
				setPromiscuousMode();
			   break;
		   }
		   step++;
		   break;
	   case SET_SINGLEWIRE_MODE:
		   if (in_byte == 0x10) 
		   {
			   settings.singleWireMode = true;
			   setSWCANEnabled();
		   }
		   else 
		   {
			   settings.singleWireMode = false;
			   setSWCANSleep();
		   }
		   EEPROM.write(EEPROM_PAGE, settings);
		   state = IDLE;
		   break;
	   case SET_SYSTYPE:
		   settings.sysType = in_byte;		   
		   EEPROM.write(EEPROM_PAGE, settings);
		   loadSettings();
		   state = IDLE;
		   break;
	   case ECHO_CAN_FRAME:
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
		       out_bus = in_byte & 1;
		       break;
		   case 5:
			   build_out_frame.length = in_byte & 0xF;
			   if (build_out_frame.length > 8) build_out_frame.length = 8;
			   break;
		   default:
			   if (step < build_out_frame.length + 6)
			   {
			      build_out_frame.data.bytes[step - 6] = in_byte;
			   }
			   else 
			   {
				   state = IDLE;
				   //this would be the checksum byte. Compute and compare.
				   temp8 = checksumCalc(buff, step);
				   //if (temp8 == in_byte) 
				   //{
				   toggleRXLED();
				   if (isConnected) sendFrameToUSB(build_out_frame, 0);
				   //}
			   }
			   break;
		   }
		   step++;
		   break;
	   }
  }
	Logger::loop();
	//this should still be here. It checks for a flag set during an interrupt
	//sys_io_adc_poll();
}

