/* Name: aquarium.h
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2019-08-25
 * Tabsize: 4
 * Copyright: (c) 2019 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 */

#ifndef __AQUARIUM_H_INCLUDED__
#define __AQUARIUM_H_INCLUDED__

/*
 * Initialize the aquarium data and peripheral.
 */
extern void aquarium_init(void);

/*
 * Read time from RTC.
 * Adjust current time by amount of daily correction if needed.
 */
extern void aquarium_process_time(void);

/*
 * Process the sensors triggering.
 */
extern void aquarium_process_sensors(void);

/*
 * Turn on/off heater accordingly to current temperature and settings.
 */
extern void aquarium_process_heat(void);

/*
 * Turn on/off light accordingly to current time and settings.
 */
extern void aquarium_process_light(void);

/*
 * Process UART connection.
 * If a valid command is received it will be executed.
 */
extern void aquarium_process_uart(void);

#endif /* __AQUARIUM_H_INCLUDED__ */
