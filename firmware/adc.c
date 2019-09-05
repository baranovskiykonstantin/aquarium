/* Name: adc.c
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2013-12-01
 * Tabsize: 4
 * Copyright: (c) 2017-2019 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 */

#include "adc.h"

void adc_init(void)
{
    // Registers
    ADCSRA |= (1 << ADEN); // Enable ADC
    ADCSRA |= (1 << ADPS1 | 1 << ADPS2); // ADC prescaler /64
}

uint8_t adc_sensor_state(uint8_t sensor)
{
    ADMUX &= 0xf0; // Reset channel to default
    /*
     * ADC channel 0x06 (5 + 1) -> Sensor #1
     * ADC channel 0x07 (5 + 2) -> Sensor #2
     */
    ADMUX |= 5 + sensor; // Set channel for measurement
    ADCSRA |= (1 << ADSC); // Start measuring
    while (ADCSRA & (1 << ADSC)); // Wait until measurement is finished
    // Return logical state of the sensor
    if (ADCH > 1) // 0x0200(512)...
    {
        return 1;
    }
    return 0;
}
