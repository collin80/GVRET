#include <Arduino.h>
#include <due_wire.h>
#include "EEPROM.h"

EEPROMCLASS::EEPROMCLASS(TwoWire *i2cport)
{
    port = i2cport;
    //port->begin();
}

uint8_t EEPROMCLASS::readByte(uint32_t address) 
{
  uint8_t d,e;
  uint8_t buffer[3];
  uint8_t i2c_id;

   buffer[0] = ((address & 0xFF00) >> 8);
   buffer[1] = ((uint8_t)(address & 0x00FF));
   i2c_id = 0b01010000 + ((address >> 16) & 0x03); //10100 is the chip ID then the two upper bits of the address
    port->beginTransmission(i2c_id);  
    port->write(buffer, 2);
    port->endTransmission(false); //do NOT generate stop 
    port->requestFrom(i2c_id, 1); //this will generate stop though.
    if(port->available())    
    { 
        d = port->read(); // receive a byte as character
        return d;
    }
    return 255;
}    

void EEPROMCLASS::writeByte(uint32_t address, uint8_t valu) 
{
  uint16_t d;
  uint8_t buffer[3];
  uint8_t i2c_id;

  while (writeTime > millis());

  buffer[0] = ((address & 0xFF00) >> 8);
  buffer[1] = ((uint8_t)(address & 0x00FF));
  buffer[2] = valu;
  i2c_id = 0b01010000 + ((address >> 16) & 0x03); //10100 is the chip ID then the two upper bits of the address
  port->beginTransmission(i2c_id);
  port->write(buffer, 3);
  port->endTransmission(true);
  writeTime = millis() + 8;
}

void EEPROMCLASS::setWPPin(uint8_t pin) {
	pinMode(pin, OUTPUT);
	digitalWrite(pin, LOW);
}

//Instantiate the class with the proper name to pretend this is still the class from the non-Due arduinos
EEPROMCLASS EEPROM(&Wire);

