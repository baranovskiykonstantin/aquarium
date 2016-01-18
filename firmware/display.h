/* Name: display.h
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2015 Baranovskiy Konstantin
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
    {&PORTB, PB3}, // 1
    {&PORTB, PB0}, // 2
    {&PORTD, PD7}, // 3
    {&PORTC, PC0}  // 4
};

const static pin_t segments[8] = {
    {&PORTB, PB2}, // A
    {&PORTD, PD6}, // B
    {&PORTC, PC2}, // C
    {&PORTB, PB5}, // D
    {&PORTB, PB4}, // E
    {&PORTB, PB1}, // F
    {&PORTC, PC1}, // G
    {&PORTC, PC3}  // H
};

volatile uint8_t display[4];                 // Symbols that must be shown
volatile uint8_t current_digit;              // Current digit
volatile uint8_t current_segment;            // Current segment
const static uint8_t symbols[] = {
    //HGFEDCBA
    0b00111111,   // 0  - 0
    0b00110000,   // 1  - 1
    0b01011011,   // 2  - 2
    0b01111001,   // 3  - 3
    0b01110100,   // 4  - 4
    0b01101101,   // 5  - 5
    0b01101111,   // 6  - 6
    0b00111000,   // 7  - 7
    0b01111111,   // 8  - 8
    0b01111101,   // 9  - 9
    0b01011100,   // 10 - degr.
    0b00001111,   // 11 - C
    };

/* Initialize the ports and interrupts for display working.
 */
extern void display_init (void);

/* Shows the current value of the time on the display.
 */
extern void display_time (datetime_t);

/* Shows the current value of the temperature on the display.
 */
extern void display_temp (double value);

#endif /* __DISPLAY_H_INCLUDED__ */
