// *******************************************************************************************************************************
// *******************************************************************************************************************************
//
//		Name:		driver_arduino_serial_in.cpp
//		Purpose:	Input Driver, Arduino Serial port.
//		Created:	20th October 2015
//		Author:		Paul Robson (paul@robsons.org.uk)
//
// *******************************************************************************************************************************
// *******************************************************************************************************************************

#include <Arduino.h>
#include "driver_common.h"

static BYTE8 isInitialised = 0;

// *******************************************************************************************************************************
//												Arduino Serial In
// *******************************************************************************************************************************

DRVPARAM DRVRead(BYTE8 command,DRVPARAM data) {
	if (isInitialised == 0) {
		Serial.begin(9600);
		isInitialised = 1;
	}
	DRVPARAM retVal = 0x00;
	switch(command) {
		case DRA1_KEYBOARD:
			if (Serial.available()) {
				retVal = Serial.read();
			}
			break;
		case DRA1_ISRESET:
			break;
	}
	return retVal;
}