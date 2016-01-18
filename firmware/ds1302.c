/* Name: ds1302.c
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2015 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 * This Revision: 1
 */

#include "ds1302.h"

uint8_t ds1302_bin8_to_bcd (uint8_t data)
{
    uint8_t hpart;
    uint8_t lpart;

    hpart = data / 10;
    lpart = data % 10;

    return((hpart << 4) | lpart);
}

uint8_t ds1302_bcd_to_bin8(uint8_t data)
{
    uint8_t hpart;
    uint8_t lpart;

    hpart = ((data >> 4) & 0b00000111);
    lpart = data & 0x0F;
    data = hpart*10 + lpart;
    return data;
}

void ds1302_write(uint8_t cmd)
{
    uint8_t i;
    DS1302_SCLK_CLR;
    DS1302_CE_SET;
    DS1302_IO_AS_OUT;
    for (i=0; i<8; i++)
    {
        DS1302_SCLK_CLR;
        _delay_us(1);
        if (cmd & (1 << i))
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

void ds1302_end_write_data()
{
    DS1302_CE_CLR;
    DS1302_SCLK_CLR;
}

uint8_t ds1302_read()
{
    uint8_t readbyte;
    uint8_t i;
    readbyte = 0;
    DS1302_IO_AS_IN;
    DS1302_IO_SET; // Enable pull-up res.
    for (i=0; i<8; i++)
    {
        DS1302_SCLK_CLR;
        if (DS1302_IO_GET)
        {
            readbyte |= (1 << i);
        }
        else
        {
            readbyte &= ~(1 << i);
        }
        _delay_us(1);
        DS1302_SCLK_SET;
        _delay_us(1);
    }
    DS1302_CE_CLR;
    DS1302_SCLK_CLR;
    return readbyte;
}

void ds1302_init()
{
    DS1302_CE_AS_OUT;
    DS1302_CE_CLR;
    DS1302_SCLK_AS_OUT;
    DS1302_SCLK_CLR;
    DS1302_IO_AS_IN;
}

datetime_t ds1302_read_datetime()
{
    datetime_t datetime;
    // read seconds
    ds1302_write(0x81);
    datetime.sec = ds1302_bcd_to_bin8(ds1302_read());
    // read minutes
    ds1302_write(0x83);
    datetime.min = ds1302_bcd_to_bin8(ds1302_read());
    // read hour
    ds1302_write(0x85);
    datetime.hour = ds1302_read();
    datetime.AMPM = (datetime.hour & 0b00100000);
    datetime.H12_24 = (datetime.hour & 0b10000000);
    if (datetime.H12_24 == H12) {
        datetime.hour = datetime.hour & 0b00011111;
    }
    else {
        datetime.hour = datetime.hour & 0b00111111;
    }
    datetime.hour = ds1302_bcd_to_bin8(datetime.hour);
    // read day
    ds1302_write(0x87);
    datetime.day = ds1302_bcd_to_bin8(ds1302_read());
    // read month
    ds1302_write(0x89);
    datetime.month = ds1302_bcd_to_bin8(ds1302_read());
    // read weekday
    ds1302_write(0x8B);
    datetime.weekday=ds1302_read();
    // read year
    ds1302_write(0x8D);
    datetime.year = ds1302_bcd_to_bin8(ds1302_read());
    return datetime;
}

void ds1302_write_datetime(datetime_t datetime)
{
    uint8_t tmp;
    // disable write protection
    ds1302_write(0x8e);
    ds1302_write(0x00);
    ds1302_end_write_data();
    // set seconds
    ds1302_write(0x80);
    ds1302_write(ds1302_bin8_to_bcd(datetime.sec));
    ds1302_end_write_data();
    // set minutes
    ds1302_write(0x82);
    ds1302_write(ds1302_bin8_to_bcd(datetime.min));
    ds1302_end_write_data();
    // set hour
    tmp = (ds1302_bin8_to_bcd(datetime.hour) | datetime.AMPM | datetime.H12_24);
    ds1302_write(0x84);
    ds1302_write(tmp);
    ds1302_end_write_data();
    // set date
    ds1302_write(0x86);
    ds1302_write(ds1302_bin8_to_bcd(datetime.day));
    ds1302_end_write_data();
    // set month
    ds1302_write(0x88);
    ds1302_write(ds1302_bin8_to_bcd(datetime.month));
    ds1302_end_write_data();
    // set day (of week)
    ds1302_write(0x8A);
    ds1302_write(datetime.weekday);
    ds1302_end_write_data();
    // set year
    ds1302_write(0x8C);
    ds1302_write(ds1302_bin8_to_bcd(datetime.year));
    ds1302_end_write_data();
}
