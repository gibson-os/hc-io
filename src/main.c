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
	hcInitI2C();
	ioInit();
	
	while(1)
	{
		ioReadInputs();
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