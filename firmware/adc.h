/* Name: adc.h
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2013-12-01
 * Tabsize: 4
 * Copyright: (c) 2017-2019 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 */

#ifndef __ADC_H_INCLUDED__
#define __ADC_H_INCLUDED__

#include <avr/io.h>

/*
 * Initialize the ADC for measuring level of the sensor output.
 */
extern void adc_init(void);

/*
 * Check state of specific sensor.
 */
extern uint8_t adc_sensor_state(uint8_t sensor);

#endif /* __ADC_H_INCLUDED__ */
