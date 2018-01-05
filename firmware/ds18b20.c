/* Name: ds18b20.c
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2017 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 * This Revision: 1
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "ds18b20.h"
#include "crc8.h"

/*
 * variable decreases every 256 us by Timer2
 */
static uint16_t timer = 0;

/*
 * ds18b20 init
 */
uint8_t ds18b20_reset()
{
    uint8_t i;

    //low for 480us
    DS18B20_PORT &= ~(1<<DS18B20_DQ); //low
    DS18B20_DDR |= (1<<DS18B20_DQ); //output
    sei();
    _delay_us(480);
    cli();

    //release line and wait for 60uS
    DS18B20_DDR &= ~(1<<DS18B20_DQ); //input
    _delay_us(60);

    //get value and wait 420us
    i = (DS18B20_PIN & (1<<DS18B20_DQ));
    sei();
    _delay_us(420);
    cli();

    //return the read value, 0=ok, 1=error
    return i;
}

/*
 * write one bit
 */
void ds18b20_writebit(uint8_t bit)
{
    //low for 1uS
    DS18B20_PORT &= ~(1<<DS18B20_DQ); //low
    DS18B20_DDR |= (1<<DS18B20_DQ); //output
    _delay_us(1);

    //if we want to write 1, release the line (if not will keep low)
    if (bit)
        DS18B20_DDR &= ~(1<<DS18B20_DQ); //input

    //wait 60uS and release the line
    sei();
    _delay_us(60);
    cli();
    DS18B20_DDR &= ~(1<<DS18B20_DQ); //input
}

/*
 * read one bit
 */
uint8_t ds18b20_readbit(void)
{
    uint8_t bit = 0;

    //low for 1uS
    DS18B20_PORT &= ~(1<<DS18B20_DQ); //low
    DS18B20_DDR |= (1<<DS18B20_DQ); //output
    _delay_us(1);

    //release line and wait for 14uS
    DS18B20_DDR &= ~(1<<DS18B20_DQ); //input
    _delay_us(14);

    //read the value
    if (DS18B20_PIN & (1<<DS18B20_DQ))
        bit = 1;

    //wait 45uS and return read value
    sei();
    _delay_us(45);
    cli();
    return bit;
}

/*
 * write one byte
 */
void ds18b20_writebyte(uint8_t byte)
{
    uint8_t i;
    for (i=8; i>0; i--){
        ds18b20_writebit(byte & 1);
        byte >>= 1;
    }
}

/*
 * read one byte
 */
uint8_t ds18b20_readbyte(void)
{
    uint8_t i;
    uint8_t n=0;
    for (i=8; i>0; i--){
        n >>= 1;
        n |= (ds18b20_readbit() << 7);
    }
    return n;
}

/*
 * Decrease timer
 */
void ds18b20_decrease_timer(void)
{
    if (timer > 0)
        timer--;
}

/*
 * Hard reset
 */
void ds18b20_hard_reset(void)
{
    DS18B20_PWR_OFF;
    _delay_ms(10);
    DS18B20_PWR_ON;
    _delay_ms(10);
    timer = 0;
}

/*
 * Get temperature
 */
int8_t ds18b20_gettemp(void)
{
    uint8_t scratchpad[SCRATCHPAD_SIZE];
    uint8_t i;
    double temp_value = DS18B20_ERR;

    if (timer > 0) {
        /* Converting not finished -- wait */
        return DS18B20_BUSY;
    } else {
        /* Read value of the measured temperature */

        cli();
        // reset
        if (ds18b20_reset()) {
            sei();
            return DS18B20_ERR;
        }
        ds18b20_writebyte(DS18B20_CMD_SKIPROM); //skip ROM
        ds18b20_writebyte(DS18B20_CMD_RSCRATCHPAD); //read scratchpad

        //read scratchpad
        for (i=0; i<SCRATCHPAD_SIZE; i++)
            scratchpad[i] = ds18b20_readbyte();

        sei();

        if (crc8(scratchpad, SCRATCHPAD_SIZE - 1) == scratchpad[SCRATCHPAD_CRC])
            //convert the value obtained
            temp_value = ((scratchpad[SCRATCHPAD_TEMP_H] << 8) + scratchpad[SCRATCHPAD_TEMP_L]) * 0.0625;

        /* Start new measurement */

        cli();
        // reset
        if (ds18b20_reset()) {
            sei();
            return DS18B20_ERR;
        }
        ds18b20_writebyte(DS18B20_CMD_SKIPROM); //skip ROM
        ds18b20_writebyte(DS18B20_CMD_WSCRATCHPAD); //write to scratchpad
        ds18b20_writebyte(DS18B20_RES); //conversion resolution

        sei();
        _delay_ms(1); // Update display
        cli();

        // reset
        if (ds18b20_reset()) {
            sei();
            return DS18B20_ERR;
        }
        ds18b20_writebyte(DS18B20_CMD_SKIPROM); //skip ROM
        ds18b20_writebyte(DS18B20_CMD_CONVERTTEMP); //start temperature conversion
        sei();

        // time for conversion
        // time counts by timer2 (see display.c)
        switch (DS18B20_RES) {
        // 368 * 256us = 94ms
        case DS18B20_RES_09: timer = 368; break;
        // 735 * 256us = 188ms
        case DS18B20_RES_10: timer = 735; break;
        // 1465 * 256us = 375ms
        case DS18B20_RES_11: timer = 1465; break;
        // 2930 * 256us = 750ms
        case DS18B20_RES_12: timer = 2930; break;
        }

        return (uint8_t)temp_value;
    }
}
