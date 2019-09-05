/* Name: pwm.h
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2017-02-06
 * Tabsize: 4
 * Copyright: (c) 2017-2019 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 */

#ifndef __PWM_H_INCLUDED__
#define __PWM_H_INCLUDED__

#include <avr/io.h>

#define PWM_LEVEL_STEP 655 // 65535/100

/*
 * Initialize the PWM for controlling the light.
 */
extern void pwm_init(void);

/*
 * Setup PWM.
 * level - brightness level in percent.
 * risetime - light rise time in minutes.
 */
extern void pwm_setup(uint8_t level, uint8_t risetime);

/*
 * Turn on the light immediately.
 */
extern void pwm_on(void);

/*
 * Turn off the light immediately.
 */
extern void pwm_off(void);

/*
 * PWM steps up to light level.
 */
extern void pwm_rise(void);

/*
 * PWM steps down to 0.
 */
extern void pwm_fall(void);

/*
 * Get status of PWM activity.
 */
extern uint8_t pwm_status(void);

#endif /* __PWM_H_INCLUDED__ */
