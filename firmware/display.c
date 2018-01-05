/* Name: display.c
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2017 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 * This Revision: 1
 */

#include <avr/interrupt.h>
#include <stdlib.h>
#include <math.h>

#include "display.h"
#include "ds18b20.h"
#include "ds1302.h"

static volatile uint8_t display[4];             // Symbols that must be shown
static volatile uint8_t current_digit;          // Current digit
static volatile uint8_t current_segment;        // Current segment

typedef struct {
    volatile uint8_t *port;
    uint8_t pin;
} pin_t;

static const pin_t digits[4] = {
    {&PORTB, PB1}, // 1
    {&PORTC, PC5}, // 2
    {&PORTC, PC4}, // 3
    {&PORTC, PC3}  // 4
};

static const pin_t segments[8] = {
    {&PORTD, PD4}, // A
    {&PORTD, PD2}, // B
    {&PORTD, PD7}, // C
    {&PORTD, PD5}, // D
    {&PORTB, PB7}, // E
    {&PORTD, PD3}, // F
    {&PORTB, PB0}, // G
    {&PORTD, PD6}  // H
};

static const uint8_t symbols[] = {
    //HGFEDCBA
    0b00111111,   // 0  - 0
    0b00000110,   // 1  - 1
    0b01011011,   // 2  - 2
    0b01001111,   // 3  - 3
    0b01100110,   // 4  - 4
    0b01101101,   // 5  - 5
    0b01111101,   // 6  - 6
    0b00000111,   // 7  - 7
    0b01111111,   // 8  - 8
    0b01101111,   // 9  - 9
    0b01100011,   // 10 - degr.
    0b00111001,   // 11 - C
    };

/* ------------------------- Initialize the display ------------------------ */
void display_init(void)
{
    /* I/O ports
     */
    PORTB |= 0x83;
    DDRB |= 0x83;
    PORTC |= 0x38;
    DDRC |= 0x38;
    PORTD |= 0xFC;
    DDRD |= 0xFC;

    /* Timer 2 - switch segments on display dynamically every 256 us
     */
    TCCR2 |= (1 << CS21);       // Normal mode, clk/8
    TCNT2 = 0x00;               // 256 us
    TIMSK |= (1 << TOIE2);      // Enable overflow on timer 2

    /* Variables
     */
    current_digit = 0;
    current_segment = 0;
    display[0] = 0xff;
    display[1] = 0xff;
    display[2] = 0xff;
    display[3] = 0xff;
}

/* ---------------------- Shows the value of the time ---------------------- */
void display_time(datetime_t *datetime)
{
    display[0] = symbols[datetime->min % 10];
    display[1] = symbols[datetime->min / 10];
    display[2] = symbols[datetime->hour % 10];
    if (datetime->hour < 10)
        display[3] = 0;
    else
        display[3] = symbols[datetime->hour / 10];
    if (datetime->sec % 2) {
        display[0] |= 0x80;
        display[1] |= 0x80;
    }
}

/* ------------------ Shows the value of the temperature ------------------- */
void display_temp(int8_t value)
{
    if (value == DS18B20_ERR) {
        display[0] = 0;
        display[1] = 0x40;
        display[2] = 0x40;
        display[3] = 0;
    } else if (labs(value) < 10) {
        display[0] = 0;
        display[1] = symbols[labs(value)];
        if (value < 0)
            display[2] = 0x40; // -
        else
            display[2] = 0;
        display[3] = 0;
    } else if (labs(value) < 100) {
        display[0] = 0;
        display[1] = symbols[labs(value) % 10];
        display[2] = symbols[labs(value) / 10];
        if (value < 0)
            display[3] = 0x40;
        else
            display[3] = 0;
    } else {
        display[0] = symbols[labs(value) % 10];
        display[1] = symbols[labs(value) % 100 / 10];
        display[2] = symbols[labs(value) % 1000 / 100];
        display[3] = 0;
    }
}

/* ------------------------ Timer 2 overflow ------------------------------- */
ISR (TIMER2_OVF_vect)
{
    uint8_t i;

    // Decrease timer of temperature measurement
    ds18b20_decrease_timer();

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

    if (++current_segment > 7) {
        current_segment = 0;
        if (++current_digit > 3) {
            current_digit = 0;
        } // All digits 0..3
    } // All segments 0..7
}

