/* Name: ds1302.h
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2017 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 * This Revision: 1
 */

#ifndef __DS1302_H_INCLUDED__
#define __DS1302_H_INCLUDED__

#include <avr/io.h>
#include <util/delay.h>

#include "datetime.h"

#define DS1302_CE_AS_OUT DDRB |= (1 << PB4)
#define DS1302_CE_SET PORTB |= (1 << PB4)
#define DS1302_CE_CLR PORTB &= ~(1 << PB4)

#define DS1302_SCLK_AS_OUT DDRB |= (1 << PB3)
#define DS1302_SCLK_SET PORTB |= (1 << PB3)
#define DS1302_SCLK_CLR PORTB &= ~(1 << PB3)

#define DS1302_IO_AS_IN DDRB &= ~(1 << PB5)
#define DS1302_IO_AS_OUT DDRB |= (1 << PB5)
#define DS1302_IO_SET PORTB |= (1 << PB5)
#define DS1302_IO_CLR PORTB &= ~(1 << PB5)
#define DS1302_IO_GET (PINB & (1 <<PB5))

/*
 * Initialize the peripherals to work with ds1302
 */
extern void ds1302_init(void);

/*
 * Read date and time
 */
extern void ds1302_read_datetime(datetime_t *datetime);

/*
 * Write date and time
 */
extern void ds1302_write_datetime(datetime_t *datetime);

/*
 * Read byte from RAM
 */
extern uint8_t ds1302_read_byte_from_ram(uint8_t offset);

/*
 * Write byte to RAM
 */
extern void ds1302_write_byte_to_ram(uint8_t offset, uint8_t value);

/*
 * Read date and time from RAM
 */
extern void ds1302_read_datetime_from_ram(uint8_t offset, datetime_t *datetime);

/*
 * Write date and time to RAM
 */
extern void ds1302_write_datetime_to_ram(uint8_t offset, datetime_t *datetime);

#endif /* __DS1302_H_INCLUDED__ */
