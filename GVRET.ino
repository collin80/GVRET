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


/*
Loop executes as often as possible all the while interrupts fire in the background.
The serial comm protocol is as follows:
All commands start with 0xF1 this helps to synchronize if there were comm issues
Then the next byte specifies which command this is. 
Then the command data bytes which are specific to the command
Lastly, 0xF2 delimits the end of the command.
Any bytes between 0xF2 and 0xF1 are thrown away

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
  }

  if (CAN2.rx_avail()) {
	CAN2.get_rx_buff(incoming); 
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
			   buff[2] = 0xF2;
			   SerialUSB.write(buff, 3);
			   state = RECEIVE_END_BYTE;
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
			   buff[9] = 0xF2;
			   SerialUSB.write(buff, 10);
			   state = RECEIVE_END_BYTE;
			   break;
		   case 4:
			   state = SET_DIG_OUTPUTS;
			   break;
		   case 5:
			   state = SETUP_CANBUS;
			   break;
		   }
		   break;
	   case BUILD_CAN_FRAME:
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
			   if (step < build_out_frame.length + 4)
			   {
			      build_out_frame.data.bytes[step - 5] = in_byte;
			   }
			   else 
			   {
				   build_out_frame.data.bytes[step - 5] = in_byte;
				   state = RECEIVE_END_BYTE;
				   //frame is built as of this point. We can send it
				   //Need to find a way for sending end to specify which bus though!
				   CAN.sendFrame(build_out_frame);
			   }
			   break;
		   }
		   step++;
		   break;
	   case TIME_SYNC:
		   break;
	   case SET_DIG_OUTPUTS:
		   for (int c = 0; c < 8; c++) 
		   {
			   if (in_byte & (1 << c)) setOutput(c, true);
			   else setOutput(c, false);
		   }
		   state = RECEIVE_END_BYTE;
		   break;
	   case SETUP_CANBUS:
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
			   state = RECEIVE_END_BYTE;
			   break;
		   }
		   step++;
		   break;
	   case RECEIVE_END_BYTE:
		   if (in_byte == 0xF2) state = IDLE;
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
