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

/*
 * Initialize the ports and interrupts for display working.
 */
extern void display_init(void);

/*
 * Shows the current value of the time on the display.
 */
extern void display_time(datetime_t *datetime);

/*
 * Shows the current value of the temperature on the display.
 */
extern void display_temp(int8_t value);

#endif /* __DISPLAY_H_INCLUDED__ */
