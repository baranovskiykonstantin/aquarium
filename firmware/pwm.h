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
