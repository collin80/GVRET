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
	CAN.init(CAN_BPS_250K);
	CAN2.init(CAN_BPS_250K);

#ifdef CFG_EFFICIENCY_CALCS
	mainLoopTimer = new PerfTimer();
#endif
}

void loop()
{
	CAN_FRAME incoming;

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


   //this should still be here. It checks for a flag set during an interrupt
   sys_io_adc_poll();

#ifdef CFG_EFFICIENCY_CALCS
	mainLoopTimer->stop();
#endif
}
