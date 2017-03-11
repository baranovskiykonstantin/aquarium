/* Name: adc.h
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2013-12-01
 * Tabsize: 4
 * Copyright: (c) 2017 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 * This Revision: 1
 */

#ifndef __ADC_H_INCLUDED__
#define __ADC_H_INCLUDED__

#include <avr/io.h>

#define SENSOR_1    0x06    // Channel for sensor 1 - ADC6
#define SENSOR_2    0x07    // Channel for sensor 2 - ADC7

/* Initialize the ADC for measuring level of the sensor output.
 */
extern void adc_init (void);

/* Measure specific sensor.
 */
extern uint8_t get_sensor_state (uint8_t channel);

#endif /* __ADC_H_INCLUDED__ */
