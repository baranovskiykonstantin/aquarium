/* Name: display.h
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2017-2019 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 */

#ifndef __DISPLAY_H_INCLUDED__
#define __DISPLAY_H_INCLUDED__

#include <avr/io.h>

#include "datetime.h"

/*
 * Initialize the I/O and interrupts for display working.
 */
extern void display_init(void);

/*
 * Show time on the display.
 */
extern void display_time(time_t *time);

/*
 * Show temperature of the water on the display.
 */
extern void display_temp(int8_t temp);

/*
 * Show "On" message on the display for 1 sec.
 */
extern void display_message_on(void);

/*
 * Show "OFF" message on the display for 1 sec.
 */
extern void display_message_off(void);

/*
 * Show "AUtO" message on the display for 1 sec.
 */
extern void display_message_auto(void);

#endif /* __DISPLAY_H_INCLUDED__ */
