#ifndef EEPROM_H_
#define EEPROM_H_

#include "Arduino.h"

class EEPROMCLASS {
public:
	uint8_t readByte(uint32_t address);
	void writeByte(uint32_t address, uint8_t valu);

	void setWPPin(uint8_t pin);

    EEPROMCLASS(TwoWire *i2cport);
    
    //read and write input address in bytes. The page size is assumed to be 32 bytes. If it's larger we'll
    //be writing the same page potentially multiple times but its not the end of the world.

	template <class T> int write(int ee, const T& value)
	{	
		uint8_t buffer[35];
		uint8_t i2c_id;
		const byte* p = (const byte*)(const void*)&value;
		unsigned int i;
        int valSize = sizeof(value);
        int page;
        int writeAddr;
        int writeSize;
        
        page = (ee / 32);
        
        while (valSize > 0)
        {
            writeAddr = page * 32;
            while (writeTime > millis());

            for (i = 0; i < 35; i++) buffer[i] = 0xFF;
            buffer[0] = ((writeAddr & 0xFF00) >> 8);
            buffer[1] = ((uint8_t)(writeAddr & 0x00FF));
            i2c_id = 0b01010000 + ((ee >> 8) & 0x03); //10100 is the chip ID then the two upper bits of the address
            writeSize = valSize;
            if (writeSize > 32) writeSize = 32;
            for (i = 0; i < writeSize; i++) 
            {
                buffer[i + 2] = *p++;
            }
            //Blast page in single shot
            port->beginTransmission(i2c_id);
            port->write(buffer, writeSize + 2);
            port->endTransmission(true);
            valSize -= writeSize;
            page++;

            writeTime = millis() + 30; //wait for transfer over i2c plus eeprom write
        }

		return i;
	}

	template <class T> int read(int ee, T& value)
	{
		uint8_t buffer[3];
		uint8_t i2c_id;
	    byte* p = (byte*)(void*)&value;
		unsigned int i;
        int valSize = sizeof(value);
        int page;
        int readAddr;
        int readSize;
        
        page = (ee / 32);
        
        while (valSize > 0)
        {
            readAddr = page * 32;
            buffer[0] = ((readAddr & 0xFF00) >> 8);
            buffer[1] = ((uint8_t)(readAddr & 0x00FF));
            i2c_id = 0b01010000 + ((ee >> 8) & 0x03); //10100 is the chip ID then the two upper bits of the address
		    //send the address to get the chip ready.
            port->beginTransmission(i2c_id);  
		    port->write(buffer, 2);
		    port->endTransmission(false); //do NOT generate stop
		    //Now, tell it we'd like to read up to a whole page
		    port->requestFrom(i2c_id, sizeof(value)); //this will generate stop though.
            readSize = valSize;
            if (readSize > 32) readSize = 32;
	        for (i = 0; i < sizeof(value); i++) 
		    {
                if (port->available()) *p++ = port->read();
		    }
		    valSize -= readSize;
            page++;
        }
		return i;
	}

private:
	uint32_t writeTime;
    TwoWire *port;
};

extern EEPROMCLASS EEPROM;

#endif
