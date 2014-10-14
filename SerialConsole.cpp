/*
 * SerialConsole.cpp
 *
 Copyright (c) 2014 Collin Kidder

 Shamelessly copied from the version in GEVCU

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

#include "SerialConsole.h"

SerialConsole::SerialConsole() {
	init();
}

void SerialConsole::init() {
	//State variables for serial console
	ptrBuffer = 0;
	state = STATE_ROOT_MENU;      
}

void SerialConsole::printMenu() {
	char buff[80];
	//Show build # here as well in case people are using the native port and don't get to see the start up messages
	SerialUSB.print("Build number: ");
	SerialUSB.println(CFG_BUILD_NUM);
	SerialUSB.println("System Menu:");
	SerialUSB.println();
	SerialUSB.println("Enable line endings of some sort (LF, CR, CRLF)");
	SerialUSB.println();
	SerialUSB.println("Short Commands:");
	SerialUSB.println("h = help (displays this message)");
	SerialUSB.println("K = set all outputs high");
	SerialUSB.println("J = set all outputs low");
	SerialUSB.println("R = reset to factory defaults");
	SerialUSB.println("s = Start logging to file");
	SerialUSB.println("S = Stop logging to file");
	SerialUSB.println();
	SerialUSB.println("Config Commands (enter command=newvalue). Current values shown in parenthesis:");
    SerialUSB.println();

    Logger::console("LOGLEVEL=%i - set log level (0=debug, 1=info, 2=warn, 3=error, 4=off)", Logger::getLogLevel());
	SerialUSB.println();

	Logger::console("CAN0EN=%i - Enable/Disable CAN0 (0 = Disable, 1 = Enable)", settings.CAN0_Enabled);
	Logger::console("CAN0SPEED=%i - Set speed of CAN0 in baud (125000, 250000, etc)", settings.CAN0Speed);
	for (int i = 0; i < 8; i++) {
		sprintf(buff, "CAN0FILTER%i=%%i,%%i,%%i,%%i (ID, Mask, Extended, Enabled)", i);
		Logger::console(buff, settings.CAN0Filters[i].id, settings.CAN0Filters[i].mask,
			settings.CAN0Filters[i].extended, settings.CAN0Filters[i].enabled);
	}
	SerialUSB.println();

	Logger::console("CAN1EN=%i - Enable/Disable CAN1 (0 = Disable, 1 = Enable)", settings.CAN1_Enabled);
	Logger::console("CAN1SPEED=%i - Set speed of CAN1 in baud (125000, 250000, etc)", settings.CAN1Speed);
	for (int i = 0; i < 8; i++) {
		sprintf(buff, "CAN0FILTER%i=%%i,%%i,%%i,%%i (ID, Mask, Extended, Enabled)", i);
		Logger::console(buff, settings.CAN0Filters[i].id, settings.CAN0Filters[i].mask,
			settings.CAN0Filters[i].extended, settings.CAN0Filters[i].enabled);
	}
	SerialUSB.println();

	Logger::console("BINSERIAL=%i - Enable/Disable Binary Sending of CANBus Frames to Serial (0=Dis, 1=En)", settings.useBinarySerialComm);
	Logger::console("BINFILE=%i - Enable/Disable Binary File Format (0=Ascii, 1=Binary)", settings.useBinaryFile);
	SerialUSB.println();

	Logger::console("FILEBASE=%s - Set filename base for saving", (char *)settings.fileNameBase);
	Logger::console("FILEEXT=%s - Set filename ext for saving", (char *)settings.fileNameExt);
	Logger::console("FILENUM=%i - Set incrementing number for filename", settings.fileNum);
	Logger::console("FILEAPPEND=%i - Append to file (no numbers) or use incrementing numbers after basename (0=Incrementing Numbers, 1=Append)", settings.appendFile);
}

/*	There is a help menu (press H or h or ?)
 This is no longer going to be a simple single character console.
 Now the system can handle up to 80 input characters. Commands are submitted
 by sending line ending (LF, CR, or both)
 */
void SerialConsole::rcvCharacter(uint8_t chr) {
	if (chr == 10 || chr == 13) { //command done. Parse it.
		handleConsoleCmd();
		ptrBuffer = 0; //reset line counter once the line has been processed
	} else {
		cmdBuffer[ptrBuffer++] = (unsigned char) chr;
		if (ptrBuffer > 79)
			ptrBuffer = 79;
	}
}

void SerialConsole::handleConsoleCmd() {
	if (state == STATE_ROOT_MENU) {
		if (ptrBuffer == 1) { //command is a single ascii character
			handleShortCmd();
		} else { //if cmd over 1 char then assume (for now) that it is a config line
			handleConfigCmd();
		}
	}
}

/*For simplicity the configuration setting code uses four characters for each configuration choice. This makes things easier for
 comparison purposes.
 */
void SerialConsole::handleConfigCmd() {
	int i;
	int newValue;

	//Logger::debug("Cmd size: %i", ptrBuffer);
	if (ptrBuffer < 6)
		return; //4 digit command, =, value is at least 6 characters
	cmdBuffer[ptrBuffer] = 0; //make sure to null terminate
	String cmdString = String();
	unsigned char whichEntry = '0';
	i = 0;

	while (cmdBuffer[i] != '=' && i < ptrBuffer) {
	 cmdString.concat(String(cmdBuffer[i++]));
	}
	i++; //skip the =
	if (i >= ptrBuffer)
	{
		Logger::console("Command needs a value..ie TORQ=3000");
		Logger::console("");
		return; //or, we could use this to display the parameter instead of setting
	}

	// strtol() is able to parse also hex values (e.g. a string "0xCAFE"), useful for enable/disable by device id
	newValue = strtol((char *) (cmdBuffer + i), NULL, 0);

	cmdString.toUpperCase();
/*
	if (cmdString == String("TORQ") && motorConfig) {
		Logger::console("Setting Torque Limit to %i", newValue);
		motorConfig->torqueMax = newValue;
		motorController->saveConfiguration();
	} else if (cmdString == String("RPM") && motorConfig) {
		Logger::console("Setting RPM Limit to %i", newValue);
		motorConfig->speedMax = newValue;
		motorController->saveConfiguration();
	} else if (cmdString == String("REVLIM") && motorConfig) {
		Logger::console("Setting Reverse Limit to %i", newValue);
		motorConfig->reversePercent = newValue;
		motorController->saveConfiguration();
	} else if (cmdString == String("TPOT") && acceleratorConfig) {
		Logger::console("Setting # of Throttle Pots to %i", newValue);
		acceleratorConfig->numberPotMeters = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("TTYPE") && acceleratorConfig) {
		Logger::console("Setting Throttle Subtype to %i", newValue);
		acceleratorConfig->throttleSubType = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("T1ADC") && acceleratorConfig) {
		Logger::console("Setting Throttle1 ADC pin to %i", newValue);
		acceleratorConfig->AdcPin1 = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("T1MN") && acceleratorConfig) {
		Logger::console("Setting Throttle1 Min to %i", newValue);
		acceleratorConfig->minimumLevel1 = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("T1MX") && acceleratorConfig) {
		Logger::console("Setting Throttle1 Max to %i", newValue);
		acceleratorConfig->maximumLevel1 = newValue;
		accelerator->saveConfiguration();
	}
	else if (cmdString == String("T2ADC") && acceleratorConfig) {
		Logger::console("Setting Throttle2 ADC pin to %i", newValue);
		acceleratorConfig->AdcPin2 = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("T2MN") && acceleratorConfig) {
		Logger::console("Setting Throttle2 Min to %i", newValue);
		acceleratorConfig->minimumLevel2 = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("T2MX") && acceleratorConfig) {
		Logger::console("Setting Throttle2 Max to %i", newValue);
		acceleratorConfig->maximumLevel2 = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("TRGNMAX") && acceleratorConfig) {
		Logger::console("Setting Throttle Regen maximum to %i", newValue);
		acceleratorConfig->positionRegenMaximum = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("TRGNMIN") && acceleratorConfig) {
		Logger::console("Setting Throttle Regen minimum to %i", newValue);
		acceleratorConfig->positionRegenMinimum = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("TFWD") && acceleratorConfig) {
		Logger::console("Setting Throttle Forward Start to %i", newValue);
		acceleratorConfig->positionForwardMotionStart = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("TMAP") && acceleratorConfig) {
		Logger::console("Setting Throttle MAP Point to %i", newValue);
		acceleratorConfig->positionHalfPower = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("TMINRN") && acceleratorConfig) {
		Logger::console("Setting Throttle Regen Minimum Strength to %i", newValue);
		acceleratorConfig->minimumRegen = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("TMAXRN") && acceleratorConfig) {
		Logger::console("Setting Throttle Regen Maximum Strength to %i", newValue);
		acceleratorConfig->maximumRegen = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("TCREEP") && acceleratorConfig) {
		Logger::console("Setting Throttle Creep Strength to %i", newValue);
		acceleratorConfig->creep = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("BMAXR") && brakeConfig) {
		Logger::console("Setting Max Brake Regen to %i", newValue);
		brakeConfig->maximumRegen = newValue;
		brake->saveConfiguration();
	} else if (cmdString == String("BMINR") && brakeConfig) {
		Logger::console("Setting Min Brake Regen to %i", newValue);
		brakeConfig->minimumRegen = newValue;
		brake->saveConfiguration();
	}
	else if (cmdString == String("B1ADC") && acceleratorConfig) {
		Logger::console("Setting Brake ADC pin to %i", newValue);
		brakeConfig->AdcPin1 = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("B1MX") && brakeConfig) {
		Logger::console("Setting Brake Max to %i", newValue);
		brakeConfig->maximumLevel1 = newValue;
		brake->saveConfiguration();
	} else if (cmdString == String("B1MN") && brakeConfig) {
		Logger::console("Setting Brake Min to %i", newValue);
		brakeConfig->minimumLevel1 = newValue;
		brake->saveConfiguration();
	} else if (cmdString == String("PREC") && motorConfig) {
		Logger::console("Setting Precharge Capacitance to %i", newValue);
		motorConfig->kilowattHrs = newValue;
		motorController->saveConfiguration();
	} else if (cmdString == String("PREDELAY") && motorConfig) {
		Logger::console("Setting Precharge Time Delay to %i milliseconds", newValue);
		motorConfig->prechargeR = newValue;
		motorController->saveConfiguration();
	} else if (cmdString == String("NOMV") && motorConfig) {
		Logger::console("Setting fully charged voltage to %d vdc", newValue);
		motorConfig->nominalVolt = newValue * 10;
		motorController->saveConfiguration();
        } else if (cmdString == String("BRAKELT") && motorConfig) {
		motorConfig->brakeLight = newValue;
		motorController->saveConfiguration();
		Logger::console("Brake light output set to DOUT%i.",newValue);
 } else if (cmdString == String("REVLT") && motorConfig) {
		motorConfig->revLight = newValue;
		motorController->saveConfiguration();
		Logger::console("Reverse light output set to DOUT%i.",newValue);
 } else if (cmdString == String("ENABLEIN") && motorConfig) {
		motorConfig->enableIn = newValue;
		motorController->saveConfiguration();
		Logger::console("Motor Enable input set to DIN%i.",newValue);
 } else if (cmdString == String("REVIN") && motorConfig) {
		motorConfig->reverseIn = newValue;
		motorController->saveConfiguration();
		Logger::console("Motor Reverse input set to DIN%i.",newValue);
	} else if (cmdString == String("MRELAY") && motorConfig) {
		Logger::console("Setting Main Contactor relay output to DOUT%i", newValue);
		motorConfig->mainContactorRelay = newValue;
		motorController->saveConfiguration();
	} else if (cmdString == String("PRELAY") && motorConfig) {
		Logger::console("Setting Precharge Relay output to DOUT%i", newValue);
		motorConfig->prechargeRelay = newValue;
		motorController->saveConfiguration();
	} else if (cmdString == String("ENABLE")) {
		if (PrefHandler::setDeviceStatus(newValue, true)) {
			sysPrefs->forceCacheWrite(); //just in case someone takes us literally and power cycles quickly
			Logger::console("Successfully enabled device.(%X, %d) Power cycle to activate.", newValue, newValue);
		}
		else {
			Logger::console("Invalid device ID (%X, %d)", newValue, newValue);
		}
	} else if (cmdString == String("DISABLE")) {
		if (PrefHandler::setDeviceStatus(newValue, false)) {
			sysPrefs->forceCacheWrite(); //just in case someone takes us literally and power cycles quickly
			Logger::console("Successfully disabled device. Power cycle to deactivate.");
		}
		else {
			Logger::console("Invalid device ID (%X, %d)", newValue, newValue);
		}
	} else if (cmdString == String("SYSTYPE")) {
		if (newValue < 5 && newValue > 0) {
			sysPrefs->write(EESYS_SYSTEM_TYPE, (uint8_t)(newValue));
			sysPrefs->saveChecksum();
			sysPrefs->forceCacheWrite(); //just in case someone takes us literally and power cycles quickly
			Logger::console("System type updated. Power cycle to apply.");
		}
		else Logger::console("Invalid system type. Please enter a value 1 - 4");

       
	} else if (cmdString == String("LOGLEVEL")) {
		switch (newValue) {
		case 0:
			Logger::setLoglevel(Logger::Debug);
			Logger::console("setting loglevel to 'debug'");
			break;
		case 1:
			Logger::setLoglevel(Logger::Info);
			Logger::console("setting loglevel to 'info'");
			break;
		case 2:
			Logger::console("setting loglevel to 'warning'");
			Logger::setLoglevel(Logger::Warn);
			break;
		case 3:
			Logger::console("setting loglevel to 'error'");
			Logger::setLoglevel(Logger::Error);
			break;
		case 4:
			Logger::console("setting loglevel to 'off'");
			Logger::setLoglevel(Logger::Off);
			break;
		}
		sysPrefs->write(EESYS_LOG_LEVEL, (uint8_t)newValue);
		sysPrefs->saveChecksum();

	} else if (cmdString == String("WIREACH")) {
		DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)(cmdBuffer + i));
		Logger::info("sent \"AT+i%s\" to WiReach wireless LAN device", (cmdBuffer + i));
                DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"DOWN");	
		updateWifi = false;
	} else if (cmdString == String("SSID")) {
		String cmdString = String();
    	        cmdString.concat("WLSI");
   		cmdString.concat('=');
		cmdString.concat((char *)(cmdBuffer + i));
                Logger::info("Sent \"%s\" to WiReach wireless LAN device", (cmdString.c_str()));
       		DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)cmdString.c_str());
                DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"DOWN");	
		updateWifi = false;
        } else if (cmdString == String("IP")) {
		String cmdString = String();
    	        cmdString.concat("DIP");
   		cmdString.concat('=');
		cmdString.concat((char *)(cmdBuffer + i));
                Logger::info("Sent \"%s\" to WiReach wireless LAN device", (cmdString.c_str()));
       		DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)cmdString.c_str());
                DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"DOWN");	
		updateWifi = false;
 } else if (cmdString == String("CHANNEL")) {
		String cmdString = String();
    	        cmdString.concat("WLCH");
   		cmdString.concat('=');
		cmdString.concat((char *)(cmdBuffer + i));
                Logger::info("Sent \"%s\" to WiReach wireless LAN device", (cmdString.c_str()));
       		DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)cmdString.c_str());
                DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"DOWN");	
		updateWifi = false;
	} else if (cmdString == String("SECURITY")) {
		String cmdString = String();
    	        cmdString.concat("WLPP");
   		cmdString.concat('=');
		cmdString.concat((char *)(cmdBuffer + i));
                Logger::info("Sent \"%s\" to WiReach wireless LAN device", (cmdString.c_str()));
       		DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)cmdString.c_str());
                DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"DOWN");	
		updateWifi = false;
	
   } else if (cmdString == String("PWD")) {
		String cmdString = String();
    	        cmdString.concat("WPWD");
   		cmdString.concat('=');
		cmdString.concat((char *)(cmdBuffer + i));
                Logger::info("Sent \"%s\" to WiReach wireless LAN device", (cmdString.c_str()));
       		DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)cmdString.c_str());
                DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"DOWN");	
		updateWifi = false;
	
	} else if (cmdString == String("COOLFAN") && motorConfig) {		
		Logger::console("Cooling fan output updated to: %i", newValue);
        motorConfig->coolFan = newValue;
		motorController->saveConfiguration();
	} else if (cmdString == String("COOLON")&& motorConfig) {
		if (newValue <= 200 && newValue >= 0) {
			Logger::console("Cooling fan ON temperature updated to: %i degrees", newValue);
			motorConfig->coolOn = newValue;
			motorController->saveConfiguration();
		}
		else Logger::console("Invalid cooling ON temperature. Please enter a value 0 - 200F");
	} else if (cmdString == String("COOLOFF")&& motorConfig) {
		if (newValue <= 200 && newValue >= 0) {
			Logger::console("Cooling fan OFF temperature updated to: %i degrees", newValue);
			motorConfig->coolOff = newValue;
			motorController->saveConfiguration();
	    }
		else Logger::console("Invalid cooling OFF temperature. Please enter a value 0 - 200F");
	} else if (cmdString == String("OUTPUT") && newValue<8) {
                int outie = getOutput(newValue);
                Logger::console("DOUT%d,  STATE: %d",newValue, outie);
                if(outie)
                  {
                    setOutput(newValue,0);
                    motorController->statusBitfield1 &= ~(1 << newValue);//Clear
                  }
                   else
                     {
                       setOutput(newValue,1);
                        motorController->statusBitfield1 |=1 << newValue;//setbit to Turn on annunciator
		      }
                  
             
        Logger::console("DOUT0:%d, DOUT1:%d, DOUT2:%d, DOUT3:%d, DOUT4:%d, DOUT5:%d, DOUT6:%d, DOUT7:%d", getOutput(0), getOutput(1), getOutput(2), getOutput(3), getOutput(4), getOutput(5), getOutput(6), getOutput(7));
	} else if (cmdString == String("NUKE")) {
		if (newValue == 1) 
		{   //write zero to the checksum location of every device in the table.
			//Logger::console("Start of EEPROM Nuke");
			uint8_t zeroVal = 0;
			for (int j = 0; j < 64; j++) 
			{
				memCache->Write(EE_DEVICES_BASE + (EE_DEVICE_SIZE * j), zeroVal);
				memCache->FlushAllPages();
			}			
			Logger::console("Device settings have been nuked. Reboot to reload default settings");
		}
	} else {
		Logger::console("Unknown command");
		updateWifi = false;
	}
	// send updates to ichip wifi
	if (updateWifi)
		DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_CONFIG_CHANGE, NULL);
		*/
}

void SerialConsole::handleShortCmd() {
	uint8_t val;

	switch (cmdBuffer[0]) {
	case 'h':
	case '?':
	case 'H':
		printMenu();
		break;
	case 'E':
		Logger::console("Reading System EEPROM values");
		for (int i = 0; i < 256; i++) {
			//memCache->Read(EE_SYSTEM_START + i, &val);
			//Logger::console("%d: %d", i, val);
		}
		break;
	case 'K': //set all outputs high
		for (int tout = 0; tout < NUM_OUTPUT; tout++) setOutput(tout, true);
		Logger::console("all outputs: ON");
		break;
	case 'J': //set the four outputs low
		for (int tout = 0; tout < NUM_OUTPUT; tout++) setOutput(tout, false);
		Logger::console("all outputs: OFF");
		break;        
	case 'X':
		setup(); //this is probably a bad idea. Do not do this while connected to anything you care about - only for debugging in safety!
		break;
	}
}
