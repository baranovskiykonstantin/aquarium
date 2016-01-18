/* Name: ds18b20.h
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2015 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 * This Revision: 1
 */

#ifndef __DS18B20_H_INCLUDED__
#define __DS18B20_H_INCLUDED__

#include <avr/io.h>

#define DS18B20_ERR -111.11

//setup connection
#define DS18B20_PORT PORTD
#define DS18B20_DDR DDRD
#define DS18B20_PIN PIND
#define DS18B20_DQ PD3

//commands
#define DS18B20_CMD_CONVERTTEMP 0x44
#define DS18B20_CMD_RSCRATCHPAD 0xbe
#define DS18B20_CMD_WSCRATCHPAD 0x4e
#define DS18B20_CMD_CPYSCRATCHPAD 0x48
#define DS18B20_CMD_RECEEPROM 0xb8
#define DS18B20_CMD_RPWRSUPPLY 0xb4
#define DS18B20_CMD_SEARCHROM 0xf0
#define DS18B20_CMD_READROM 0x33
#define DS18B20_CMD_MATCHROM 0x55
#define DS18B20_CMD_SKIPROM 0xcc
#define DS18B20_CMD_ALARMSEARCH 0xec

//conversion resolutions
#define DS18B20_RES_09 0x1f
#define DS18B20_RES_10 0x3f
#define DS18B20_RES_11 0x5f
#define DS18B20_RES_12 0x7f
#define DS18B20_RES DS18B20_RES_09

//scratchpad
#define SCRATCHPAD_SIZE 9
#define SCRATCHPAD_TEMP_L 0
#define SCRATCHPAD_TEMP_H 1
#define SCRATCHPAD_USER_TH 2
#define SCRATCHPAD_USER_TL 3
#define SCRATCHPAD_CONF 4
/* 5-7 reserved */
#define SCRATCHPAD_CRC 8

//functions
extern double ds18b20_gettemp();

#endif /* __DS18B20_H_INCLUDED__ */
