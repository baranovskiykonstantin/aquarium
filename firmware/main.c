/* Name: main.c
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2017-2019 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 */

#include <avr/io.h>
#include <avr/wdt.h>

#include "aquarium.h"

int main(void)
{
    uint16_t loop_counter = 0;

    wdt_enable(WDTO_1S);

    aquarium_init();

    while (1)
    {
        wdt_reset();

        if (++loop_counter > 5000)
        {
            loop_counter = 0;
        }

        if ((loop_counter % 1000) == 0)
        {
            aquarium_process_time();

            if (loop_counter == 2000)
            {
                aquarium_process_sensors();
            }

            if (loop_counter == 3000)
            {
                aquarium_process_heat();
            }

            if (loop_counter == 5000)
            {
                aquarium_process_light();
            }
        }

        aquarium_process_uart();
    }
}
