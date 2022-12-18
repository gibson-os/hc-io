/*
 * I2C Slave.c
 *
 * Created: 04.04.2017 22:42:10
 * Author : ich
 */ 

#include <avr/io.h>
//#include <avr/wdt.h> 
#include "hcI2cSlave.h"
#include "io.h"

int main(void)
{
	//wdt_enable(WDTO_1S);
	hcInitI2C();
	ioInit();
	
	/*hcI2cReadBuffer[0] = 3;
	hcI2cReadBuffer[2] = IO_DIRECTION_INPUT + 16;
	ioSetData();
	
	hcI2cReadBuffer[0] = 9;
	hcI2cReadBuffer[2] = IO_DIRECTION_OUTPUT;
	ioSetData();

	hcI2cReadBuffer[0] = IO_COMMAND_STATUS_IN_EEPROM;
	hcI2cReadBuffer[2] = 255;
	hcI2cReadBuffer[3] = 255;
	ioSetData();
	
	hcI2cReadBuffer[0] = IO_COMMAND_ADD_DC;
	hcI2cReadBuffer[2] = 255;
	hcI2cReadBuffer[3] = 255;
	hcI2cReadBuffer[4] = 3;
	hcI2cReadBuffer[5] = 9;//9;
	hcI2cReadBuffer[6] = 7;//4
	hcI2cReadBuffer[7] = 0;
	ioSetData();
	
	hcI2cReadBuffer[0] = IO_COMMAND_ADD_DC;
	hcI2cReadBuffer[2] = 255;
	hcI2cReadBuffer[3] = 255;
	hcI2cReadBuffer[4] = 3;
	hcI2cReadBuffer[5] = 137;//9;
	hcI2cReadBuffer[6] = 7;//4
	hcI2cReadBuffer[7] = 0;
	ioSetData();
	
	hcI2cReadBuffer[0] = IO_COMMAND_GET_DC;
	hcI2cReadBuffer[2] = 3;
	hcI2cReadBuffer[3] = 0;
	ioSetData();
	
	hcI2cReadBuffer[0] = IO_COMMAND_GET_DC;
	ioGetData();*/
	
	/*hcI2cReadBuffer[0] = IO_COMMAND_ADD_DC;
	hcI2cReadBuffer[2] = 255;
	hcI2cReadBuffer[3] = 255;
	hcI2cReadBuffer[4] = 3;
	hcI2cReadBuffer[5] = 10;
	hcI2cReadBuffer[6] = 0;
	hcI2cReadBuffer[7] = 0;
	ioSetData();
	
	hcI2cReadBuffer[0] = IO_COMMAND_ADD_DC;
	hcI2cReadBuffer[2] = 255;
	hcI2cReadBuffer[3] = 255;
	hcI2cReadBuffer[4] = 7;
	hcI2cReadBuffer[5] = 7;
	hcI2cReadBuffer[6] = 15;
	hcI2cReadBuffer[7] = 31;
	ioSetData();
	
	hcI2cReadBuffer[0] = IO_COMMAND_ADD_DC;
	hcI2cReadBuffer[2] = 255;
	hcI2cReadBuffer[3] = 255;
	hcI2cReadBuffer[4] = 3;
	hcI2cReadBuffer[5] = 7;
	hcI2cReadBuffer[6] = 15;
	hcI2cReadBuffer[7] = 31;
	ioSetData();
	
	hcI2cReadBuffer[0] = IO_COMMAND_ADD_DC;
	hcI2cReadBuffer[2] = 255;
	hcI2cReadBuffer[3] = 255;
	hcI2cReadBuffer[4] = 3;
	hcI2cReadBuffer[5] = 7;
	hcI2cReadBuffer[6] = 15;
	hcI2cReadBuffer[7] = 31;
	ioSetData();
	
	hcI2cReadBuffer[0] = IO_COMMAND_SET_DC;
	hcI2cReadBuffer[2] = 255;
	hcI2cReadBuffer[3] = 255;
	hcI2cReadBuffer[4] = 3;
	hcI2cReadBuffer[5] = 1;
	hcI2cReadBuffer[6] = 8;
	hcI2cReadBuffer[7] = 16;
	hcI2cReadBuffer[8] = 32;
	ioSetData();
	
	hcI2cReadBuffer[0] = IO_COMMAND_DELETE_DC;
	hcI2cReadBuffer[2] = 255;
	hcI2cReadBuffer[3] = 255;
	hcI2cReadBuffer[4] = 7;
	hcI2cReadBuffer[5] = 0;
	ioSetData();
	
	hcI2cReadBuffer[0] = IO_COMMAND_DELETE_DC;
	hcI2cReadBuffer[2] = 255;
	hcI2cReadBuffer[3] = 255;
	hcI2cReadBuffer[4] = 3;
	hcI2cReadBuffer[5] = 1;
	ioSetData();
	
	hcI2cReadBuffer[0] = IO_COMMAND_RESET_DC;
	hcI2cReadBuffer[2] = 255;
	hcI2cReadBuffer[3] = 255;
	hcI2cReadBuffer[4] = 3;
	ioSetData();
	
	hcI2cReadBuffer[0] = IO_COMMAND_DEFRAG_DC;
	hcI2cReadBuffer[2] = 255;
	hcI2cReadBuffer[3] = 255;
	ioSetData();*/
	
	while(1)
	{
		//hcI2cInterruptCount++;
		ioReadInputs();
		//hcI2cInterruptCheck();
	}
}

uint8_t getConfiguration()
{
	return ioGetConfiguration();
}

uint8_t getStatus()
{
	return ioGetStatus();
}

uint8_t getData()
{
	return ioGetData();
}

void setData()
{
	ioSetData();
}

uint8_t getChangedData()
{
	return ioGetChangedData();
}

void isrEveryCall()
{
	ioIsrEveryCall();
}

void isrEveryStepReset()
{
	ioIsrEveryStepReset();
}