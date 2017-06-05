/* Name: main.c
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2017 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 * This Revision: 1
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <math.h>

#include "display.h"
#include "ds18b20.h"
#include "ds1302.h"
#include "datetime.h"
#include "uart.h"
#include "adc.h"
#include "pwm.h"

/*
 * Addresses of the parameters in EEPROM
 */
#define EE_ADDR_TEMP_L (uint8_t *)0
#define EE_ADDR_TEMP_H (uint8_t *)1

#define EE_ADDR_TIME_ON_HOUR (uint8_t *)2
#define EE_ADDR_TIME_ON_MIN (uint8_t *)3
#define EE_ADDR_TIME_ON_SEC (uint8_t *)4

#define EE_ADDR_TIME_OFF_HOUR (uint8_t *)5
#define EE_ADDR_TIME_OFF_MIN (uint8_t *)6
#define EE_ADDR_TIME_OFF_SEC (uint8_t *)7

#define EE_ADDR_TIME_DAILY_CORR_SIGN (uint8_t *)8
#define EE_ADDR_TIME_DAILY_CORR_SEC (uint8_t *)9

#define EE_ADDR_HEAT_MODE (uint8_t *)10
#define EE_ADDR_LIGHT_MODE (uint8_t *)11
#define EE_ADDR_DISPLAY_MODE (uint8_t *)12

#define EE_ADDR_LIGHT_TOP_LEVEL (uint8_t *)13

/*
 * I/O configuration
 */
#define HEAT_AS_OUT DDRB |= (1 << PB6)
#define HEAT_ON PORTB |= (1 << PB6)
#define HEAT_OFF PORTB &= ~(1 << PB6)
#define HEAT_STATE (PINB & (1 << PB6)) >> PB6

#define SENSORS_PWR_AS_OUT DDRC |= (1 << PC0)
#define SENSORS_PWR_ON PORTC |= (1 << PC0)
#define SENSORS_PWR_OFF PORTC &= ~(1 << PC0)
#define SENSORS_PWR_STATE (PINC & (1 << PC0)) >> PC0

/*
 * Modes of the display
 */
enum {SHOW_TIME=1, SHOW_TEMP=2};

/*
 * Modes of the light and heat
 */
enum {AUTO='a', MANUAL='m'};

/*
 * Buffer for storing data received from UART
 */
uint8_t uart_str[UART_RX_BUFFER_SIZE];

/* ------------------------------------------------------------------------- *
 * Send int to UART as ASCII
 * ------------------------------------------------------------------------- */
void uart_puti (int8_t value, uint8_t padding_zeros)
{
    if (value < 0 )
        uart_putc ('-');
    if (labs (value) >= 100 || padding_zeros >= 3)
        uart_putc (labs (value) / 100 + 0x30);
    if (labs (value) >= 10 || padding_zeros >= 2)
        uart_putc (labs (value) % 100 / 10 + 0x30);
    uart_putc (value % 10 + 0x30);
}

/* ------------------------------------------------------------------------- *
 * Get int from buffer
 * ------------------------------------------------------------------------- */
uint8_t uart_str_get_int (uint8_t indx, uint8_t len)
{
    uint8_t i;
    uint8_t value = 0;
    for (i=0; i < len; i++)
    {
        value *= 10;
        value += (uart_str[indx + i] - 0x30);
    }
    return value;
}

/* ------------------------------------------------------------------------- *
 * Send confirmation of the success or error message to UART
 * ------------------------------------------------------------------------- */
void uart_ok (char success)
{
    if (success)
    {
        uart_puts ("OK\r\n");
    }
    else
    {
        uart_puts ("ERROR\r\n");
    }
}

/* ------------------------------------------------------------------------- *
 * Check if char is digit
 * ------------------------------------------------------------------------- */
uint8_t chr_is_digit (uint8_t chr)
{
    if (chr >= '0' && chr <= '9')
        return 1;
    else
        return 0;
}

/* ------------------------------------------------------------------------- *
 * Time correction
 * ------------------------------------------------------------------------- */
void time_correction (datetime_t *datetime_current,
                      datetime_t *datetime_corr,
                      time_t *daily_corr)
{
    // If current date bigger then date of correction more then one day...
    if (datetime_current->year > datetime_corr->year ||
            (datetime_current->year == datetime_corr->year &&
             datetime_current->month > datetime_corr->month) ||
            (datetime_current->year == datetime_corr->year &&
             datetime_current->month == datetime_corr->month &&
             datetime_current->day > datetime_corr->day)
            )
    {
        // ... and if the current time bigger than time of the setup time...
        if (datetime_current->hour > datetime_corr->hour ||
                (datetime_current->hour == datetime_corr->hour &&
                 datetime_current->min > datetime_corr->min) ||
                (datetime_current->hour == datetime_corr->hour &&
                 datetime_current->min == datetime_corr->min &&
                 datetime_current->sec > datetime_corr->sec)
                )
        {
            // Apply time correction
            // In AMPM is stored sign of the time correction
            switch (daily_corr->AMPM)
            {
                case '+':
                    datetime_add_time (datetime_current, daily_corr);
                    break;
                case '-':
                    datetime_sub_time (datetime_current, daily_corr);
            }
            ds1302_write_datetime (datetime_current);
            // Save date and time of last correction to RAM of RTC
            datetime_corr->year = datetime_current->year;
            datetime_corr->month = datetime_current->month;
            datetime_corr->day = datetime_current->day;
            datetime_corr->weekday = datetime_current->weekday;
            ds1302_write_datetime_to_ram (0, datetime_corr);
        }
    }
}

/* ========================================================================= *
 * Main function - start point of the program
 * ========================================================================= */
int main (void)
{
    uint16_t loop_count = 0;

    uint16_t uart_chr;
    uint8_t uart_str_indx = 0;

    uint8_t sensor_state = 0;
    uint8_t sensor_prev_state = 1;

    uint8_t display_mode = SHOW_TIME;

    datetime_t * volatile datetime_current;
    datetime_t * volatile datetime_corr;
    datetime_t * volatile datetime_tmp;
    time_t daily_corr, time_on, time_off;

    uint8_t temp_fail_counter = 0;
    int8_t temp_tmp;
    int8_t temp_value = DS18B20_ERR;
    int8_t temp_h, temp_l;

    uint8_t heat_mode, light_mode;

    // Initialize I/O
    HEAT_AS_OUT;
    HEAT_OFF;
    DS18B20_PWR_AS_OUT;
    DS18B20_PWR_ON;
    SENSORS_PWR_AS_OUT;
    SENSORS_PWR_ON;

    display_init ();                 // Initialize the display
    ds1302_init ();                  // Initialize the RTC
    uart_init (UART_BAUD_SELECT (9600, F_CPU)); // Initialize the UART
    adc_init ();                     // Initialize the ADC for sensors
    pwm_init ();                     // Initialize PWM for lighting
    wdt_enable (WDTO_2S);            // Enable watchdog timer

    // Restore parameters from EEPROM
    temp_l = eeprom_read_byte (EE_ADDR_TEMP_L);
    temp_h = eeprom_read_byte (EE_ADDR_TEMP_H);

    pwm_top_level = eeprom_read_byte (EE_ADDR_LIGHT_TOP_LEVEL);

    time_on.hour = eeprom_read_byte (EE_ADDR_TIME_ON_HOUR);
    time_on.min = eeprom_read_byte (EE_ADDR_TIME_ON_MIN);
    time_on.sec = eeprom_read_byte (EE_ADDR_TIME_ON_SEC);
    time_on.AMPM = AM;
    time_on.H12_24 = H24;

    time_off.hour = eeprom_read_byte (EE_ADDR_TIME_OFF_HOUR);
    time_off.min = eeprom_read_byte (EE_ADDR_TIME_OFF_MIN);
    time_off.sec = eeprom_read_byte (EE_ADDR_TIME_OFF_SEC);
    time_off.AMPM = AM;
    time_off.H12_24 = H24;

    // In AMPM is stored sign of the time correction
    daily_corr.AMPM = eeprom_read_byte (EE_ADDR_TIME_DAILY_CORR_SIGN);
    daily_corr.hour = 0;
    daily_corr.min = 0;
    daily_corr.sec = eeprom_read_byte (EE_ADDR_TIME_DAILY_CORR_SEC);
    daily_corr.H12_24 = H24;

    heat_mode = eeprom_read_byte (EE_ADDR_HEAT_MODE);
    light_mode = eeprom_read_byte (EE_ADDR_LIGHT_MODE);
    display_mode = eeprom_read_byte (EE_ADDR_DISPLAY_MODE);

    // Read date and time of the last time correction from RAM of DS1302
    ds1302_read_datetime_from_ram (0, datetime_corr);

    sei ();                          // Enable all interrupts

    // Sensors recalibration
    SENSORS_PWR_OFF;
    _delay_ms (10);
    SENSORS_PWR_ON;

    // Test display
    _delay_ms (500);
    wdt_reset ();

    // Read time
    ds1302_read_datetime (datetime_current);

    // Update display
    switch (display_mode)
    {
        case SHOW_TIME: display_time (datetime_current); break;
        case SHOW_TEMP: display_temp (temp_value); break;
    }

    /* -------------------------- Main loop -------------------------------- */
    while (1)
    {
        loop_count++;

        // Watchdog
        wdt_reset ();

        // Time
        if ((loop_count % 1000) == 0)
        {
            ds1302_read_datetime (datetime_current);
            if (display_mode == SHOW_TIME)
            {
                display_time (datetime_current);
            }
        }

        // Time correction
        if (loop_count == 0)
        {
            time_correction (datetime_current, datetime_corr, &daily_corr);
        }

        // Light
        if ((loop_count % 5000) == 0)
        {
            if (light_mode == AUTO)
            {
                if (((datetime_current->hour > time_on.hour) ||
                     (datetime_current->hour >= time_on.hour &&
                      datetime_current->min > time_on.min) ||
                     (datetime_current->hour >= time_on.hour &&
                     datetime_current->min >= time_on.min &&
                     datetime_current->sec >= time_on.sec)
                     )
                    &&
                    ((datetime_current->hour < time_off.hour) ||
                     (datetime_current->hour <= time_off.hour &&
                      datetime_current->min < time_off.min) ||
                     (datetime_current->hour <= time_off.hour &&
                      datetime_current->min <= time_off.min &&
                      datetime_current->sec < time_off.sec))
                    )
                    pwm_enable ();
                else
                    pwm_disable ();
            }
        }

        // Temp
        if ((loop_count % 3000) == 0)
        {
            temp_tmp = ds18b20_gettemp ();
            if (temp_tmp == DS18B20_ERR)
            {
                if (temp_fail_counter++)
                {
                    temp_value = temp_tmp;
                    temp_fail_counter = 0;
                    // Hard reset
                    DS18B20_PWR_OFF;
                    _delay_ms (10);
                    DS18B20_PWR_ON;
                    _delay_ms (10);
                    ds18b20_convert_flag = 0;
                }
            }
            else if (temp_tmp != DS18B20_BUSY)
            {
                // By default (after reset) DS18B20 returns value 85 - skip it!
                if (temp_value == DS18B20_ERR && temp_tmp == 85)
                {
                    temp_value = DS18B20_ERR;
                }
                else
                {
                    temp_value = temp_tmp;
                }
            }

            if (display_mode == SHOW_TEMP)
            {
                display_temp (temp_value);
            }
        }

        // Heat
        if (heat_mode == AUTO)
        {
            if (temp_value == DS18B20_ERR)
            {
                HEAT_OFF;
            }
            else
            {
                if ((temp_value < temp_l) && !(HEAT_STATE))
                    HEAT_ON;
                if ((temp_value > temp_h) && (HEAT_STATE))
                    HEAT_OFF;
            }
        }

        // Sensor 1
        if ((loop_count % 1000) == 0)
        {
            sensor_state = get_sensor_state (SENSOR_1);
            if (sensor_state != sensor_prev_state)
            {
                // Eliminate random triggers
                _delay_ms (100);
                sensor_state = get_sensor_state (SENSOR_1);
                if (sensor_state != sensor_prev_state)
                {
                    sensor_prev_state = sensor_state;
                    if (sensor_state)
                    {
                        switch (display_mode)
                        {
                            case SHOW_TIME: display_mode = SHOW_TEMP;
                                            display_temp (temp_value);
                                            break;
                            case SHOW_TEMP: display_mode = SHOW_TIME;
                                            display_time (datetime_current);
                                            break;
                        }
                        eeprom_update_byte (EE_ADDR_DISPLAY_MODE, display_mode);
                    }
                }
            }
        }

        // UART
        uart_chr = uart_getc ();
        while ((uart_chr & 0xff00) == 0)
        {
            // Put char to buffer
            uart_str[uart_str_indx++] = (uart_chr & 0xff);
            if (uart_str_indx >= UART_RX_BUFFER_SIZE)
            {
                uart_str_indx = 0;
                break;
            }
            // Backspace correction
            if (uart_str[uart_str_indx - 1] == 0x08 ||
                uart_str[uart_str_indx - 1] == 0x7f)
            {
                if (uart_str_indx < 2)
                {
                    uart_str_indx = 0;
                }
                else
                {
                    uart_str_indx -= 2;
                    // Echo backspace - erase previous character
                    uart_putc('\b');
                    uart_putc(' ');
                    uart_putc('\b');
                }
                break;
            }
            // Echo
            uart_putc(uart_chr);
            // Command processing
            if (uart_str[uart_str_indx - 1] == '\n')
            {
                if (uart_str[0] == 's' &&
                    uart_str[1] == 't' &&
                    uart_str[2] == 'a' &&
                    uart_str[3] == 't' &&
                    uart_str[4] == 'u' &&
                    uart_str[5] == 's'
                    )
                {
                        uart_puts ("Date: ");
                        uart_puti (datetime_current->day, 2);
                        uart_putc ('.');
                        uart_puti (datetime_current->month, 2);
                        uart_putc ('.');
                        uart_puti (datetime_current->year, 2);
                        uart_putc (' ');
                        switch (datetime_current->weekday)
                        {
                            case 1: uart_puts ("Monday"); break;
                            case 2: uart_puts ("Tuesday"); break;
                            case 3: uart_puts ("Wednesday"); break;
                            case 4: uart_puts ("Thursday"); break;
                            case 5: uart_puts ("Friday"); break;
                            case 6: uart_puts ("Saturday"); break;
                            case 7: uart_puts ("Sunday"); break;
                        }
                        uart_puts ("\r\nTime: ");
                        uart_puti (datetime_current->hour, 2);
                        uart_putc (':');
                        uart_puti (datetime_current->min, 2);
                        uart_putc (':');
                        uart_puti (datetime_current->sec, 2);
                        uart_puts (" (");
                        uart_putc (daily_corr.AMPM);
                        uart_puti (daily_corr.sec, 0);
                        uart_puts (" sec at ");
                        ds1302_read_datetime_from_ram(0, datetime_tmp);
                        uart_puti (datetime_tmp->hour, 2);
                        uart_putc (':');
                        uart_puti (datetime_tmp->min, 2);
                        uart_putc (':');
                        uart_puti (datetime_tmp->sec, 2);
                        uart_putc (')');
                        uart_puts ("\r\nTemp: ");
                        if (temp_value == DS18B20_ERR)
                            uart_puts ("--");
                        else
                            uart_puti (temp_value, 0);
                        uart_puts ("\r\nHeat: ");
                        if (HEAT_STATE)
                            uart_puts ("ON");
                        else
                            uart_puts ("OFF");
                        switch (heat_mode)
                        {
                            case AUTO:
                                uart_puts (" auto "); break;
                            case MANUAL:
                                uart_puts (" manual "); break;
                        }
                        uart_putc ('(');
                        uart_puti (temp_l, 0);
                        uart_putc ('-');
                        uart_puti (temp_h, 0);
                        uart_putc (')');
                        uart_puts ("\r\nLight: ");
                        if (pwm_enabled)
                            uart_puts ("ON");
                        else
                            uart_puts ("OFF");
                        switch (light_mode)
                        {
                            case AUTO:
                                uart_puts (" auto "); break;
                            case MANUAL:
                                uart_puts (" manual "); break;
                        }
                        uart_putc ('(');
                        uart_puti (time_on.hour, 2);
                        uart_putc (':');
                        uart_puti (time_on.min, 2);
                        uart_putc (':');
                        uart_puti (time_on.sec, 2);
                        uart_putc ('-');
                        uart_puti (time_off.hour, 2);
                        uart_putc (':');
                        uart_puti (time_off.min, 2);
                        uart_putc (':');
                        uart_puti (time_off.sec, 2);
                        uart_putc (')');
                        uart_putc (' ');
                        uart_puti (pwm_top_level, 0);
                        uart_putc ('%');
                        uart_puts ("\r\nDisplay: ");
                        switch (display_mode)
                        {
                            case SHOW_TIME:
                                uart_puts ("time"); break;
                            case SHOW_TEMP:
                                uart_puts ("temp"); break;
                        }
                        uart_puts ("\r\n");
                }
                else if (uart_str[0] == 'd' &&
                         uart_str[1] == 'a' &&
                         uart_str[2] == 't' &&
                         uart_str[3] == 'e' &&
                         uart_str[4] == ' ' &&
                         chr_is_digit (uart_str[5]) &&
                         chr_is_digit (uart_str[6]) &&
                         uart_str[7] == '.' &&
                         chr_is_digit (uart_str[8]) &&
                         chr_is_digit (uart_str[9]) &&
                         uart_str[10] == '.' &&
                         chr_is_digit (uart_str[11]) &&
                         chr_is_digit (uart_str[12]) &&
                         uart_str[13] == ' ' &&
                         chr_is_digit (uart_str[14])
                         )
                {
                    datetime_current->day = uart_str_get_int (5, 2);
                    if (datetime_current->day > 31)
                        datetime_current->day = 31;
                    datetime_current->month = uart_str_get_int (8, 2);
                    if (datetime_current->month > 12)
                        datetime_current->month = 12;
                    datetime_current->year = uart_str_get_int (11, 2);
                    datetime_current->weekday = uart_str_get_int (14, 1);
                    if (datetime_current->weekday > 7)
                        datetime_current->weekday = 7;
                    ds1302_write_datetime (datetime_current);
                    datetime_corr = datetime_current;
                    ds1302_write_datetime_to_ram (0, datetime_corr);
                    uart_ok (1);
                }
                else if (uart_str[0] == 't' &&
                         uart_str[1] == 'i' &&
                         uart_str[2] == 'm' &&
                         uart_str[3] == 'e' &&
                         uart_str[4] == ' '
                         )
                {
                    if (chr_is_digit (uart_str[5]) &&
                        chr_is_digit (uart_str[6]) &&
                        uart_str[7] == ':' &&
                        chr_is_digit (uart_str[8]) &&
                        chr_is_digit (uart_str[9]) &&
                        uart_str[10] == ':' &&
                        chr_is_digit (uart_str[11]) &&
                        chr_is_digit (uart_str[12])
                        )
                    {
                        if (uart_str[13] == ' ' &&
                            (uart_str[14] == '+'|| uart_str[14] == '-') &&
                            chr_is_digit (uart_str[15]) &&
                            chr_is_digit (uart_str[16])
                            )
                        {
                            datetime_current->hour = uart_str_get_int (5, 2);
                            if (datetime_current->hour > 23)
                                datetime_current->hour = 23;
                            datetime_current->min = uart_str_get_int (8, 2);
                            if (datetime_current->min > 59)
                                datetime_current->min =59;
                            datetime_current->sec = uart_str_get_int (11, 2);
                            if (datetime_current->sec > 59)
                                datetime_current->sec = 59;
                            datetime_current->H12_24 = H24;
                            datetime_current->AMPM = AM;
                            ds1302_write_datetime (datetime_current);
                            datetime_corr = datetime_current;
                            ds1302_write_datetime_to_ram (0, datetime_corr);

                            daily_corr.AMPM = uart_str[14];
                            daily_corr.sec = uart_str_get_int (15, 2);
                            if (daily_corr.sec > 59)
                                daily_corr.sec = 59;
                            eeprom_update_byte (EE_ADDR_TIME_DAILY_CORR_SIGN,
                                    daily_corr.AMPM);
                            eeprom_update_byte (EE_ADDR_TIME_DAILY_CORR_SEC,
                                    daily_corr.sec);
                            uart_ok (1);
                        }
                        else
                        {
                            datetime_current->hour = uart_str_get_int (5, 2);
                            if (datetime_current->hour > 23)
                                datetime_current->hour = 23;
                            datetime_current->min = uart_str_get_int (8, 2);
                            if (datetime_current->min > 59)
                                datetime_current->min =59;
                            datetime_current->sec = uart_str_get_int (11, 2);
                            if (datetime_current->sec > 59)
                                datetime_current->sec = 59;
                            datetime_current->H12_24 = H24;
                            datetime_current->AMPM = AM;
                            ds1302_write_datetime (datetime_current);
                            datetime_corr = datetime_current;
                            ds1302_write_datetime_to_ram (0, datetime_corr);
                            uart_ok (1);
                        }
                    }
                    else if ((uart_str[5] == '+'|| uart_str[5] == '-') &&
                             chr_is_digit (uart_str[6]) &&
                             chr_is_digit (uart_str[7])
                             )
                    {
                        daily_corr.AMPM = uart_str[5];
                        daily_corr.sec = uart_str_get_int (6, 2);
                        if (daily_corr.sec > 59)
                            daily_corr.sec = 59;
                        eeprom_update_byte (EE_ADDR_TIME_DAILY_CORR_SIGN,
                                daily_corr.AMPM);
                        eeprom_update_byte (EE_ADDR_TIME_DAILY_CORR_SEC,
                                daily_corr.sec);
                        uart_ok (1);
                    }
                    else
                    {
                        uart_ok (0);
                    }
                }
                else if (uart_str[0] == 'h' &&
                         uart_str[1] == 'e' &&
                         uart_str[2] == 'a' &&
                         uart_str[3] == 't' &&
                         uart_str[4] == ' '
                         )
                {
                    if (chr_is_digit (uart_str[5]) &&
                        chr_is_digit (uart_str[6]) &&
                        uart_str[7] == '-' &&
                        chr_is_digit (uart_str[8]) &&
                        chr_is_digit (uart_str[9])
                        )
                    {
                        temp_l = uart_str_get_int (5, 2);
                        temp_h = uart_str_get_int (8, 2);
                        eeprom_update_byte (EE_ADDR_TEMP_L, temp_l);
                        eeprom_update_byte (EE_ADDR_TEMP_H, temp_h);
                        uart_ok (1);
                    }
                    else if (uart_str[5] == 'o' &&
                             uart_str[6] == 'n'
                             )
                    {
                        heat_mode = MANUAL;
                        eeprom_update_byte (EE_ADDR_HEAT_MODE, heat_mode);
                        HEAT_ON;
                        uart_ok (1);
                    }
                    else if (uart_str[5] == 'o' &&
                             uart_str[6] == 'f' &&
                             uart_str[7] == 'f'
                             )
                    {
                        heat_mode = MANUAL;
                        eeprom_update_byte (EE_ADDR_HEAT_MODE, heat_mode);
                        HEAT_OFF;
                        uart_ok (1);
                    }
                    else if (uart_str[5] == 'a' &&
                             uart_str[6] == 'u' &&
                             uart_str[7] == 't' &&
                             uart_str[8] == 'o'
                             )
                    {
                        heat_mode = AUTO;
                        eeprom_update_byte (EE_ADDR_HEAT_MODE, heat_mode);
                        uart_ok (1);
                    }
                    else
                    {
                        uart_ok (0);
                    }
                }
                else if (uart_str[0] == 'l' &&
                         uart_str[1] == 'i' &&
                         uart_str[2] == 'g' &&
                         uart_str[3] == 'h' &&
                         uart_str[4] == 't' &&
                         uart_str[5] == ' '
                         )
                {
                    if (chr_is_digit (uart_str[6]) &&
                        chr_is_digit (uart_str[7]) &&
                        uart_str[8] == ':' &&
                        chr_is_digit (uart_str[9]) &&
                        chr_is_digit (uart_str[10]) &&
                        uart_str[11] == ':' &&
                        chr_is_digit (uart_str[12]) &&
                        chr_is_digit (uart_str[13]) &&
                        uart_str[14] == '-' &&
                        chr_is_digit (uart_str[15]) &&
                        chr_is_digit (uart_str[16]) &&
                        uart_str[17] == ':' &&
                        chr_is_digit (uart_str[18]) &&
                        chr_is_digit (uart_str[19]) &&
                        uart_str[20] == ':' &&
                        chr_is_digit (uart_str[21]) &&
                        chr_is_digit (uart_str[22])
                        )
                    {
                        if (uart_str[23] == ' ' &&
                            chr_is_digit (uart_str[24]) &&
                            chr_is_digit (uart_str[25]) &&
                            chr_is_digit (uart_str[26])
                            )
                        {
                            time_on.hour = uart_str_get_int (6, 2);
                            if (time_on.hour > 23)
                                time_on.hour = 23;
                            time_on.min = uart_str_get_int (9, 2);
                            if (time_on.min > 59)
                                time_on.min = 59;
                            time_on.sec = uart_str_get_int (12, 2);
                            if (time_on.sec > 59)
                                time_on.sec =59;
                            time_off.hour = uart_str_get_int (15, 2);
                            if (time_off.hour > 23)
                                time_off.hour = 23;
                            time_off.min = uart_str_get_int (18, 2);
                            if (time_off.min > 59)
                                time_off.min = 59;
                            time_off.sec = uart_str_get_int (21, 2);
                            if (time_off.sec > 59)
                                time_off.sec =59;
                            eeprom_update_byte (EE_ADDR_TIME_ON_HOUR, time_on.hour);
                            eeprom_update_byte (EE_ADDR_TIME_ON_MIN, time_on.min);
                            eeprom_update_byte (EE_ADDR_TIME_ON_SEC, time_on.sec);
                            eeprom_update_byte (EE_ADDR_TIME_OFF_HOUR, time_off.hour);
                            eeprom_update_byte (EE_ADDR_TIME_OFF_MIN, time_off.min);
                            eeprom_update_byte (EE_ADDR_TIME_OFF_SEC, time_off.sec);

                            pwm_top_level = uart_str_get_int (24, 3);
                            if (pwm_top_level > 100)
                                pwm_top_level = 100;
                            pwm_update_level ();
                            eeprom_update_byte (EE_ADDR_LIGHT_TOP_LEVEL, pwm_top_level);
                            uart_ok (1);
                        }
                        else
                        {
                            time_on.hour = uart_str_get_int (6, 2);
                            if (time_on.hour > 23)
                                time_on.hour = 23;
                            time_on.min = uart_str_get_int (9, 2);
                            if (time_on.min > 59)
                                time_on.min = 59;
                            time_on.sec = uart_str_get_int (12, 2);
                            if (time_on.sec > 59)
                                time_on.sec =59;
                            time_off.hour = uart_str_get_int (15, 2);
                            if (time_off.hour > 23)
                                time_off.hour = 23;
                            time_off.min = uart_str_get_int (18, 2);
                            if (time_off.min > 59)
                                time_off.min = 59;
                            time_off.sec = uart_str_get_int (21, 2);
                            if (time_off.sec > 59)
                                time_off.sec =59;
                            eeprom_update_byte (EE_ADDR_TIME_ON_HOUR, time_on.hour);
                            eeprom_update_byte (EE_ADDR_TIME_ON_MIN, time_on.min);
                            eeprom_update_byte (EE_ADDR_TIME_ON_SEC, time_on.sec);
                            eeprom_update_byte (EE_ADDR_TIME_OFF_HOUR, time_off.hour);
                            eeprom_update_byte (EE_ADDR_TIME_OFF_MIN, time_off.min);
                            eeprom_update_byte (EE_ADDR_TIME_OFF_SEC, time_off.sec);
                        }
                    }
                    else if (uart_str[6] == 'o' &&
                             uart_str[7] == 'n'
                             )
                    {
                        light_mode = MANUAL;
                        eeprom_update_byte (EE_ADDR_LIGHT_MODE, light_mode);
                        pwm_enable ();
                        uart_ok (1);
                    }
                    else if (uart_str[6] == 'o' &&
                             uart_str[7] == 'f' &&
                             uart_str[8] == 'f'
                             )
                    {
                        light_mode = MANUAL;
                        eeprom_update_byte (EE_ADDR_LIGHT_MODE, light_mode);
                        pwm_disable ();
                        uart_ok (1);
                    }
                    else if (uart_str[6] == 'a' &&
                             uart_str[7] == 'u' &&
                             uart_str[8] == 't' &&
                             uart_str[9] == 'o'
                             )
                    {
                        light_mode = AUTO;
                        eeprom_update_byte (EE_ADDR_LIGHT_MODE, light_mode);
                        uart_ok (1);
                    }
                    else if (uart_str[6]  == 'l' &&
                             uart_str[7]  == 'e' &&
                             uart_str[8]  == 'v' &&
                             uart_str[9]  == 'e' &&
                             uart_str[10] == 'l' &&
                             uart_str[11] == ' ' &&
                             chr_is_digit (uart_str[12]) &&
                             chr_is_digit (uart_str[13]) &&
                             chr_is_digit (uart_str[14])
                             )
                    {
                        pwm_top_level = uart_str_get_int (12, 3);
                        if (pwm_top_level > 100)
                            pwm_top_level = 100;
                        pwm_update_level ();
                        eeprom_update_byte (EE_ADDR_LIGHT_TOP_LEVEL, pwm_top_level);
                        uart_ok (1);
                    }
                    else
                    {
                        uart_ok (0);
                    }
                }
                else if (uart_str[0] == 'd' &&
                         uart_str[1] == 'i' &&
                         uart_str[2] == 's' &&
                         uart_str[3] == 'p' &&
                         uart_str[4] == 'l' &&
                         uart_str[5] == 'a' &&
                         uart_str[6] == 'y' &&
                         uart_str[7] == ' '
                         )
                {
                    if (uart_str[8]  == 't' &&
                        uart_str[9]  == 'i' &&
                        uart_str[10] == 'm' &&
                        uart_str[11] == 'e'
                        )
                    {
                        display_mode = SHOW_TIME;
                        display_time (datetime_current);
                        eeprom_update_byte (EE_ADDR_DISPLAY_MODE, display_mode);
                        uart_ok (1);
                    }
                    else if (uart_str[8]  == 't' &&
                             uart_str[9]  == 'e' &&
                             uart_str[10] == 'm' &&
                             uart_str[11] == 'p'
                             )
                    {
                        display_mode = SHOW_TEMP;
                        display_temp (temp_value);
                        eeprom_update_byte (EE_ADDR_DISPLAY_MODE, display_mode);
                        uart_ok (1);
                    }
                    else
                    {
                        uart_ok (0);
                    }
                }
                else
                {
                    uart_ok (0);
                }
                uart_str_indx = 0;
            }
            uart_chr = uart_getc ();
        }
    }
}
