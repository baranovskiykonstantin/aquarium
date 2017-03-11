/* Name: adc.c
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2013-12-01
 * Tabsize: 4
 * Copyright: (c) 2017 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 * This Revision: 1
 */

#include "adc.h"
#include <avr/io.h>

/* ------------------------ Initialize the ADC ----------------------------- */
void adc_init (void)
{
    /* Registers
     */
    ADCSRA |= (1 << ADEN);          // Enable ADC
    ADCSRA |= (1 << ADPS1 | 1 << ADPS2); // ADC prescaler /64
}

/* ------------- Convert analog value to the logical state ----------------- */
uint8_t get_sensor_state (uint8_t channel)
{
    ADMUX &= 0xf0;                      // Reset channel to default
    ADMUX |= channel;                   // Set channel for measurement
    ADCSRA |= (1 << ADSC);              // Start measuring
    while (bit_is_set(ADCSRA, ADSC));   // Wait until measurement is finished
    if (ADCW > 512)                     // Return logical state of the sensor
        return 1;
    else
        return 0;
}
