/*
 * GEVCU.h
 *
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

#ifndef GEVCU_H_
#define GEVCU_H_

#include <Arduino.h>
#include "due_can.h"
#include "Heartbeat.h"
#include "SystemIO.h"
#include "MemCache.h"
#include "PerfTimer.h"

#ifdef __cplusplus
extern "C" {
#endif
void loop();
void setup();
#ifdef __cplusplus
} // extern "C"
#endif

#define CFG_BUILD_NUM   300
#define CFG_VERSION "GEV-RET Alpha Version July 2, 2014"

//define this to add in latency and efficiency calculations. Comment it out for builds you're going to 
//use in an actual car. No need to waste cycles for 99% of everyone using the code.
#define CFG_EFFICIENCY_CALCS


/*
 * SERIAL CONFIGURATION
 */
#define CFG_SERIAL_SPEED 115200
//#define SerialUSB Serial // re-route serial-usb output to programming port ;) comment if output should go to std usb

/*
 * TIMER INTERVALS
 *
 * specify the intervals (microseconds) at which each device type should be "ticked"
 * try to use the same numbers for several devices because then they will share
 * the same timer (out of a limited number of 9 timers).
 */
#define CFG_TICK_INTERVAL_HEARTBEAT                 2000000
#define CFG_TICK_INTERVAL_MEM_CACHE                 40000
#define CFG_TICK_INTERVAL_SYSTEM_IO                 200000

/*
 * CAN BUS CONFIGURATION
 */
#define CFG_CAN0_SPEED CAN_BPS_500K // specify the speed of the CAN0 bus (EV)
#define CFG_CAN1_SPEED CAN_BPS_500K // specify the speed of the CAN1 bus (Car)
#define CFG_CAN0_NUM_RX_MAILBOXES 7 // amount of CAN bus receive mailboxes for CAN0
#define CFG_CAN1_NUM_RX_MAILBOXES 7 // amount of CAN bus receive mailboxes for CAN1

/*
 * ARRAY SIZE
 *
 * Define the maximum number of various object lists.
 * These values should normally not be changed.
 */
#define CFG_CAN_NUM_OBSERVERS 10 // maximum number of device subscriptions per CAN bus
#define CFG_TIMER_NUM_OBSERVERS 9 // the maximum number of supported observers per timer
#define CFG_TIMER_BUFFER_SIZE 100 // the size of the queuing buffer for TickHandler

/*
 * PIN ASSIGNMENT
 */
#define CFG_OUTPUT_NONE    255
#define BLINK_LED          73 //13 is L, 73 is TX, 72 is RX

#define CFG_NUMBER_ANALOG_INPUTS  4
#define CFG_NUMBER_DIGITAL_INPUTS 4
#define CFG_NUMBER_DIGITAL_OUTPUTS  8

#endif /* GEVCU_H_ */



