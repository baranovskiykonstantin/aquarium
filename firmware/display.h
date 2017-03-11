/* Name: display.h
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2017 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 * This Revision: 1
 */

#ifndef __DISPLAY_H_INCLUDED__
#define __DISPLAY_H_INCLUDED__

#include <avr/io.h>
#include "ds1302.h"

typedef struct {
    volatile uint8_t *port;
    uint8_t pin;
} pin_t;

const static pin_t digits[4] = {
    {&PORTB, PB1}, // 1
    {&PORTC, PC5}, // 2
    {&PORTC, PC4}, // 3
    {&PORTC, PC3}  // 4
};

const static pin_t segments[8] = {
    {&PORTD, PD4}, // A
    {&PORTD, PD2}, // B
    {&PORTD, PD7}, // C
    {&PORTD, PD5}, // D
    {&PORTB, PB7}, // E
    {&PORTD, PD3}, // F
    {&PORTB, PB0}, // G
    {&PORTD, PD6}  // H
};

volatile uint8_t display[4];                 // Symbols that must be shown
volatile uint8_t current_digit;              // Current digit
volatile uint8_t current_segment;            // Current segment
const static uint8_t symbols[] = {
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

/* Initialize the ports and interrupts for display working.
 */
extern void display_init (void);

/* Shows the current value of the time on the display.
 */
extern void display_time (datetime_t);

/* Shows the current value of the temperature on the display.
 */
extern void display_temp (int8_t value);

#endif /* __DISPLAY_H_INCLUDED__ */
