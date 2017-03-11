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

/* ------------------------ Initialize the PWM ----------------------------- */
void pwm_init (void)
{
    /* I/O configuration
     */
    DDRB |= (1 << PB2);
    PORTB &= ~(1 << PB2);

    /* Timer/Counter 0
     * Increase PWM level every 920 us.
     * From 0 to 100% in ~60 seconds (65535 * 920us).
     */
    TCCR0 = 0x03;               // Prescaler clk/64
    TCNT0 = 0x8c;               // 920 us
    TIMSK |= (1 << TOIE0);      // Enable overflow on timer 0

    /* Timer/Counter 1
     * Fast PWM on PB2 (OCR1B), non-inverting mode, prescaler disabled.
     */
    TCCR1A = (1 << COM1B1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
    ICR1 = 0xffff;
    OCR1B = 0;

    /* Variables
     */
    pwm_enabled = 0;
    pwm_top_level = 100;
}

/* ----------------------- Set PWM new top level --------------------------- */
void pwm_update_level (void)
{
    TCNT0 = 0x8c;               // 920 us
    TIMSK |= (1 << TOIE0);      // Enable PWM's timer
}

/* ------------------------ PWM step up to top ----------------------------- */
void pwm_enable (void)
{
    pwm_enabled = 1;
    pwm_update_level ();
}

/* ------------------------ PWM step down to 0 ----------------------------- */
void pwm_disable (void)
{
    pwm_enabled = 0;
    pwm_update_level ();
}

/* ----------- Change PWM level by one step in specified direction --------- */
ISR (TIMER0_OVF_vect)
{
    TCNT0 = 0x8c;

    uint16_t pwm_level = pwm_top_level * PWM_STEP;
    if (pwm_top_level == 100)
        pwm_level = 0xffff;

    if (pwm_enabled)
    {
        if (OCR1B < pwm_level)
            OCR1B += 1;
        else if (OCR1B > pwm_level)
            OCR1B -= 1;
        else
            TIMSK &= ~(1 << TOIE0);      // Disable PWM's timer
    }
    else
    {
        if (OCR1B > 0)
            OCR1B -= 1;
        else
            TIMSK &= ~(1 << TOIE0);      // Disable PWM's timer
    }
}
