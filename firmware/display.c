/* Name: display.c
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2017-2019 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 */

#include <avr/interrupt.h>
#include <stdlib.h>

#include "display.h"
#include "ds18b20.h"

#define MSG_DELAY 3906 // 3906 * 256us (T2_OVF) = 1 sec

static volatile uint8_t display[4];
static volatile uint8_t current_digit;
static volatile uint8_t current_segment;
static volatile uint16_t message_delay;

typedef struct
{
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
    0b00111111,   // 0  - 0      --A--
    0b00000110,   // 1  - 1    |       |
    0b01011011,   // 2  - 2    F       B
    0b01001111,   // 3  - 3    |       |
    0b01100110,   // 4  - 4     ---G---
    0b01101101,   // 5  - 5    |       |
    0b01111101,   // 6  - 6    E       C
    0b00000111,   // 7  - 7    |       |
    0b01111111,   // 8  - 8      --D--  _H
    0b01101111    // 9  - 9
};
//  0b00111111       O  - 0x3F
//  0b01010100       n  - 0x54
//  0b01110001       F  - 0x71
//  0b01110111       A  - 0x77
//  0b00111110       U  - 0x3E
//  0b01111000       t  - 0x78
//  0b01000000       -  - 0x40

void display_init(void)
{
    // I/O ports
    PORTB |= 0x83;
    DDRB |= 0x83;
    PORTC |= 0x38;
    DDRC |= 0x38;
    PORTD |= 0xFC;
    DDRD |= 0xFC;

    // Timer 2 - switch segments on display dynamically every 256 us
    TCCR2 |= (1 << CS21); // Normal mode, clk/8
    TCNT2 = 0x00; // 256 us
    TIMSK |= (1 << TOIE2); // Enable overflow on timer 2

    // Variables
    current_digit = 0;
    current_segment = 0;
    display[0] = 0xff;
    display[1] = 0xff;
    display[2] = 0xff;
    display[3] = 0xff;
}

void display_time(time_t *time)
{
    if (message_delay > 0)
    {
        return;
    }

    display[0] = symbols[time->min % 10];
    display[1] = symbols[time->min / 10];
    display[2] = symbols[time->hour % 10];
    display[3] = (time->hour < 10) ? 0x00 : symbols[time->hour / 10];
    // Colon at even seconds
    if (!(time->sec % 2))
    {
        display[0] |= 0x80;
        display[1] |= 0x80;
    }
}

void display_temp(int8_t temp)
{
    if (message_delay > 0)
    {
        return;
    }

    if (temp == DS18B20_ERR)
    {
        // " -- "
        display[0] = 0x00;
        display[1] = 0x40;
        display[2] = 0x40;
        display[3] = 0x00;
    }
    else if (labs(temp) < 10)
    {
        display[0] = 0x00;
        display[1] = symbols[labs(temp)];
        display[2] = (temp < 0) ? 0x40 : 0x00;
        display[3] = 0x00;
    }
    else if (labs(temp) < 100)
    {
        display[0] = 0x00;
        display[1] = symbols[labs(temp) % 10];
        display[2] = symbols[labs(temp) / 10];
        display[3] = (temp < 0) ? 0x40 : 0x00;
    }
    else
    {
        display[0] = symbols[labs(temp) % 10];
        display[1] = symbols[labs(temp) % 100 / 10];
        display[2] = symbols[labs(temp) % 1000 / 100];
        display[3] = 0x00;
    }
}

void display_message_on(void)
{
    display[0] = 0x00;
    display[1] = 0x54;
    display[2] = 0x3f;
    display[3] = 0x00;
    message_delay = MSG_DELAY;
}

void display_message_off(void)
{
    display[0] = 0x71;
    display[1] = 0x71;
    display[2] = 0x3f;
    display[3] = 0x00;
    message_delay = MSG_DELAY;
}

void display_message_auto(void)
{
    display[0] = 0x3f;
    display[1] = 0x78;
    display[2] = 0x3E;
    display[3] = 0x77;
    message_delay = MSG_DELAY;
}

ISR (TIMER2_OVF_vect)
{
    uint8_t i;

    // Turn off all digits
    for (i = 0; i < 4; i++)
    {
        *(digits[i].port) &= ~(1 << digits[i].pin);
    }

    // Turn off all segments
    for (i = 0; i < 8; i++)
    {
        *(segments[i].port) |= (1 << segments[i].pin);
    }

    // Turn on current segment if needed
    if (display[current_digit] & (1 << current_segment))
    {
        *(segments[current_segment].port) &= ~(1 << segments[current_segment].pin);
    }

    // Turn on current digit
    *(digits[current_digit].port) |= (1 << digits[current_digit].pin);

    if (++current_segment > 7)
    {
        current_segment = 0;
        if (++current_digit > 3)
        {
            current_digit = 0;
        } // All digits 0..3
    } // All segments 0..7

    if (message_delay > 0)
    {
        message_delay -= 1;
    }
}
