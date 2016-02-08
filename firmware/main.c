/* Name: main.c
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2015 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 * This Revision: 1
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdlib.h>
#include <math.h>

#include "display.h"
#include "ds18b20.h"
#include "ds1302.h"
#include "uart.h"

/*
 * Addresses of the parameters in EEPROM
 */
#define EE_ADDR_TEMP_L (uint8_t *)0
#define EE_ADDR_TEMP_H (uint8_t *)1

#define EE_ADDR_TIME_ON_SEC (uint8_t *)2
#define EE_ADDR_TIME_ON_MIN (uint8_t *)3
#define EE_ADDR_TIME_ON_HOUR (uint8_t *)4

#define EE_ADDR_TIME_OFF_SEC (uint8_t *)5
#define EE_ADDR_TIME_OFF_MIN (uint8_t *)6
#define EE_ADDR_TIME_OFF_HOUR (uint8_t *)7

/*
 * I/O configuration
 */
#define HEAT_AS_OUT DDRC |= (1 << PC5)
#define HEAT_ON PORTC |= (1 << PC5)
#define HEAT_OFF PORTC &= ~(1 << PC5)
#define HEAT_STATE (PINC & (1 << PC5)) >> PC5

#define LIGHT_AS_OUT DDRC |= (1 << PC4)
#define LIGHT_ON PORTC |= (1 << PC4)
#define LIGHT_OFF PORTC &= ~(1 << PC4)
#define LIGHT_STATE (PINC & (1 << PC4)) >> PC4

#define SENSOR_AS_IN DDRD &= ~(1 << PD4)
#define SENSOR_PULLUP_ENABLE PORTD |= (1 << PD4)
#define SENSOR_PULLUP_DISABLE PORTD &= ~(1 << PD4)
#define SENSOR_STATE (PIND & (1 << PD4)) >> PD4

/*
 * Modes of the display
 */
enum {SHOW_TIME, SHOW_TEMP};

/*
 * Buffer for storing data received from UART
 */
uint8_t uart_str[UART_RX_BUFFER_SIZE];

/* ---------------------- Send int to UART as ASCII ------------------------ */
void uart_puti (int8_t value)
{
    if (value < 0 )
        uart_putc('-');
    if (labs(value) >= 100)
        uart_putc(labs(value) / 100 + 0x30);
    uart_putc(labs(value) % 100 / 10 + 0x30);
    uart_putc(value % 10 + 0x30);
}

/* ------------------------- Get int from buffer --------------------------- */
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

/* ----------------------- Convert char case to low ------------------------ */
uint8_t chr_to_lower (uint8_t chr)
{
    return (chr | 0x20);
}

/* ------------------------ Check if char is digit ------------------------- */
uint8_t chr_is_digit (uint8_t chr)
{
    if (chr >= '0' && chr <= '9')
        return 1;
    else
        return 0;
}

/* ------------------------------ UART ok ---------------------------------- */
void uart_ok (void)
{
    uart_puts("OK\r\n");
}

/* =============== Main function - start point of the program ============== */
int main (void)
{
    uint16_t uart_chr;
    uint8_t sensor_prev_state = 0;
    uint8_t display_state = SHOW_TIME;
    uint32_t time_sec, time_sec_on, time_sec_off;
    uint8_t temp_fail_counter = 0;
    double temp_tmp;
    double temp_value = DS18B20_ERR;
    int32_t temp_int = lround(DS18B20_ERR);
    int8_t temp_h, temp_l;
    datetime_t current_datetime;
    time_t time_on, time_off;
    uint8_t uart_str_indx = 0;

    temp_l = eeprom_read_byte(EE_ADDR_TEMP_L);
    temp_h = eeprom_read_byte(EE_ADDR_TEMP_H);

    time_on.sec = eeprom_read_byte(EE_ADDR_TIME_ON_SEC);
    time_on.min = eeprom_read_byte(EE_ADDR_TIME_ON_MIN);
    time_on.hour = eeprom_read_byte(EE_ADDR_TIME_ON_HOUR);
    time_on.AMPM = AM;
    time_on.H12_24 = H24;

    time_off.sec = eeprom_read_byte(EE_ADDR_TIME_OFF_SEC);
    time_off.min = eeprom_read_byte(EE_ADDR_TIME_OFF_MIN);
    time_off.hour = eeprom_read_byte(EE_ADDR_TIME_OFF_HOUR);
    time_off.AMPM = AM;
    time_off.H12_24 = H24;

    // Initialize I/O
    LIGHT_AS_OUT;
    HEAT_AS_OUT;

    display_init();                 // Initialize the display
    ds1302_init();                  // Initialize the RTC
    uart_init(UART_BAUD_SELECT(9600, F_CPU)); // Initialize the UART

    sei();                          // Enable all interrupts

    while (1)
    {
        // Time
        current_datetime = ds1302_read_datetime();

        // Light
        time_sec = current_datetime.hour * 3600L + current_datetime.min * 60L + current_datetime.sec;
        time_sec_on = time_on.hour * 3600L + time_on.min * 60L + time_on.sec;
        time_sec_off = time_off.hour * 3600L + time_off.min * 60L + time_off.sec;

        if (time_sec >= time_sec_on && time_sec <= time_sec_off)
        {
            if (!(LIGHT_STATE))
                LIGHT_ON;
        }
        else if (LIGHT_STATE)
        {
            LIGHT_OFF;
        }

        // Temp
        temp_tmp = ds18b20_gettemp();
        if (temp_tmp == DS18B20_ERR)
        {
            if (temp_fail_counter < 5)
            {
                temp_fail_counter += 1;
            }
            else
            {
                temp_value = temp_tmp;
                temp_int = lround(temp_value);
                temp_fail_counter = 0;
            }
        }
        else
        {
            temp_value = temp_tmp;
            temp_int = lround(temp_value);
            temp_fail_counter = 0;
        }

        // Heat
        if (temp_value == DS18B20_ERR)
        {
            HEAT_OFF;
        }
        else
        {
            if ((temp_int < temp_l) && !(HEAT_STATE))
                HEAT_ON;
            if ((temp_int > temp_h) && (HEAT_STATE))
                HEAT_OFF;
        }

        // Sensor
        if (SENSOR_STATE != sensor_prev_state)
        {
            sensor_prev_state = SENSOR_STATE;
            if (SENSOR_STATE)
            {
                switch (display_state)
                {
                    case SHOW_TIME: display_state = SHOW_TEMP; break;
                    case SHOW_TEMP: display_state = SHOW_TIME; break;
                }
            }
        }

        // Display mode
        switch (display_state)
        {
            case SHOW_TIME: display_time(current_datetime); break;
            case SHOW_TEMP: display_temp(temp_value); break;
        }

        // UART
        uart_chr = uart_getc();
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
            if (uart_str[uart_str_indx - 1] == 0x08)
            {
                if (uart_str_indx < 2)
                    uart_str_indx = 0;
                else
                    uart_str_indx -= 2;
            }
            // Command processing
            if (uart_str[uart_str_indx - 1] == '\n')
            {
                if (chr_to_lower(uart_str[0]) == 's' &&
                    chr_to_lower(uart_str[1]) == 't' &&
                    chr_to_lower(uart_str[2]) == 'a' &&
                    chr_to_lower(uart_str[3]) == 't' &&
                    chr_to_lower(uart_str[4]) == 'e')
                {
                        uart_puti(current_datetime.day);
                        uart_putc('.');
                        uart_puti(current_datetime.month);
                        uart_puts(".20");
                        uart_puti(current_datetime.year);
                        uart_putc(' ');
                        switch (current_datetime.weekday)
                        {
                            case 1: uart_puts("Monday"); break;
                            case 2: uart_puts("Tuesday"); break;
                            case 3: uart_puts("Wednesday"); break;
                            case 4: uart_puts("Thursday"); break;
                            case 5: uart_puts("Friday"); break;
                            case 6: uart_puts("Saturday"); break;
                            case 7: uart_puts("Sunday"); break;
                        }
                        uart_putc(' ');
                        uart_puti(current_datetime.hour);
                        uart_putc(':');
                        uart_puti(current_datetime.min);
                        uart_putc(':');
                        uart_puti(current_datetime.sec);
                        uart_puts("\r\nTemp: ");
                        if (temp_value == DS18B20_ERR)
                            uart_puts("--");
                        else
                            uart_puti((int8_t)temp_int);
                        uart_puts("\r\nHeat: ");
                        if (HEAT_STATE)
                            uart_puts("ON");
                        else
                            uart_puts("OFF");
                        uart_putc(' ');
                        uart_putc('(');
                        uart_puti(temp_l);
                        uart_putc('-');
                        uart_puti(temp_h);
                        uart_putc(')');
                        uart_puts("\r\nLight: ");
                        if (LIGHT_STATE)
                            uart_puts("ON");
                        else
                            uart_puts("OFF");
                        uart_putc(' ');
                        uart_putc('(');
                        uart_puti(time_on.hour);
                        uart_putc(':');
                        uart_puti(time_on.min);
                        uart_putc(':');
                        uart_puti(time_on.sec);
                        uart_putc('-');
                        uart_puti(time_off.hour);
                        uart_putc(':');
                        uart_puti(time_off.min);
                        uart_putc(':');
                        uart_puti(time_off.sec);
                        uart_putc(')');
                        uart_puts("\r\n");
                }
                else if (chr_to_lower(uart_str[0]) == 'd' &&
                         chr_to_lower(uart_str[1]) == 'a' &&
                         chr_to_lower(uart_str[2]) == 't' &&
                         chr_to_lower(uart_str[3]) == 'e' &&
                         uart_str[4] == ' ' &&
                         chr_is_digit(uart_str[5]) &&
                         chr_is_digit(uart_str[6]) &&
                         uart_str[7] == '.' &&
                         chr_is_digit(uart_str[8]) &&
                         chr_is_digit(uart_str[9]) &&
                         uart_str[10] == '.' &&
                         chr_is_digit(uart_str[11]) &&
                         chr_is_digit(uart_str[12]) &&
                         uart_str[13] == ' ' &&
                         chr_is_digit(uart_str[14]))
                {
                    current_datetime.day = uart_str_get_int(5, 2);
                    current_datetime.month = uart_str_get_int(8, 2);
                    current_datetime.year = uart_str_get_int(11, 2);
                    current_datetime.weekday = uart_str_get_int(14, 1);
                    ds1302_write_datetime(current_datetime);
                    uart_ok();
                }
                else if (chr_to_lower(uart_str[0]) == 't' &&
                         chr_to_lower(uart_str[1]) == 'i' &&
                         chr_to_lower(uart_str[2]) == 'm' &&
                         chr_to_lower(uart_str[3]) == 'e' &&
                         uart_str[4] == ' ' &&
                         chr_is_digit(uart_str[5]) &&
                         chr_is_digit(uart_str[6]) &&
                         uart_str[7] == ':' &&
                         chr_is_digit(uart_str[8]) &&
                         chr_is_digit(uart_str[9]) &&
                         uart_str[10] == ':' &&
                         chr_is_digit(uart_str[11]) &&
                         chr_is_digit(uart_str[12]))
                {
                    current_datetime.hour = uart_str_get_int(5, 2);
                    current_datetime.min = uart_str_get_int(8, 2);
                    current_datetime.sec = uart_str_get_int(11, 2);
                    current_datetime.H12_24 = H24;
                    current_datetime.AMPM = AM;
                    ds1302_write_datetime(current_datetime);
                    uart_ok();
                }
                else if (chr_to_lower(uart_str[0]) == 'h' &&
                         chr_to_lower(uart_str[1]) == 'e' &&
                         chr_to_lower(uart_str[2]) == 'a' &&
                         chr_to_lower(uart_str[3]) == 't' &&
                         uart_str[4] == ' ' &&
                         chr_is_digit(uart_str[5]) &&
                         chr_is_digit(uart_str[6]) &&
                         uart_str[7] == '-' &&
                         chr_is_digit(uart_str[8]) &&
                         chr_is_digit(uart_str[9]))
                {
                    temp_l = uart_str_get_int(5, 2);
                    temp_h = uart_str_get_int(8, 2);
                    eeprom_update_byte(EE_ADDR_TEMP_L, temp_l);
                    eeprom_update_byte(EE_ADDR_TEMP_H, temp_h);
                    uart_ok();
                }
                else if (chr_to_lower(uart_str[0]) == 'l' &&
                         chr_to_lower(uart_str[1]) == 'i' &&
                         chr_to_lower(uart_str[2]) == 'g' &&
                         chr_to_lower(uart_str[3]) == 'h' &&
                         chr_to_lower(uart_str[4]) == 't' &&
                         uart_str[5] == ' ' &&
                         chr_is_digit(uart_str[6]) &&
                         chr_is_digit(uart_str[7]) &&
                         uart_str[8] == ':' &&
                         chr_is_digit(uart_str[9]) &&
                         chr_is_digit(uart_str[10]) &&
                         uart_str[11] == ':' &&
                         chr_is_digit(uart_str[12]) &&
                         chr_is_digit(uart_str[13]) &&
                         uart_str[14] == '-' &&
                         chr_is_digit(uart_str[15]) &&
                         chr_is_digit(uart_str[16]) &&
                         uart_str[17] == ':' &&
                         chr_is_digit(uart_str[18]) &&
                         chr_is_digit(uart_str[19]) &&
                         uart_str[20] == ':' &&
                         chr_is_digit(uart_str[21]) &&
                         chr_is_digit(uart_str[22]))
                {
                    time_on.hour = uart_str_get_int(6, 2);
                    time_on.min = uart_str_get_int(9, 2);
                    time_on.sec = uart_str_get_int(12, 2);
                    time_off.hour = uart_str_get_int(15, 2);
                    time_off.min = uart_str_get_int(18, 2);
                    time_off.sec = uart_str_get_int(21, 2);
                    eeprom_update_byte(EE_ADDR_TIME_ON_HOUR, time_on.hour);
                    eeprom_update_byte(EE_ADDR_TIME_ON_MIN, time_on.min);
                    eeprom_update_byte(EE_ADDR_TIME_ON_SEC, time_on.sec);
                    eeprom_update_byte(EE_ADDR_TIME_OFF_HOUR, time_off.hour);
                    eeprom_update_byte(EE_ADDR_TIME_OFF_MIN, time_off.min);
                    eeprom_update_byte(EE_ADDR_TIME_OFF_SEC, time_off.sec);
                    uart_ok();
                }
                else
                {
                    uart_puts("ERROR\r\n");
                }
                uart_str_indx = 0;
            }
            uart_chr = uart_getc();
        }
        _delay_ms(100);
    }
    return 0;
}
