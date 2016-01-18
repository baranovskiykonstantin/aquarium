/* Name: ds1302.h
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2015 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 * This Revision: 1
 */

#ifndef __DS1302_H_INCLUDED__
#define __DS1302_H_INCLUDED__

#include <avr/io.h>
#include <util/delay.h>

#define DS1302_CE_AS_OUT DDRD |= (1 << PD5)
#define DS1302_CE_SET PORTD |= (1 << PD5)
#define DS1302_CE_CLR PORTD &= ~(1 << PD5)

#define DS1302_SCLK_AS_OUT DDRB |= (1 << PB6)
#define DS1302_SCLK_SET PORTB |= (1 << PB6)
#define DS1302_SCLK_CLR PORTB &= ~(1 << PB6)

#define DS1302_IO_AS_IN DDRB &= ~(1 << PB7)
#define DS1302_IO_AS_OUT DDRB |= (1 << PB7)
#define DS1302_IO_SET PORTB |= (1 << PB7)
#define DS1302_IO_CLR PORTB &= ~(1 << PB7)
#define DS1302_IO_GET (PINB & (1 <<PB7))

#define AM 0
#define PM 0b00100000

#define H12 0b10000000
#define H24 0

typedef struct
    {
    uint8_t     sec;
    uint8_t     min;
    uint8_t     hour;
    uint8_t     AMPM;
    uint8_t     H12_24;
    } time_t;

typedef struct
    {
    uint8_t     month;
    uint8_t     day;
    uint8_t     year;
    uint8_t     weekday;
    } date_t;

typedef struct
    {
    uint8_t     sec;
    uint8_t     min;
    uint8_t     hour;
    uint8_t     month;
    uint8_t     day;
    uint8_t     year;
    uint8_t     weekday;
    uint8_t     AMPM;
    uint8_t     H12_24;
    } datetime_t;

void ds1302_init();
datetime_t ds1302_read_datetime();
void ds1302_write_datetime(datetime_t);

#endif /* __DS1302_H_INCLUDED__ */
