/* Name: pwm.c
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2017-02-06
 * Tabsize: 4
 * Copyright: (c) 2017 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 * This Revision: 1
 */

#include "pwm.h"
#include <avr/io.h>
#include <avr/interrupt.h>

static uint8_t pwm_enabled = 0;
static uint8_t pwm_value = 100;
static uint8_t rise_time = 1;
static const uint8_t rise_values[] PROGMEM = {
    0x00,    // 0 min
    0xf9,    // 1 min
    0xf2,    // 2 min
    0xeb,    // 3 min
    0xe3,    // 4 min
    0xdc,    // 5 min
    0xd5,    // 6 min
    0xce,    // 7 min
    0xc7,    // 8 min
    0xc0,    // 9 min
    0xb8,    // 10 min
    0xb1,    // 11 min
    0xaa,    // 12 min
    0xa3,    // 13 min
    0x9c,    // 14 min
    0x95,    // 15 min
    0x8e,    // 16 min
    0x86,    // 17 min
    0x7f,    // 18 min
    0x78,    // 19 min
    0x71,    // 20 min
    0x6a,    // 21 min
    0x63,    // 22 min
    0x5b,    // 23 min
    0x54,    // 24 min
    0x4d,    // 25 min
    0x46,    // 26 min
    0x3f,    // 27 min
    0x38,    // 28 min
    0x31,    // 29 min
    0x29     // 30 min
    };

/* ------------------------ Initialize the PWM ----------------------------- */
void pwm_init(void)
{
    /* I/O configuration
     */
    DDRB |= (1 << PB2);
    PORTB &= ~(1 << PB2);

    /* Timer/Counter 0
     * Increase PWM level every 920 us.
     * From 0 to 100% in ~60 seconds (65535 * 920us):
     *      TCCR0 = 0x03;               // Prescaler clk/64
     *      TCNT0 = 0x8d;               // 920 us
     * From 0 to 100% in ~30 minutes (65535 * 27.52ms):
     *      TCCR0 = 0x05;               // Prescaler clk/1024
     *      TCNT0 = 0x29;               // 27.52 ms
     */
    TCCR0 = 0x05;               // Prescaler clk/1024
    TCNT0 = 0x29;               // 27.52 ms
    TIMSK |= (1 << TOIE0);      // Enable overflow on timer 0

    /* Timer/Counter 1
     * Fast PWM on PB2 (OCR1B), non-inverting mode, prescaler disabled.
     */
    TCCR1A = (1 << COM1B1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
    ICR1 = 0xffff;
    OCR1B = 0;
}

/* --------------------------- Update PWM state ---------------------------- */
void pwm_update(void)
{
    uint16_t pwm_level;

    if (rise_time) {
        // Initialize timer
        TCNT0 = pgm_read_byte(&(rise_values[rise_time]));
        // Enable PWM's timer
        TIMSK |= (1 << TOIE0);
    } else {
        TIMSK &= ~(1 << TOIE0);      // Disable PWM's timer

        if (pwm_enabled) {
            if (pwm_value == 100)
                pwm_level = 0xffff;
            else
                pwm_level = pwm_value * PWM_STEP;
        } else {
            pwm_level = 0;
        }

        OCR1B = pwm_level;
    }
}

/* -------------------------- Get new rise time ---------------------------- */
uint8_t pwm_get_risetime(void)
{
    return rise_time;
}

/* -------------------------- Set new rise time ---------------------------- */
void pwm_set_risetime(uint8_t new_rise_time)
{
    if (new_rise_time > 30)
        rise_time = 30;
    else
        rise_time = new_rise_time;
    pwm_update();
}

/* -------------------------- Set new PWM value ---------------------------- */
uint8_t pwm_get_value(void)
{
    return pwm_value;
}

/* -------------------------- Set new PWM value ---------------------------- */
void pwm_set_value(uint8_t new_pwm_value)
{
    if (new_pwm_value > 100)
        pwm_value = 100;
    else
        pwm_value = new_pwm_value;
    pwm_update();
}

/* -------------------------- Get PWM status ------------------------------- */
uint8_t pwm_get_status(void)
{
    return pwm_enabled;
}

/* ------------------------ PWM step up to top ----------------------------- */
void pwm_enable(void)
{
    pwm_enabled = 1;
    pwm_update();
}

/* ------------------------ PWM step down to 0 ----------------------------- */
void pwm_disable(void)
{
    pwm_enabled = 0;
    pwm_update();
}

/* ----------- Change PWM level by one step in specified direction --------- */
ISR (TIMER0_OVF_vect)
{
    // Initialize timer
    TCNT0 = pgm_read_byte(&(rise_values[rise_time]));

    uint16_t pwm_level;

    if (pwm_value == 100)
        pwm_level = 0xffff;
    else
        pwm_level = pwm_value * PWM_STEP;

    if (pwm_enabled) {
        if (OCR1B < pwm_level)
            OCR1B += 1;
        else if (OCR1B > pwm_level)
            OCR1B -= 1;
        else
            TIMSK &= ~(1 << TOIE0);      // Disable PWM's timer
    } else {
        if (OCR1B > 0)
            OCR1B -= 1;
        else
            TIMSK &= ~(1 << TOIE0);      // Disable PWM's timer
    }
}
