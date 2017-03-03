/*
 * Logger.cpp
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

#include "Logger.h"
#include "config.h"
#include "sys_io.h"
#include <due_wire.h>
#include "EEPROM.h"
#include <SdFat.h>
#include <Arduino_Due_SD_HSCMI.h>

Logger::LogLevel Logger::logLevel = Logger::Info;
uint32_t Logger::lastLogTime = 0;
uint16_t Logger::fileBuffWritePtr = 0;
SdFile Logger::fileRef; //file we're logging to
uint8_t Logger::filebuffer[BUF_SIZE]; //size of buffer for file output
uint32_t Logger::lastWriteTime = 0;
extern FileStore FS;

/*
 * Output a debug message with a variable amount of parameters.
 * printf() style, see Logger::log()
 *
 */
void Logger::debug(const char *message, ...)
{
    if (logLevel > Debug) {
        return;
    }

    va_list args;
    va_start(args, message);
    Logger::log(Debug, message, args);
    va_end(args);
}

/*
 * Output a info message with a variable amount of parameters
 * printf() style, see Logger::log()
 */
void Logger::info(const char *message, ...)
{
    if (logLevel > Info) {
        return;
    }

    va_list args;
    va_start(args, message);
    Logger::log(Info, message, args);
    va_end(args);
}

/*
 * Output a warning message with a variable amount of parameters
 * printf() style, see Logger::log()
 */
void Logger::warn(const char *message, ...)
{
    if (logLevel > Warn) {
        return;
    }

    va_list args;
    va_start(args, message);
    Logger::log(Warn, message, args);
    va_end(args);
}

/*
 * Output a error message with a variable amount of parameters
 * printf() style, see Logger::log()
 */
void Logger::error(const char *message, ...)
{
    if (logLevel > Error) {
        return;
    }

    va_list args;
    va_start(args, message);
    Logger::log(Error, message, args);
    va_end(args);
}

/*
 * Output a comnsole message with a variable amount of parameters
 * printf() style, see Logger::logMessage()
 */
void Logger::console(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    Logger::logMessage(message, args);
    va_end(args);
}

void Logger::buffPutChar(char c)
{
	*(filebuffer + fileBuffWritePtr++) = c;
}

void Logger::buffPutString(const char *c)
{
	while (*c) *(filebuffer + fileBuffWritePtr++) = *c++;
}

void Logger::flushFileBuff()
{
	Logger::debug("Write to SD Card %i bytes", fileBuffWritePtr);
	lastWriteTime = millis();
    
    if (settings.sysType < 3)
    {
        if (fileRef.write(filebuffer, fileBuffWritePtr) != fileBuffWritePtr) {
            Logger::error("Write to SDCard failed!");
            SysSettings.useSD = false; //borked so stop trying.
            fileBuffWritePtr = 0;
            return;
        }
        fileRef.sync(); //needed in order to update the file if you aren't closing it ever
    }
    else //Macchina M2 
    {
        if (!FS.Write((const char *)filebuffer, fileBuffWritePtr)) {
            Logger::error("Write to SDCard failed!");
            SysSettings.useSD = false;
            fileBuffWritePtr = 0;
            return;
        }
        FS.Flush(); //force write of the data to card
    }
        
	SysSettings.logToggle = !SysSettings.logToggle;
	setLED(SysSettings.LED_LOGGING, SysSettings.logToggle);
	fileBuffWritePtr = 0;
}

boolean Logger::setupFile()
{
    if (settings.sysType < 3)
    {
        if (!fileRef.isOpen())  //file not open. Try to open it.
        {
            String filename;
            if (settings.appendFile == 1)
            {
                filename = String(settings.fileNameBase);
                filename.concat(".");
                filename.concat(settings.fileNameExt);
                fileRef.open(filename.c_str(), O_APPEND | O_WRITE);
            }
            else {
                filename = String(settings.fileNameBase);
                filename.concat(settings.fileNum++);
                filename.concat(".");
                filename.concat(settings.fileNameExt);
                EEPROM.write(EEPROM_ADDR, settings); //save settings to save updated filenum
                fileRef.open(filename.c_str(), O_CREAT | O_TRUNC | O_WRITE);
            }
            if (!fileRef.isOpen())
            {
                Logger::error("open failed");
                return false;
            }
        }
	}
	else //Macchina M2
    {
        if (!FS.inUse)
        {
            String filename;
            if (settings.appendFile == 1)
            {
                filename = String(settings.fileNameBase);
                filename.concat(".");
                filename.concat(settings.fileNameExt);
                FS.Open("0:", filename.c_str(), true);
                FS.GoToEnd();
            }
            else {
                filename = String(settings.fileNameBase);
                filename.concat(settings.fileNum++);
                filename.concat(".");
                filename.concat(settings.fileNameExt);
                EEPROM.write(EEPROM_ADDR, settings); //save settings to save updated filenum
                FS.Open("0:", filename.c_str(), true);                
            }
            if (!FS.inUse)
            {
                Logger::error("open failed");
                return false;
            }            
        }
    }

	//Before we add the next frame see if the buffer is nearly full. if so flush it first.
	if (fileBuffWritePtr > BUF_SIZE - 40)
	{
		flushFileBuff();
	}
	return true;
}

void Logger::loop()
{
	if (fileBuffWritePtr > 0) {
		if (millis() > (lastWriteTime + 1000)) //if it's been at least 1 second since the last write and we have data to write
		{
			flushFileBuff();
		}
	}
}

void Logger::file(const char *message, ...)
{
	if (!SysSettings.SDCardInserted) return; // not possible to log without card

	char buff[20];

	va_list args;
	va_start(args, message);	

	if (!setupFile()) return;
	
	for (; *message != 0; ++message) {
		if (*message == '%') {
			++message;

			if (*message == '\0') {
				break;
			}

			if (*message == '%') {
				buffPutChar(*message);
				continue;
			}

			if (*message == 's') {
				register char *s = (char *)va_arg(args, int);
				buffPutString(s);
				continue;
			}

			if (*message == 'd' || *message == 'i') {
				sprintf(buff, "%i", va_arg(args, int));
				buffPutString(buff);
				continue;
			}

			if (*message == 'f') {
				sprintf(buff, "%f0.2", va_arg(args, double));
				buffPutString(buff);
				continue;
			}

			if (*message == 'x') {
				sprintf(buff, "%x", va_arg(args, int));
				buffPutString(buff);				
				continue;
			}

			if (*message == 'X') {
				buffPutString("0x");
				sprintf(buff, "%x", va_arg(args, int));
				buffPutString(buff);
				continue;
			}

			if (*message == 'l') {
				sprintf(buff, "%l", va_arg(args, long));
				buffPutString(buff);
				continue;
			}

			if (*message == 'c') {
				buffPutChar(va_arg(args, int));
				continue;
			}

			if (*message == 't') {
				if (va_arg(args, int) == 1) {
					buffPutString("T");
				}
				else {
					buffPutString("F");
				}

				continue;
			}

			if (*message == 'T') {
				if (va_arg(args, int) == 1) {
					buffPutString("TRUE");
				}
				else {
					buffPutString("FALSE");
				}
				continue;
			}

		}

		buffPutChar(*message);
	}

	buffPutString("\r\n");

	va_end(args);

}

void Logger::fileRaw(uint8_t* buff, int sz) 
{
	if (!SysSettings.SDCardInserted) return; // not possible to log without card

	if (!setupFile()) return;

	for (int i; i < sz; i++) {
		buffPutChar(*buff++);
	}
}

/*
 * Set the log level. Any output below the specified log level will be omitted.
 */
void Logger::setLoglevel(LogLevel level)
{
    logLevel = level;
}

/*
 * Retrieve the current log level.
 */
Logger::LogLevel Logger::getLogLevel()
{
    return logLevel;
}

/*
 * Return a timestamp when the last log entry was made.
 */
uint32_t Logger::getLastLogTime()
{
    return lastLogTime;
}

/*
 * Returns if debug log level is enabled. This can be used in time critical
 * situations to prevent unnecessary string concatenation (if the message won't
 * be logged in the end).
 *
 * Example:
 * if (Logger::isDebug()) {
 *    Logger::debug("current time: %d", millis());
 * }
 */
boolean Logger::isDebug()
{
    return logLevel == Debug;
}

/*
 * Output a log message (called by debug(), info(), warn(), error(), console())
 *
 * Supports printf() like syntax:
 *
 * %% - outputs a '%' character
 * %s - prints the next parameter as string
 * %d - prints the next parameter as decimal
 * %f - prints the next parameter as double float
 * %x - prints the next parameter as hex value
 * %X - prints the next parameter as hex value with '0x' added before
 * %b - prints the next parameter as binary value
 * %B - prints the next parameter as binary value with '0b' added before
 * %l - prints the next parameter as long
 * %c - prints the next parameter as a character
 * %t - prints the next parameter as boolean ('T' or 'F')
 * %T - prints the next parameter as boolean ('true' or 'false')
 */
void Logger::log(LogLevel level, const char *format, va_list args)
{
    lastLogTime = millis();
    SerialUSB.print(lastLogTime);
    SerialUSB.print(" - ");

    switch (level) {
        case Debug:
            SerialUSB.print("DEBUG");
            break;

        case Info:
            SerialUSB.print("INFO");
            break;

        case Warn:
            SerialUSB.print("WARNING");
            break;

        case Error:
            SerialUSB.print("ERROR");
            break;
    }

    SerialUSB.print(": ");

    logMessage(format, args);
}

/*
 * Output a log message (called by log(), console())
 *
 * Supports printf() like syntax:
 *
 * %% - outputs a '%' character
 * %s - prints the next parameter as string
 * %d - prints the next parameter as decimal
 * %f - prints the next parameter as double float
 * %x - prints the next parameter as hex value
 * %X - prints the next parameter as hex value with '0x' added before
 * %b - prints the next parameter as binary value
 * %B - prints the next parameter as binary value with '0b' added before
 * %l - prints the next parameter as long
 * %c - prints the next parameter as a character
 * %t - prints the next parameter as boolean ('T' or 'F')
 * %T - prints the next parameter as boolean ('true' or 'false')
 */
void Logger::logMessage(const char *format, va_list args)
{
    for (; *format != 0; ++format) {
        if (*format == '%') {
            ++format;

            if (*format == '\0') {
                break;
            }

            if (*format == '%') {
                SerialUSB.print(*format);
                continue;
            }

            if (*format == 's') {
                register char *s = (char *) va_arg(args, int);
                SerialUSB.print(s);
                continue;
            }

            if (*format == 'd' || *format == 'i') {
                SerialUSB.print(va_arg(args, int), DEC);
                continue;
            }

            if (*format == 'f') {
                SerialUSB.print(va_arg(args, double), 2);
                continue;
            }

            if (*format == 'x') {
                SerialUSB.print(va_arg(args, int), HEX);
                continue;
            }

            if (*format == 'X') {
                SerialUSB.print("0x");
                SerialUSB.print(va_arg(args, int), HEX);
                continue;
            }

            if (*format == 'b') {
                SerialUSB.print(va_arg(args, int), BIN);
                continue;
            }

            if (*format == 'B') {
                SerialUSB.print("0b");
                SerialUSB.print(va_arg(args, int), BIN);
                continue;
            }

            if (*format == 'l') {
                SerialUSB.print(va_arg(args, long), DEC);
                continue;
            }

            if (*format == 'c') {
                SerialUSB.print(va_arg(args, int));
                continue;
            }

            if (*format == 't') {
                if (va_arg(args, int) == 1) {
                    SerialUSB.print("T");
                } else {
                    SerialUSB.print("F");
                }

                continue;
            }

            if (*format == 'T') {
                if (va_arg(args, int) == 1) {
                    SerialUSB.print("TRUE");
                } else {
                    SerialUSB.print("FALSE");
                }

                continue;
            }

        }

        SerialUSB.print(*format);
    }

    SerialUSB.println();
}


