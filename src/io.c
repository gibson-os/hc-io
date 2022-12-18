/*
 * io.c
 *
 * Created: 17.12.2017 23:10:25
 *  Author: Meins
 */
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "io.h"
#include "hcI2cSlave.h"

#ifdef HC_USE_WATCHDOG
	#include <avr/wdt.h>
#endif

#define	bis(ADDRESS,BIT)	(ADDRESS & (1<<BIT))		// bit is set?
#define	bic(ADDRESS,BIT)	(!(ADDRESS & (1<<BIT)))		// bit is clear?
#define sbi(ADDRESS,BIT) 	((ADDRESS) |= (1<<(BIT)))	// set Bit
#define cbi(ADDRESS,BIT) 	((ADDRESS) &= ~(1<<(BIT)))	// clear Bit
#define ioGetEepromAddress(highByte, lowByte) ((highByte << 8) | lowByte)
#define ioGetInputCount(ADDRESS) ((ios[ADDRESS].pwm << 5) | ios[ADDRESS].blink)

struct ioStruct {
	unsigned direction:1;
	unsigned pullUp:1;
	unsigned pwm;
	unsigned blink:5;
	unsigned value:1;
	unsigned fadeIn;
	unsigned pwmCount;
	unsigned blinkCount:16;
	unsigned fadeInCount:12;
	unsigned blinkValue:1;
};

volatile struct ioStruct ios[IO_COUNT];
volatile uint8_t ioInputCount[IO_COUNT];
volatile uint8_t ioDirectConnectStatus = 1;
volatile uint8_t ioBufferPos = 0;
volatile uint8_t ioGetDirectConnectIo = 255;
volatile uint8_t ioGetDirectConnectNumber = 0;

volatile uint8_t *ioDdrs[] =  { &DDRD,  &DDRD,  &DDRD,  &DDRD,  &DDRC,  &DDRC,  &DDRC,  &DDRC,  &DDRD,  &DDRD,  &DDRD,  &DDRD,  &DDRB,  &DDRB,  &DDRB,  &DDRB  };
volatile uint8_t *ioPorts[] = { &PORTD, &PORTD, &PORTD, &PORTD, &PORTC, &PORTC, &PORTC, &PORTC, &PORTD, &PORTD, &PORTD, &PORTD, &PORTB, &PORTB, &PORTB, &PORTB };
volatile uint8_t *ioPins[] =  { &PIND,  &PIND,  &PIND,  &PIND,  &PINC,  &PINC,  &PINC,  &PINC,  &PIND,  &PIND,  &PIND,  &PIND,  &PINB,  &PINB,  &PINB,  &PINB  };
volatile uint8_t ioBits[] =   { 0,      1,      2,      3,      3,      2,      1,      0,      4,      5,      6,      7,      4,      3,      2,      1      };

void ioInit()
{
	for (uint8_t i = 0; i < IO_COUNT; i++) {
		ios[i].direction = IO_DIRECTION_INPUT;
		ios[i].pullUp = 1;
		ios[i].pwm = 0;
		ios[i].blink = 0;
		
		sbi(*ioPorts[i], ioBits[i]); // Set Pullup
		cbi(*ioDdrs[i], ioBits[i]); // Set Input
		
		ios[i].value = ((*ioPins[i] >> ioBits[i]) & 1);
	}
	
	hcI2cEepromPosition = IO_DC_EEPROM_CONFIG_LENGTH_ALL;
	
	for (uint16_t i = IO_DC_EEPROM_LENGTH-2; i >= hcI2cEepromPosition; i -= IO_DC_EEPROM_DIRECT_CONNECT_LENGTH) {
		if (hcI2cReadByteFromEeprom(i) != 255) {
			hcI2cEepromPosition = i+2;
			break;
		}
	}

	ioLoadStatusFromEeprom();
	hcI2cDataChangedLength = IO_DATA_LENGTH;
}

uint8_t ioLoadStatusFromEeprom()
{
	uint16_t address = 0;
	uint16_t dcAddress;
	uint8_t setByte;
	uint8_t pwmByte;
	uint8_t addressHigh;
	uint8_t addressLow;
	
	for (uint8_t i = 0; i < IO_COUNT; i++) {
		setByte = hcI2cReadByteFromEeprom(address++);
		pwmByte = hcI2cReadByteFromEeprom(address++);
		addressLow = hcI2cReadByteFromEeprom(address++);
		addressHigh = hcI2cReadByteFromEeprom(address++);
		
		if (addressHigh == 255) {
			return 0;
		}
		
		ioSetPort(setByte, pwmByte, i);
		
		#ifdef HC_USE_WATCHDOG
			wdt_reset();
		#endif
		
		if (ios[i].direction != IO_DIRECTION_INPUT) {
			continue;
		}
		
		dcAddress = ioGetEepromAddress(addressHigh, addressLow);
		ioExecuteDirectConnect(dcAddress, &i);
	}
	
	return 255;
}

void ioReadInputs()
{
	#ifdef HC_USE_WATCHDOG
		wdt_reset();
	#endif
	
	for (uint8_t i = 0; i < IO_COUNT; i++) {
		hcI2cInterruptCheck();
		
		if (ios[i].direction != IO_DIRECTION_INPUT) {
			continue;
		}
		
		if (ios[i].value != ((*ioPins[i] >> ioBits[i]) & 1)) {
			ios[i].value = ~ios[i].value;
			ioInputCount[i] = 1;
			hcI2cDataChangedLength = IO_DATA_LENGTH;
		}
			
		if (ioInputCount[i] && ioInputCount[i] >= ioGetInputCount(i)) {
			ioInputCount[i] = 0;
			ioStartExecuteDirectConnect(&i);
		}
	}
}

void ioStartExecuteDirectConnect(uint8_t *ioNumber)
{
	uint8_t address = *ioNumber*IO_DC_EEPROM_CONFIG_LENGTH+1;
	uint8_t addressLow = hcI2cReadByteFromEeprom(++address);
	uint8_t addressHigh = hcI2cReadByteFromEeprom(++address);
	
	if (addressHigh != 255) {
		address = ioGetEepromAddress(addressHigh, addressLow);
		ioExecuteDirectConnect(address, ioNumber);
	}
}

void ioExecuteDirectConnect(uint16_t eepromPos, uint8_t *ioNumber)
{
	if (eepromPos == 0) {
		return;
	}
		
	if (!ioDirectConnectStatus) {
		return;
	}
	
	hcI2cInterruptCheck();
	
	uint8_t ioByte = hcI2cReadByteFromEeprom(eepromPos++);
	
	if ((ioByte >> 7) == ios[*ioNumber].value) {
		uint8_t setIoNumber = (ioByte & 0b01111111);
		
		if (ios[setIoNumber].direction == IO_DIRECTION_OUTPUT) {
			uint8_t setByte = hcI2cReadByteFromEeprom(eepromPos);
			uint8_t pwmByte = hcI2cReadByteFromEeprom(eepromPos+1);
			
			ios[setIoNumber].value = (setByte >> HC_IO_VALUE_BIT);
			ios[setIoNumber].fadeIn = 0;
			
			if (((setByte >> HC_IO_ADD_BIT) & 1) == ((setByte >> HC_IO_SUB_BIT) & 1)) { // Absolute
				ios[setIoNumber].pwm = pwmByte;
				ios[setIoNumber].blink = (setByte >> HC_IO_BLINK_BIT);
				
				if (ios[setIoNumber].value && ios[setIoNumber].pwm) {
					ios[setIoNumber].fadeIn = ios[setIoNumber].pwm;
					ios[setIoNumber].pwm = 255;
					ios[setIoNumber].value = 0;
				}
			} else if ((setByte >> HC_IO_SUB_BIT) & 1) { // Sub
				ios[setIoNumber].pwm -= pwmByte;
				ios[setIoNumber].blink -= (setByte >> HC_IO_BLINK_BIT);
			} else { // Add
				ios[setIoNumber].pwm += pwmByte;
				ios[setIoNumber].blink += (setByte >> HC_IO_BLINK_BIT);
			}
		
			hcI2cDataChangedLength = IO_DATA_LENGTH;
		}
	}
	
	eepromPos += 2;
	
	#ifdef HC_USE_WATCHDOG
		wdt_reset();
	#endif
	
	uint8_t addressLowByte = hcI2cReadByteFromEeprom(eepromPos++);
	uint8_t addressHighByte = hcI2cReadByteFromEeprom(eepromPos++);
	eepromPos = ioGetEepromAddress(addressHighByte, addressLowByte);
		
	if (eepromPos < IO_DC_EEPROM_LENGTH) {
		ioExecuteDirectConnect(eepromPos, ioNumber);
	}
}

void ioDefragDirectConnect()
{
	if (hcI2cEepromPosition == IO_DC_EEPROM_CONFIG_LENGTH_ALL) {
		return;
	}
	
	uint8_t highByte;
	uint16_t i, j, k, address, setAddress;
	
	for (i = IO_DC_EEPROM_CONFIG_LENGTH_ALL; i < hcI2cEepromPosition; i += IO_DC_EEPROM_DIRECT_CONNECT_LENGTH) {
		if (hcI2cReadByteFromEeprom(i+4) != 255) {
			continue;
		}
		
		// Den letzten Befehl suchen
		for (j = hcI2cEepromPosition; j >= 0; j -= IO_DC_EEPROM_DIRECT_CONNECT_LENGTH) {
			#ifdef HC_USE_WATCHDOG
				wdt_reset();
			#endif
			
			if (j == i) {
				i = hcI2cEepromPosition;
				break;
			}
			
			if (hcI2cReadByteFromEeprom(j+2) == 255) {
				continue;
			}
			
			hcI2cUpdateByteInEeprom(i, hcI2cReadByteFromEeprom(j));
			hcI2cUpdateByteInEeprom(i+1, hcI2cReadByteFromEeprom(j+1));
			hcI2cUpdateByteInEeprom(i+2, hcI2cReadByteFromEeprom(j+2));
			hcI2cUpdateByteInEeprom(i+3, hcI2cReadByteFromEeprom(j+3));
			hcI2cUpdateByteInEeprom(i+4, hcI2cReadByteFromEeprom(j+4));
			
			hcI2cUpdateByteInEeprom(j, 255);
			hcI2cUpdateByteInEeprom(j+1, 255);
			hcI2cUpdateByteInEeprom(j+2, 255);
			hcI2cUpdateByteInEeprom(j+3, 255);
			hcI2cUpdateByteInEeprom(j+4, 255);
			
			hcI2cEepromPosition = j;
			
			for (k = 0; k < IO_DC_EEPROM_CONFIG_LENGTH_ALL; k += IO_DC_EEPROM_CONFIG_LENGTH) {
				#ifdef HC_USE_WATCHDOG
					wdt_reset();
				#endif
				
				address = k;
				setAddress = k;
				highByte = hcI2cReadByteFromEeprom(address+3);
				
				while (highByte != 255) {
					address = ioGetEepromAddress(highByte, hcI2cReadByteFromEeprom(address+2));
					
					if (!address) {
						break;
					}
				
					if (address == j) {
						hcI2cUpdateByteInEeprom(setAddress+2, i);
						hcI2cUpdateByteInEeprom(setAddress+3, highByte);
						k = IO_DC_EEPROM_CONFIG_LENGTH_ALL;
						break;
					}
					
					setAddress = address;
					highByte = hcI2cReadByteFromEeprom(address+3);
					
					#ifdef HC_USE_WATCHDOG
						wdt_reset();
					#endif
				}
				
			}
			
			break;
		}
	}
}

uint8_t ioGetConfiguration()
{
	hcI2cWriteBuffer[0] = IO_COUNT;
	return 1;
}
// @todo es muss ein Zeiger eingebaut werden
uint8_t ioGetStatus()
{
	ioBufferPos = 0;
	
	for (uint8_t i = 0; i < IO_COUNT; i++) {
		hcI2cWriteBuffer[ioBufferPos++] = (ios[i].blink << HC_IO_BLINK_BIT) | (ios[i].value << HC_IO_VALUE_BIT) | (ios[i].pullUp << HC_IO_PULL_UP_BIT) | ios[i].direction;
		hcI2cWriteBuffer[ioBufferPos++] = ios[i].pwm;
	}
	
	return ioBufferPos;
}

uint8_t ioGetChangedData()
{
	hcI2cDataChangedLength = 0;
	return ioGetStatus();
}

uint8_t ioGetData()
{
	uint16_t address;
	
	switch (hcI2cReadBuffer[0]) {
		case IO_COMMAND_GET_DC:
			if (ioGetDirectConnectIo == 255) {
				hcI2cWriteBuffer[3] = 1;
				return 4;
			}
		
			address = ioGetDirectConnectIo*IO_DC_EEPROM_CONFIG_LENGTH-1;
			ioGetDirectConnectIo = 255;
			
			for (uint8_t i = 0; i <= ioGetDirectConnectNumber; i++) {
				uint8_t addressLow = hcI2cReadByteFromEeprom(address+3);
				uint8_t addressHigh = hcI2cReadByteFromEeprom(address+4);
			
				if (addressHigh == 255) {
					ioGetDirectConnectNumber = 0;
					hcI2cWriteBuffer[3] = 2;
					return 4;
				}
			
				address = ioGetEepromAddress(addressHigh, addressLow);
				
				if (!address) {
					ioGetDirectConnectNumber = 0;
					hcI2cWriteBuffer[3] = 2;
					return 4;
				}
		
				#ifdef HC_USE_WATCHDOG
					wdt_reset();
				#endif
			}
		
			ioGetDirectConnectNumber = 0;
			hcI2cWriteBuffer[0] = hcI2cReadByteFromEeprom(address);
			hcI2cWriteBuffer[1] = hcI2cReadByteFromEeprom(address+1);
			hcI2cWriteBuffer[2] = hcI2cReadByteFromEeprom(address+2);
			hcI2cWriteBuffer[3] = 0;
			
			// @todo mal wird auf null mal auf 255 geprüft. Komisch!
			if (ioGetEepromAddress(hcI2cReadByteFromEeprom(address+4), hcI2cReadByteFromEeprom(address+3))) {
				hcI2cWriteBuffer[3] = 255;
			}
		
			return 4;
		case IO_COMMAND_STATUS_IN_EEPROM:
			hcI2cWriteBuffer[0] = ioLoadStatusFromEeprom();
			return 1;
		case IO_COMMAND_DC_STATUS:
			hcI2cWriteBuffer[0] = ioDirectConnectStatus;
			return 1;
		default:
			if (hcI2cReadBuffer[0] < IO_COUNT) {
				address = hcI2cReadBuffer[0];
				hcI2cWriteBuffer[0] = (ios[address].blink << HC_IO_BLINK_BIT) | (ios[address].value << HC_IO_VALUE_BIT) | (ios[address].pullUp << HC_IO_PULL_UP_BIT) | ios[address].direction;
				hcI2cWriteBuffer[1] = ios[address].pwm;
				return 2;
			}
			
			break;
	}
	
	return 0;
}

void ioSetData()
{
	uint8_t addressLow, addressHigh, dataByte = 2;
	uint16_t address = 0, setAddress;
	
	switch (hcI2cReadBuffer[0]) {
		case IO_COMMAND_ADD_DC:
			if (hcI2cCheckDeviceId()) {
				// Letzten Befehl des IOs finden
				address = hcI2cReadBuffer[4]*IO_DC_EEPROM_CONFIG_LENGTH-1;
				setAddress = address;
				addressHigh = hcI2cReadByteFromEeprom(address+4);
				addressLow = hcI2cReadByteFromEeprom(address+3);
		
				while (addressHigh != 255) {
					address = ioGetEepromAddress(addressHigh, addressLow);
				
					if (!address) {
						break;
					}
					
					addressHigh = hcI2cReadByteFromEeprom(address+4);
					addressLow = hcI2cReadByteFromEeprom(address+3);
				
					setAddress = address;
				
					#ifdef HC_USE_WATCHDOG
						wdt_reset();
					#endif
				}
		
				if (!hcI2cEepromPosition) {
					hcI2cEepromPosition = IO_DC_EEPROM_CONFIG_LENGTH_ALL;
				}
		
				if (hcI2cEepromPosition + IO_DC_EEPROM_DIRECT_CONNECT_LENGTH > IO_DC_EEPROM_LENGTH) {
					return;
				}
			
				hcI2cUpdateByteInEeprom(setAddress+3, hcI2cEepromPosition);
				hcI2cUpdateByteInEeprom(setAddress+4, (hcI2cEepromPosition >> 8));
		
				hcI2cWriteByteToEeprom(hcI2cEepromPosition++, hcI2cReadBuffer[5]);
				hcI2cWriteByteToEeprom(hcI2cEepromPosition++, hcI2cReadBuffer[6]);
				hcI2cWriteByteToEeprom(hcI2cEepromPosition++, hcI2cReadBuffer[7]);
				hcI2cWriteByteToEeprom(hcI2cEepromPosition++, 0);
				hcI2cWriteByteToEeprom(hcI2cEepromPosition++, 0);
			}
			
			break;
		case IO_COMMAND_SET_DC:
			if (hcI2cCheckDeviceId()) {
				address = hcI2cReadBuffer[4]*IO_DC_EEPROM_CONFIG_LENGTH-1;
		
				for (uint8_t i = 0; i <= hcI2cReadBuffer[5]; i++) {
					addressHigh = hcI2cReadByteFromEeprom(address+4);
			
					if (addressHigh == 255) {
						return;
					}
			
					addressLow = hcI2cReadByteFromEeprom(address+3);
					address = ioGetEepromAddress(addressHigh, addressLow);
				
					if (
						!address ||
						address > IO_DC_EEPROM_LENGTH
					) {
						return;
					}
				}
		
				hcI2cUpdateByteInEeprom(address++, hcI2cReadBuffer[6]);
				hcI2cUpdateByteInEeprom(address++, hcI2cReadBuffer[7]);
				hcI2cUpdateByteInEeprom(address++, hcI2cReadBuffer[8]);
			}
			break;
		case IO_COMMAND_DELETE_DC:
			if (hcI2cCheckDeviceId()) {
				setAddress = hcI2cReadBuffer[4]*IO_DC_EEPROM_CONFIG_LENGTH-1;
			
				for (uint8_t i = 0; i <= hcI2cReadBuffer[5]; i++) {
					address = setAddress;
					addressHigh = hcI2cReadByteFromEeprom(address+4);
			
					if (addressHigh == 255) {
						return;
					}
			
					addressLow = hcI2cReadByteFromEeprom(address+3);
					setAddress = ioGetEepromAddress(addressHigh, addressLow);
				
					if (
						!setAddress ||
						setAddress > IO_DC_EEPROM_LENGTH
					) {
						return;
					}
			
					#ifdef HC_USE_WATCHDOG
						wdt_reset();
					#endif
				}
			
				// Die Next Address muss in den vorherigen gesetzt werden
				hcI2cUpdateByteInEeprom(address+3, hcI2cReadByteFromEeprom(setAddress+3));
				hcI2cUpdateByteInEeprom(address+4, hcI2cReadByteFromEeprom(setAddress+4));
		
				hcI2cUpdateByteInEeprom(setAddress++, 0xFF);
				hcI2cUpdateByteInEeprom(setAddress++, 0xFF);
				hcI2cUpdateByteInEeprom(setAddress++, 0xFF);
				hcI2cUpdateByteInEeprom(setAddress++, 0xFF);
				hcI2cUpdateByteInEeprom(setAddress, 0xFF);
			}
			break;
		case IO_COMMAND_RESET_DC:
			if (hcI2cCheckDeviceId()) {
				address = hcI2cReadBuffer[4]*IO_DC_EEPROM_CONFIG_LENGTH-1;
				addressHigh = hcI2cReadByteFromEeprom(address+4);
				addressLow = hcI2cReadByteFromEeprom(address+3);
			
				hcI2cUpdateByteInEeprom(address+3, 0);
				hcI2cUpdateByteInEeprom(address+4, 0);
		
				while (addressHigh != 255) {
					address = ioGetEepromAddress(addressHigh, addressLow);
				
					if (
						!address ||
						address > IO_DC_EEPROM_LENGTH
					) {
						return;
					}
				
					addressHigh = hcI2cReadByteFromEeprom(address+4);
					addressLow = hcI2cReadByteFromEeprom(address+3);
				
					hcI2cUpdateByteInEeprom(address++, 0xFF);
					hcI2cUpdateByteInEeprom(address++, 0xFF);
					hcI2cUpdateByteInEeprom(address++, 0xFF);
					hcI2cUpdateByteInEeprom(address++, 0xFF);
					hcI2cUpdateByteInEeprom(address, 0xFF);
				
					#ifdef HC_USE_WATCHDOG
						wdt_reset();
					#endif
				}
			}
			
			break;
		case IO_COMMAND_GET_DC:
			ioGetDirectConnectIo = hcI2cReadBuffer[2];
			ioGetDirectConnectNumber = hcI2cReadBuffer[3];
			break;
		case IO_COMMAND_DEFRAG_DC:
			if (hcI2cCheckDeviceId()) {
				ioDefragDirectConnect();
			}
			break;
		case IO_COMMAND_STATUS_IN_EEPROM:
			if (hcI2cCheckDeviceId()) {
				address = 0;
				
				for (uint8_t i = 0; i < IO_COUNT; i++) {
					hcI2cUpdateByteInEeprom(address++, (ios[i].blink << 3) | (ios[i].value << 2) | (ios[i].pullUp << 1) | ios[i].direction);
					hcI2cUpdateByteInEeprom(address++, ios[i].pwm);
					
					if (hcI2cReadByteFromEeprom(address+2) == 255) {
						hcI2cUpdateByteInEeprom(address, 0);
						hcI2cUpdateByteInEeprom(address+1, 0);
					}
					
					address += 2;
					
					#ifdef HC_USE_WATCHDOG
						wdt_reset();
					#endif
				}
			}
			
			break;
		case IO_COMMAND_DC_STATUS:
			ioDirectConnectStatus = hcI2cReadBuffer[dataByte];
		case IO_COMMAND_SET_PORTS:
			for (address = 0; address < IO_COUNT; address++) {
				ioSetPort(hcI2cReadBuffer[dataByte], hcI2cReadBuffer[dataByte+1], address);
				dataByte += 2;
			}
			
			break;
		default: 
			ioSetPort(hcI2cReadBuffer[dataByte], hcI2cReadBuffer[dataByte+1], hcI2cReadBuffer[0]);
			break;
	}
}

void ioSetPort(uint8_t setByte, uint8_t pwmByte, uint8_t ioNumber)
{
	if (ioNumber >= IO_COUNT) {
		return;
	}
	
	if (ios[ioNumber].direction != (setByte & 1)) {
		if (setByte & 1) {
			cbi(*ioPorts[ioNumber], ioBits[ioNumber]);
			sbi(*ioDdrs[ioNumber], ioBits[ioNumber]);
		} else {
			ios[ioNumber].pwm = 0;
			ios[ioNumber].blink = 0;
			ios[ioNumber].value = 0;
			
			cbi(*ioDdrs[ioNumber], ioBits[ioNumber]);
			sbi(*ioPorts[ioNumber], ioBits[ioNumber]);
		}
		
		ios[ioNumber].direction = setByte;
	}
	
	ios[ioNumber].pwm = pwmByte;
	ios[ioNumber].blink = (setByte >> HC_IO_BLINK_BIT);
	ios[ioNumber].fadeIn = 0;
	
	if (ios[ioNumber].direction == IO_DIRECTION_OUTPUT) {
		ios[ioNumber].value = (setByte >> HC_IO_VALUE_BIT);
		ios[ioNumber].pullUp = 0;
		
		if (ios[ioNumber].value && ios[ioNumber].pwm) {
			ios[ioNumber].fadeIn = ios[ioNumber].pwm;
			ios[ioNumber].pwm = 255;
			ios[ioNumber].value = 0;
		}
	} else {
		if ((setByte >> HC_IO_PULL_UP_BIT) & 1) {
			sbi(*ioPorts[ioNumber], ioBits[ioNumber]);
		} else {
			cbi(*ioDdrs[ioNumber], ioBits[ioNumber]);
		}
	
		ios[ioNumber].pullUp = (setByte >> HC_IO_PULL_UP_BIT);
	}
}

void ioIsrEveryCall()
{	
	if (hcI2cInterruptCount == 0) {
		return;
	}
	
	uint8_t interruptCount = hcI2cInterruptCount;
	
	for (uint8_t i = 0; i < IO_COUNT; i++) {
		if (ios[i].direction != IO_DIRECTION_OUTPUT) {
			if (ioInputCount[i] > 0) {
				ioInputCount[i] += interruptCount;
			}
			
			continue;
		}
		
		if (ios[i].pwm) {
			ios[i].pwmCount += interruptCount;
		}
		
		if (ios[i].blink) {
			ios[i].blinkCount += interruptCount;
			
			if ((ios[i].blinkCount>>8) >= (ios[i].blink + ios[i].blink + ios[i].blink + ios[i].blink + ios[i].blink + ios[i].blink + ios[i].blink + ios[i].blink)) {
				ios[i].blinkCount = 0;
				ios[i].blinkValue++;
			}
		}
		
		if (
			ios[i].value ||
			(ios[i].pwm && ios[i].pwmCount >= ios[i].pwm)
		) {
			ios[i].pwmCount = 0;
			
			if (ios[i].blink == 0 || ios[i].blinkValue) {
				sbi(*ioPorts[i], ioBits[i]);
			} else {
				cbi(*ioPorts[i], ioBits[i]);
			}
		} else {
			cbi(*ioPorts[i], ioBits[i]);
		}
	
		if (ios[i].fadeIn) {
			ios[i].fadeInCount += interruptCount;
			
			if ((ios[i].fadeInCount>>4) >= ios[i].fadeIn) {
				ios[i].fadeInCount = 0;
				ios[i].pwm--;
				ios[i].pwmCount = 0;
				
				if (ios[i].pwm == 0) {
					ios[i].fadeIn = 0;
					ios[i].value = 1;
					hcI2cDataChangedLength = IO_DATA_LENGTH;
				}
			}
		}
	}
}