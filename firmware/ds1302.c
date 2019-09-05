/* Name: ds1302.c
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2017-2019 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 */

#include <util/delay.h>

#include "ds1302.h"

__attribute__((noinline)) static uint8_t bin8_to_bcd(uint8_t bin8)
{
    uint8_t bcd;

    bcd = (bin8 / 10) << 4;
    bcd |= bin8 % 10;

    return bcd;
}

__attribute__((noinline)) static uint8_t bcd_to_bin8(uint8_t bcd)
{
    uint8_t bin8;

    bin8 = (bcd >> 4) * 10;
    bin8 += bcd & 0b00001111;

    return bin8;
}

static void write_byte(uint8_t byte)
{
    uint8_t i;

    DS1302_SCLK_CLR;
    DS1302_CE_SET;
    DS1302_IO_AS_OUT;
    for (i = 0; i < 8; i++)
    {
        DS1302_SCLK_CLR;
        _delay_us(1);
        if (byte & (1 << i))
        {
            DS1302_IO_SET;
        }
        else
        {
            DS1302_IO_CLR;
        }
        DS1302_SCLK_SET;
        _delay_us(1);
    }
}

static uint8_t read_byte(void)
{
    uint8_t byte = 0;
    uint8_t i;

    DS1302_IO_AS_IN;
    DS1302_IO_SET; // Enable pull-up res.
    for (i = 0; i < 8; i++)
    {
        DS1302_SCLK_CLR;
        if (DS1302_IO_GET)
        {
            byte |= (1 << i);
        }
        else
        {
            byte &= ~(1 << i);
        }
        _delay_us(1);
        DS1302_SCLK_SET;
        _delay_us(1);
    }

    return byte;
}

static void finish(void)
{
    DS1302_CE_CLR;
    DS1302_SCLK_CLR;
}

void ds1302_init(void)
{
    DS1302_CE_AS_OUT;
    DS1302_CE_CLR;
    DS1302_SCLK_AS_OUT;
    DS1302_SCLK_CLR;
    DS1302_IO_AS_IN;
}

void ds1302_read_datetime(datetime_t *datetime)
{
    // read seconds
    write_byte(0x81);
    datetime->sec = bcd_to_bin8(read_byte());
    finish();
    // read minutes
    write_byte(0x83);
    datetime->min = bcd_to_bin8(read_byte());
    finish();
    // read hour
    write_byte(0x85);
    datetime->hour = read_byte();
    finish();
    datetime->AMPM = (datetime->hour & 0b00100000);
    datetime->H12_24 = (datetime->hour & 0b10000000);
    if (datetime->H12_24 == H12)
    {
        datetime->hour = datetime->hour & 0b00011111;
    }
    else
    {
        datetime->hour = datetime->hour & 0b00111111;
    }
    datetime->hour = bcd_to_bin8(datetime->hour);
    // read day
    write_byte(0x87);
    datetime->day = bcd_to_bin8(read_byte());
    finish();
    // read month
    write_byte(0x89);
    datetime->month = bcd_to_bin8(read_byte());
    finish();
    // read weekday
    write_byte(0x8B);
    datetime->weekday=read_byte();
    finish();
    // read year
    write_byte(0x8D);
    datetime->year = bcd_to_bin8(read_byte());
    finish();
}

void ds1302_write_datetime(datetime_t *datetime)
{
    // disable write protection
    write_byte(0x8e);
    write_byte(0x00);
    finish();
    // set seconds
    write_byte(0x80);
    write_byte(bin8_to_bcd(datetime->sec));
    finish();
    // set minutes
    write_byte(0x82);
    write_byte(bin8_to_bcd(datetime->min));
    finish();
    // set hour
    write_byte(0x84);
    write_byte(bin8_to_bcd(datetime->hour) | datetime->AMPM | datetime->H12_24);
    finish();
    // set date
    write_byte(0x86);
    write_byte(bin8_to_bcd(datetime->day));
    finish();
    // set month
    write_byte(0x88);
    write_byte(bin8_to_bcd(datetime->month));
    finish();
    // set day(of week)
    write_byte(0x8A);
    write_byte(datetime->weekday);
    finish();
    // set year
    write_byte(0x8C);
    write_byte(bin8_to_bcd(datetime->year));
    finish();
}

uint8_t ds1302_read_byte_from_ram(uint8_t offset)
{
    uint8_t value;

    // set address
    write_byte(0xC1 + (2 * offset));
    // read value
    value = read_byte();
    finish();

    return value;
}

void ds1302_write_byte_to_ram(uint8_t value, uint8_t offset)
{
    // disable write protection
    write_byte(0x8e);
    write_byte(0x00);
    finish();
    // set address
    write_byte(0xC0 + (2 * offset));
    // write value
    write_byte(value);
    finish();
}

void ds1302_read_datetime_from_ram(datetime_t *datetime, uint8_t offset)
{
    datetime->sec = ds1302_read_byte_from_ram(offset++);
    datetime->min = ds1302_read_byte_from_ram(offset++);
    datetime->hour = ds1302_read_byte_from_ram(offset++);
    datetime->AMPM = ds1302_read_byte_from_ram(offset++);
    datetime->H12_24 = ds1302_read_byte_from_ram(offset++);
    datetime->weekday = ds1302_read_byte_from_ram(offset++);
    datetime->day = ds1302_read_byte_from_ram(offset++);
    datetime->month = ds1302_read_byte_from_ram(offset++);
    datetime->year = ds1302_read_byte_from_ram(offset++);
}

void ds1302_write_datetime_to_ram(datetime_t *datetime, uint8_t offset)
{
    ds1302_write_byte_to_ram(datetime->sec, offset++);
    ds1302_write_byte_to_ram(datetime->min, offset++);
    ds1302_write_byte_to_ram(datetime->hour, offset++);
    ds1302_write_byte_to_ram(datetime->AMPM, offset++);
    ds1302_write_byte_to_ram(datetime->H12_24, offset++);
    ds1302_write_byte_to_ram(datetime->weekday, offset++);
    ds1302_write_byte_to_ram(datetime->day, offset++);
    ds1302_write_byte_to_ram(datetime->month, offset++);
    ds1302_write_byte_to_ram(datetime->year, offset++);
}
