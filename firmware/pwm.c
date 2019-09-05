/* Name: pwm.c
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2017-02-06
 * Tabsize: 4
 * Copyright: (c) 2017-2019 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 */

#include <avr/interrupt.h>

#include "pwm.h"

static uint8_t pwm_is_rising = 0;
static uint16_t pwm_level = 0;
static union {
    uint16_t full;
    struct {
        uint8_t low;
        uint8_t high;
    };
} pwm_risetime;
static uint8_t skipping_counter;

/* ------------------------------------------------------------------------- *
 * Update PWM state
 * ------------------------------------------------------------------------- */
static void pwm_update(void)
{
    if (pwm_risetime.full == 0)
    {
        // Turn on/off light immediately
        if (pwm_is_rising)
        {
            pwm_on();
        }
        else
        {
            pwm_off();
        }
    }
    else
    {
        // Turn on/off light gradually
        skipping_counter = pwm_risetime.high;
        TCNT0 = pwm_risetime.low;
        TIMSK |= (1 << TOIE0); // Enable PWM's timer
    }
}

void pwm_init(void)
{
    /*
     * I/O configuration
     */
    DDRB |= (1 << PB2);
    PORTB &= ~(1 << PB2);

    /*
     * Timer/Counter 0.
     * Increase PWM level every 27.52ms.
     * From 0 to 100% in ~30 minutes (65535 * 27.52ms).
     */
    TCCR0 = 0x05;               // Prescaler clk/1024
    TCNT0 = 0x29;               // 27.52 ms
    TIMSK |= (1 << TOIE0);      // Enable overflow on timer 0

    /*
     * Timer/Counter 1
     * Fast PWM on PB2 (OCR1B), non-inverting mode, prescaler disabled.
     */
    TCCR1A = (1 << COM1B1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
    ICR1 = 0xffff;
}

void pwm_setup(uint8_t level, uint8_t risetime)
{
    pwm_level = (level == 100) ? 0xffff : level * PWM_LEVEL_STEP;
    /*
     * F_CPU = 8000000 Hz
     * T0_tick = 1 / F_CPU / prescaler = 1 / 8000000 / 1024 = 0.000128 sec
     * Increasing PWM level from 0 to 65535 (100%) every T0_tick takes:
     * 0.000128 * 65535 = 8.4 sec
     * To rise light from 0 to 100% in 1 minute it needs about 7 T0_ticks:
     * (7 * 0.000128) * 65535 = 59 sec
     * To rise light from 0 to 100% in n minutes the T0 must count 7n cycles
     * between increasing PWM level:
     * pwm_risetime.full = 7 * risetime;
     * To rise light from 0 to "level"% in n minutes the previous value must be
     * compensated by level percentage:
     * pwm_risetime.full = (7 * risetime) * (100 / level);
     * This value is greater than T0 can count, so additional variable
     * "skipping_counter" is used to expand T0 from one byte to two bytes.
     * T0 counts from "pwm_risetime.low" up to 0xff (and overflows),
     * then T0 counts from 0x00 up to 0xff "pwm_risetime.high" times and only
     * then increases PWM level.
     */
    pwm_risetime.full = (risetime == 0 || level == 0) ? 0 : ((700 * risetime) / level);
    pwm_update();
}

uint8_t pwm_status(void)
{
    uint8_t status;

    // Current light brightness in percent.
    status = (uint8_t)(OCR1B / PWM_LEVEL_STEP);
    // Highest bit shows direction of PWM: 0 - falling, 1 - rising.
    status = pwm_is_rising ? status | 0x80 : status & 0x7f;

    return status;
}

void pwm_on(void)
{
    TIMSK &= ~(1 << TOIE0); // Disable PWM's timer
    pwm_is_rising = 1;
    OCR1B = pwm_level;
}

void pwm_off(void)
{
    TIMSK &= ~(1 << TOIE0); // Disable PWM's timer
    pwm_is_rising = 0;
    OCR1B = 0;
}

void pwm_rise(void)
{
    if (!pwm_is_rising)
    {
        pwm_is_rising = 1;
        pwm_update();
    }
}

void pwm_fall(void)
{
    if (pwm_is_rising)
    {
        pwm_is_rising = 0;
        pwm_update();
    }
}

/* ------------------------------------------------------------------------- *
 * Change PWM level by one step up or down
 * ------------------------------------------------------------------------- */
ISR (TIMER0_OVF_vect)
{
    if (skipping_counter > 0)
    {
        skipping_counter -= 1;
        return;
    }

    // Initialize timer
    skipping_counter = pwm_risetime.high;
    TCNT0 = 0xff - pwm_risetime.low;

    if (pwm_is_rising)
    {
        if (OCR1B < pwm_level)
        {
            OCR1B += 1;
        }
        else if (OCR1B > pwm_level)
        {
            OCR1B -= 1;
        }
        else
        {
            // Light has turned on with specified brightness
            TIMSK &= ~(1 << TOIE0); // Disable PWM's timer
        }
    }
    else
    {
        if (OCR1B > 0)
        {
            OCR1B -= 1;
        }
        else
        {
            // Light has turned off
            TIMSK &= ~(1 << TOIE0); // Disable PWM's timer
        }
    }
}
