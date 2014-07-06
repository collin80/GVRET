/*
 GEV-RET.ino

 Created: 7/2/2014 10:10:14 PM
 Author: Collin Kidder

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

 */

#include "GVRET.h"
#include "due_can_special.h"

// The following includes are required in the .ino file by the Arduino IDE in order to properly
// identify the required libraries for the build.
#include <due_rtc.h>

//RTC_clock rtc_clock(XTAL); //init RTC with the external 32k crystal as a reference

//Evil, global variables
PerfTimer *mainLoopTimer;

byte i = 0;

//initializes all the system EEPROM values. Chances are this should be broken out a bit but
//there is only one checksum check for all of them so it's simple to do it all here.
void initSysEEPROM()
{
    //three temporary storage places to make saving to EEPROM easy
    uint8_t eight;
    uint16_t sixteen;
    uint32_t thirtytwo;
}

void setup()
{
    pinMode(BLINK_LED, OUTPUT);
    digitalWrite(BLINK_LED, LOW);

    SerialUSB.print("Build number: ");
    SerialUSB.println(CFG_BUILD_NUM);

    //rtc_clock.init();
    //Now, we have no idea what the real time is but the EEPROM should have stored a time in the past.
    //It's better than nothing while we try to figure out the proper time.
    /*
     uint32_t temp;
     sysPrefs->read(EESYS_RTC_TIME, &temp);
     rtc_clock.change_time(temp);
     sysPrefs->read(EESYS_RTC_DATE, &temp);
     rtc_clock.change_date(temp);

     Logger::info("RTC init ok");
     */

    sys_early_setup();
	setup_sys_io();

	//Now, initialize canbus ports (don't actually do this here. Fix it to init canbus only when asked to)
//	CAN.init(CAN_BPS_500K);
//	CAN2.init(CAN_BPS_250K);

#ifdef CFG_EFFICIENCY_CALCS
	mainLoopTimer = new PerfTimer();
#endif
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
	buff[6 + frame.length] = temp;
	SerialUSB.write(buff, 8 + frame.length);
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
	CAN_FRAME incoming;
	CAN_FRAME build_out_frame;
	int in_byte;
	static byte buff[20];
	static int step = 0;
	static STATE state = IDLE;
	static int build_int;
	uint8_t temp8;
	uint16_t temp16;

#ifdef CFG_EFFICIENCY_CALCS
	static int counts = 0;
	counts++;
	if (counts > 200000) {
		counts = 0;
		mainLoopTimer->printValues();
	}

	mainLoopTimer->start();
#endif

  if (CAN.rx_avail()) {
	CAN.get_rx_buff(incoming);
	sendFrameToUSB(incoming, 0);
  }

  if (CAN2.rx_avail()) {
	CAN2.get_rx_buff(incoming); 
	sendFrameToUSB(incoming, 1);
  }

  if (SerialUSB.available()) {
	in_byte = SerialUSB.read();
	if (in_byte != -1) { //false alarm....
	   switch (state) {
	   case IDLE:
		   if (in_byte == 0xF1) state = GET_COMMAND;
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
						CAN.sendFrame(build_out_frame);
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
				   CAN.init(build_int);
			   }
			   else //disable first canbus
			   {
				   CAN.disable();
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
				   CAN2.init(build_int);
			   }
			   else //disable first canbus
			   {
				   CAN2.disable();
			   }
			   state = IDLE;
			   break;
		   }
		   step++;
		   break;
	   }
	}
  }
   //this should still be here. It checks for a flag set during an interrupt
   sys_io_adc_poll();

#ifdef CFG_EFFICIENCY_CALCS
	mainLoopTimer->stop();
#endif
}
