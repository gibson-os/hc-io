/*
 * io.h
 *
 * Created: 17.12.2017 22:54:22
 *  Author: Meins
 */ 


#ifndef IO_H_
#define IO_H_

void ioInit();
void ioReadInputs();
uint8_t ioLoadStatusFromEeprom();
uint8_t ioGetConfiguration();
uint8_t ioGetStatus();
uint8_t ioGetChangedData();
uint8_t ioGetData();
void ioSetData();
void ioSetPort(uint8_t setByte, uint8_t pwmByte, uint8_t ioNumber);
void ioIsrEveryCall();
void ioIsrEveryStepReset();
void ioExecuteDirectConnect(uint16_t eepromPos, uint8_t *ioNumber);
void ioStartExecuteDirectConnect(uint8_t *ioNumber);
void ioDefragDirectConnect();

// Muss mindestens 4 oder ein vielfaches von 4 sein
#define IO_COUNT 16
#define IO_DATA_LENGTH IO_COUNT*2

#define IO_DC_EEPROM_LENGTH (((hcI2cEepromDataSize) >> 2) << 2)
#define IO_DC_EEPROM_CONFIG_LENGTH 4
#define IO_DC_EEPROM_CONFIG_LENGTH_ALL IO_COUNT*IO_DC_EEPROM_CONFIG_LENGTH
#define IO_DC_EEPROM_DIRECT_CONNECT_LENGTH 5

#define IO_DIRECTION_INPUT 0
#define IO_DIRECTION_OUTPUT 1

#define IO_COMMAND_SET_PORTS 128
#define IO_COMMAND_ADD_DC 129
#define IO_COMMAND_SET_DC 130
#define IO_COMMAND_DELETE_DC 131
#define IO_COMMAND_RESET_DC 132
#define IO_COMMAND_GET_DC 133
#define IO_COMMAND_DEFRAG_DC 134
#define IO_COMMAND_STATUS_IN_EEPROM 135
#define IO_COMMAND_DC_STATUS 136
#define HC_IO_ADD_BIT 1
#define HC_IO_SUB_BIT 0

#define HC_IO_DIRECTION_BIT 0
#define HC_IO_PULL_UP_BIT 1
#define HC_IO_VALUE_BIT 2
#define HC_IO_BLINK_BIT 3

#endif /* IO_H_ */