/* Name: pwm.h
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2017-02-06
 * Tabsize: 4
 * Copyright: (c) 2017 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 * This Revision: 1
 */

#ifndef __PWM_H_INCLUDED__
#define __PWM_H_INCLUDED__

#include <avr/io.h>

#define PWM_STEP 655 // 65535/100

volatile uint8_t rise_time;
volatile uint8_t rise_timer_value;
static const uint8_t rise_values_for_timer[] = {
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

/* PWM status.
 */
uint8_t pwm_enabled;

/* PWM limit.
 */
uint8_t pwm_top_level;

/* Initialize the PWM for controlling the light.
 */
extern void pwm_init (void);

/* PWM step up to top value.
 */
void pwm_enable (void);

/* PWM step down to 0.
 */
void pwm_disable (void);

/* Set PWM new top level
 */
void pwm_update_level (void);

#endif /* __PWM_H_INCLUDED__ */
