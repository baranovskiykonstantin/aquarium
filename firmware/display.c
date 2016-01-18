/* Name: display.c
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2015 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 * This Revision: 1
 */

#include <avr/interrupt.h>
#include <stdlib.h>
#include <math.h>

#include "display.h"
#include "ds18b20.h"
#include "ds1302.h"

/* ------------------------- Initialize the display ------------------------ */
void display_init (void)
{
    /* I/O ports
     */
    PORTB |= 0x3f;
    DDRB |= 0x3f;
    PORTC |= 0x0f;
    DDRC |= 0x0f;
    PORTD |= 0xc0;
    DDRD |= 0xc0;

    /* Timer 2 - switch segments on display dynamically every 256 us
     */
    TCCR2 |= (1 << CS21);       // Normal mode, clk/8
    TCNT2 = 0x00;               // 256 us
    TIMSK |= (1 << TOIE2);      // Enable overflow on timer 2

    /* Variables
     */
    current_digit = 0;
    current_segment = 0;
    display[0] = 0x40;
    display[1] = 0x40;
    display[2] = 0x40;
    display[3] = 0x40;
}

/* ---------------------- Shows the value of the time ---------------------- */
void display_time (datetime_t datetime)
{
    display[0] = symbols[datetime.min % 10];
    display[1] = symbols[datetime.min / 10];
    display[2] = symbols[datetime.hour % 10];
    if (datetime.hour < 10)
        display[3] = 0;
    else
        display[3] = symbols[datetime.hour / 10];
    if (datetime.sec % 2)
    {
        display[2] |= 0x80;
        display[3] |= 0x80;
    }
}

/* ------------------ Shows the value of the temperature ------------------- */
void display_temp (double value)
{
    int32_t temp;
    temp = lround(value);

    if (value == DS18B20_ERR)
    {
        display[0] = 0;
        display[1] = 0x40;
        display[2] = 0x40;
        display[3] = 0;
    }
    else if (labs(temp) < 10)
    {
        display[0] = 0;
        display[1] = symbols[labs(temp)];
        if (temp < 0)
            display[2] = 0x40; // -
        else
            display[2] = 0;
        display[3] = 0;
    }
    else if (labs(temp) < 100)
    {
        display[0] = 0;
        display[1] = symbols[labs(temp) % 10];
        display[2] = symbols[labs(temp) / 10];
        if (temp < 0)
            display[3] = 0x40;
        else
            display[3] = 0;
    }
    else
    {
        display[0] = symbols[labs(temp) % 10];
        display[1] = symbols[labs(temp) % 100 / 10];
        display[2] = symbols[labs(temp) % 1000 / 100];
        display[3] = 0;
    }
}

/* ------------------------ Timer 2 overflow ------------------------------- */
ISR (TIMER2_OVF_vect)
{
    uint8_t i;
    // Turn off all digits
    for (i=0; i < 4; i++)
        *(digits[i].port) &= ~(1<<digits[i].pin);

    // Turn off all segments
    for (i=0; i < 8; i++)
        *(segments[i].port) |= (1<<segments[i].pin);

    // Turn on current segment if needed
    if (display[current_digit] & (1 << current_segment))
        *(segments[current_segment].port) &= ~(1<<segments[current_segment].pin);

    // Turn on current digit
    *(digits[current_digit].port) |= (1<<digits[current_digit].pin);

    if (++current_segment > 7)                // All segments 0..7
    {
        current_segment = 0;
        if (++current_digit > 3)              // All digits 0..3
            current_digit = 0;
    }
}

