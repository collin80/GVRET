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

#include "due_can_special.h"

#define CFG_BUILD_NUM	300
#define CFG_VERSION "GVRET alpha 2014-07-04"

/*
 * SERIAL CONFIGURATION
 */
#define CFG_SERIAL_SPEED 115200
//#define SerialUSB Serial // re-route serial-usb output to programming port ;) comment if output should go to std usb

/*
 * CAN BUS CONFIGURATION
 */
#define CFG_CAN0_SPEED CAN_BPS_500K // specify the speed of the CAN0 bus (EV)
#define CFG_CAN1_SPEED CAN_BPS_500K // specify the speed of the CAN1 bus (Car)
#define CFG_CAN0_NUM_RX_MAILBOXES 7 // amount of CAN bus receive mailboxes for CAN0
#define CFG_CAN1_NUM_RX_MAILBOXES 7 // amount of CAN bus receive mailboxes for CAN1
#define CFG_CANTHROTTLE_MAX_NUM_LOST_MSG 3 // maximum number of lost messages allowed

#define BLINK_LED          73 //13 is L, 73 is TX, 72 is RX

#define NUM_ANALOG	4
#define NUM_DIGITAL	4
#define NUM_OUTPUT	8

#endif /* CONFIG_H_ */
