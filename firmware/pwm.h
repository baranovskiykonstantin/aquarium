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
#include <avr/pgmspace.h>

#define PWM_STEP 655 // 65535/100

/* Initialize the PWM for controlling the light.
 */
extern void pwm_init(void);

/* PWM status.
 */
extern uint8_t pwm_get_status(void);

/* PWM step up to top value.
 */
extern void pwm_enable(void);

/* PWM step down to 0.
 */
extern void pwm_disable(void);

/* Get PWM value in percentage.
 */
extern uint8_t pwm_get_value(void);

/* Get PWM value in percentage.
 */
extern void pwm_set_value(uint8_t new_pwm_value);

/* Get rise time in minutes 0-30.
 */
extern uint8_t pwm_get_risetime(void);

/* Set rise time in minutes 0-30.
 */
extern void pwm_set_risetime(uint8_t new_rise_time);

#endif /* __PWM_H_INCLUDED__ */
