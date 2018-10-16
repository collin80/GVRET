GEV-RET
=======

Generalized Electric Vehicle Reverse Engineering Tool

A project turn the GEVCU hardware into a specialized reverse engineering tool.

The project now builds in the Arduino IDE. So, use it to compile, send the firmware to the Arduino, and monitor serial. It all works very nicely.

#### Requirements:

You will need the following to compile this project:

- Compatible hardware such as [GEVCU](http://store.evtv.me/proddetail.php?prod=gevcu&cat=28), CANDue (1.1 or 2.0), or EVTVDue
- [Arduino IDE](https://www.arduino.cc/en/Main/Software) 1.5.4 or higher (tested all of the way up to 1.6.6)
- [due_can](https://github.com/collin80/due_can) - Object oriented canbus library for Arduino Due compatible boards.
- [can_common](https://github.com/collin80/can_common) - Common structs and functions for CAN libraries.
- [MCP2515](https://github.com/macchina/mcp2515) - Facilitate CAN functionality with the onboard MCP2515 CAN controller.
- [due_wire](https://github.com/collin80/due_wire) - An alternative I2C library for Due with DMA support.
- [SdFat](https://github.com/collin80/SdFat-beta) - Arduino FAT16/FAT32 Library (Note - now using Beta version!)
- [DueFlashStorage](https://github.com/collin80/DueFlashStorage) DueFlashStorage saves non-volatile data for Arduino Due.
- [Wire_EEPROM](https://github.com/collin80/Wire_EEPROM) I2C based routines to support EEPROM on the Due.

All libraries belong in %USERPROFILE%\Documents\Arduino\libraries (Windows) or ~/Arduino/libraries (Linux/Mac).
You will need to remove -master or any other postfixes. Your library folders should be named as above.

The canbus is supposed to be terminated on both ends of the bus. This should not be a problem as this firmware will be used to reverse engineer existing buses.

#### License:

This software is MIT licensed:

Copyright (c) 2014-2017 Collin Kidder, Michael Neuweiler, Charles Galpin

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

