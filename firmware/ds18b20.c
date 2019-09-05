/* Name: ds18b20.c
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2017-2019 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 */

#include <avr/interrupt.h>
#include <util/delay.h>

#include "ds18b20.h"
#include "crc8.h"

static uint8_t init_reset()
{
    uint8_t i;

    DS18B20_DQ_CLR;
    DS18B20_DQ_AS_OUT;
    sei();
    _delay_us(480);
    cli();

    DS18B20_DQ_AS_IN;
    _delay_us(60);
    i = DS18B20_DQ_GET;

    sei();
    _delay_us(420);
    cli();

    return i; // 0=ok, 1=error
}

static void write_bit(uint8_t bit)
{
    DS18B20_DQ_CLR;
    DS18B20_DQ_AS_OUT;
    _delay_us(1);

    if (bit)
    {
        // pulled up input == high output
        DS18B20_DQ_AS_IN;
    }

    sei();
    _delay_us(60);
    cli();
    DS18B20_DQ_AS_IN;
    _delay_us(1);
}

static uint8_t read_bit(void)
{
    uint8_t bit = 0;

    DS18B20_DQ_CLR;
    DS18B20_DQ_AS_OUT;
    _delay_us(1);

    DS18B20_DQ_AS_IN;
    _delay_us(12);

    if (DS18B20_DQ_GET)
    {
        bit = 1;
    }

    sei();
    _delay_us(48);
    cli();

    return bit;
}

static void write_byte(uint8_t byte)
{
    uint8_t i;

    for (i = 8; i > 0; i--)
    {
        write_bit(byte & 1);
        byte >>= 1;
    }
}

static uint8_t read_byte(void)
{
    uint8_t i;
    uint8_t byte = 0;

    for (i = 8; i > 0; i--)
    {
        byte >>= 1;
        byte |= (read_bit() << 7);
    }

    return byte;
}

void ds18b20_hard_reset(void)
{
    DS18B20_PWR_OFF;
    _delay_ms(10);
    DS18B20_PWR_ON;
    _delay_ms(10);
}

int8_t ds18b20_get_temp(void)
{
    uint8_t scratchpad[SCRATCHPAD_SIZE];
    double temp = DS18B20_ERR;
    uint8_t i;

    if (!(DS18B20_DQ_GET))
    {
        // Converting not finished - wait
        return DS18B20_BUSY;
    }

    // Read value of the measured temperature
    cli();
    if (init_reset())
    {
        sei();
        return DS18B20_ERR;
    }
    write_byte(DS18B20_CMD_SKIPROM);
    write_byte(DS18B20_CMD_RSCRATCHPAD);

    for (i = 0; i < SCRATCHPAD_SIZE; i++)
    {
        scratchpad[i] = read_byte();
    }

    sei();

    if (crc8(scratchpad, SCRATCHPAD_SIZE - 1) == scratchpad[SCRATCHPAD_CRC])
    {
        // convert read data to final temperature value
        temp = ((scratchpad[SCRATCHPAD_TEMP_H] << 8) + scratchpad[SCRATCHPAD_TEMP_L]) * 0.0625;
    }

    // Start new measurement

    cli();
    if (init_reset())
    {
        sei();
        return DS18B20_ERR;
    }
    write_byte(DS18B20_CMD_SKIPROM);
    write_byte(DS18B20_CMD_WSCRATCHPAD);
    write_byte(scratchpad[SCRATCHPAD_USER_TH]);
    write_byte(scratchpad[SCRATCHPAD_USER_TL]);
    write_byte(DS18B20_RES);

    if (init_reset())
    {
        sei();
        return DS18B20_ERR;
    }
    write_byte(DS18B20_CMD_SKIPROM);
    write_byte(DS18B20_CMD_CONVERTTEMP);
    sei();

    return (uint8_t)temp;
}
