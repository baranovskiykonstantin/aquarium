/* Name: aquarium.c
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2019-08-25
 * Tabsize: 4
 * Copyright: (c) 2019 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 */

#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>

#include "aquarium.h"
#include "datetime.h"
#include "display.h"
#include "adc.h"
#include "pwm.h"
#include "ds18b20.h"
#include "ds1302.h"
#include "uart.h"

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
 * Displaying modes
 */
#define SHOW_TIME 1
#define SHOW_TEMP 2

/*
 * Operating modes
 */
#define MODE_AUTO 'a'
#define MODE_MANUAL 'm'

/*
 * UART responses
 */
#define OK 2
#define ERROR 4
#define UNKNOWN 8

static struct
{
    // Displaying mode
    uint8_t display;

    // Temperature of the water
    int8_t temperature;

    struct
    {
        // Current date and time
        datetime_t now;
        // Date and time of last time adjustment
        datetime_t adjusted;
        // Time correction value
        time_t correction;
    } clock;

    struct
    {
        // Lighting mode
        uint8_t mode;
        // Time of turning light on
        time_t time_on;
        // Time of turning light off
        time_t time_off;
        // Brightness of light in percent
        uint8_t level;
        // Time of light rising from 0 to 100 % in minutes
        uint8_t risetime;
    } light;

    struct
    {
        // Heating mode
        uint8_t mode;
        // Temperature of turning heater on
        int8_t temp_l;
        // Temperature of turning heater off
        int8_t temp_h;
    } heater;

    struct
    {
        // Characters received from UART
        uint8_t buffer[UART_RX_BUFFER_SIZE];
        // Position in buffer for storing next received char from UART
        uint8_t index;
    } uart;

} aquarium;

static struct {
    uint8_t temp_l;
    uint8_t temp_h;

    uint8_t time_on_hour;
    uint8_t time_on_min;
    uint8_t time_on_sec;

    uint8_t time_off_hour;
    uint8_t time_off_min;
    uint8_t time_off_sec;

    uint8_t daily_corr_sign;
    uint8_t daily_corr_sec;

    uint8_t heat_mode;
    uint8_t light_mode;
    uint8_t display_mode;

    uint8_t light_level;
    uint8_t light_rise_time;

} config EEMEM = {
// "config" stores some important values of "aquarium" structure in EEPROM.
// Default values:
    22, 25,
    8, 0, 0,
    18, 0, 0,
    '+', 0,
    MODE_AUTO,
    MODE_AUTO,
    SHOW_TIME,
    50, 15
};

static const char help_msg[] PROGMEM = "\r\nAvailable commands:\r\n\r\n"
                                       "status\r\n"
                                       "date DD.MN.YY W\r\n"
                                       "time HH:MM:SS\r\n"
                                       "time +CC\r\n"
                                       "time -CC\r\n"
                                       "time HH:MM:SS +CC\r\n"
                                       "time HH:MM:SS -CC\r\n"
                                       "heat LO-HI\r\n"
                                       "heat on\r\n"
                                       "heat off\r\n"
                                       "heat auto\r\n"
                                       "light H1:M1:S1-H2:M2:S2\r\n"
                                       "light level LLL\r\n"
                                       "light rise RR\r\n"
                                       "light H1:M1:S1-H2:M2:S2 LLL RR\r\n"
                                       "light on\r\n"
                                       "light off\r\n"
                                       "light auto\r\n"
                                       "display time\r\n"
                                       "display temp\r\n"
                                       "reboot\r\n"
                                       "help\r\n\r\n";

/* ------------------------------------------------------------------------- *
 * Send response about command processing to UART
 * ------------------------------------------------------------------------- */
static void uart_response(uint8_t response)
{
    switch (response)
    {
        case OK:
            uart_puts("OK\r\n");
            break;
        case ERROR:
            uart_puts("ERROR\r\n");
            break;
        default:
            uart_puts("UNKNOWN\r\n");
    }
}

/* ------------------------------------------------------------------------- *
 * Send int to UART as ASCII
 * ------------------------------------------------------------------------- */
static void uart_puti(int8_t value, uint8_t padding_zeros)
{
    if (value < 0 )
    {
        uart_putc('-');
    }

    if (labs(value) >= 100 || padding_zeros >= 3)
    {
        uart_putc(labs(value) / 100 + 0x30);
    }

    if (labs(value) >= 10 || padding_zeros >= 2)
    {
        uart_putc(labs(value) % 100 / 10 + 0x30);
    }

    uart_putc(value % 10 + 0x30);
}

/* ------------------------------------------------------------------------- *
 * Check if char is digit
 * ------------------------------------------------------------------------- */
static uint8_t chr_is_digit(uint8_t chr)
{
    if (chr >= '0' && chr <= '9')
    {
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------- *
 * Extract integer from buffer
 * ------------------------------------------------------------------------- */
static uint8_t extract_int(uint8_t offset, uint8_t len)
{
    uint8_t value = 0;
    uint8_t i;

    for (i = 0; i < len; i++)
    {
        value *= 10;
        value += (aquarium.uart.buffer[offset + i] - 0x30);
    }

    return value;
}

/* ------------------------------------------------------------------------- *
 * Extract time from UART buffer
 * ------------------------------------------------------------------------- */
static void extract_time(uint8_t offset)
{
    uint8_t value;

    value = extract_int(offset, 2);
    aquarium.clock.now.hour = value > 23 ? 23 : value;

    value = extract_int((offset + 3), 2);
    aquarium.clock.now.min =value > 59 ? 59 : value;

    value = extract_int((offset + 6), 2);
    aquarium.clock.now.sec = value > 59 ? 59 : value;

    aquarium.clock.now.H12_24 = H24;
    aquarium.clock.now.AMPM = AM;

    aquarium.clock.adjusted = aquarium.clock.now;

    ds1302_write_datetime(&(aquarium.clock.now));
    ds1302_write_datetime_to_ram(&(aquarium.clock.adjusted), 0);
}

/* ------------------------------------------------------------------------- *
 * Extract time correction from UART buffer
 * ------------------------------------------------------------------------- */
static void extract_time_correction(uint8_t offset)
{
    uint8_t value;

    value = aquarium.uart.buffer[offset];
    switch (value)
    {
        case '+':
        case '-':
            aquarium.clock.correction.AMPM = value;
            break;
        default:
            aquarium.clock.correction.AMPM = '+';
    }

    value = extract_int((offset + 1), 2);
    aquarium.clock.correction.sec = value > 59 ? 59 : value;

    eeprom_update_byte(&(config.daily_corr_sign), aquarium.clock.correction.AMPM);
    eeprom_update_byte(&(config.daily_corr_sec), aquarium.clock.correction.sec);
}

/* ------------------------------------------------------------------------- *
 * Extract light thresholds from UART buffer
 * ------------------------------------------------------------------------- */
static void extract_light_thresholds(uint8_t offset)
{
    uint8_t value;

    value = extract_int(offset, 2);
    aquarium.light.time_on.hour = value > 23 ? 23 : value;

    value = extract_int((offset + 3), 2);
    aquarium.light.time_on.min = value > 59 ? 59 : value;

    value = extract_int((offset + 6), 2);
    aquarium.light.time_on.sec = value > 59 ? 59 : value;

    value = extract_int((offset + 9), 2);
    aquarium.light.time_off.hour = value > 23 ? 23 : value;

    value = extract_int((offset + 12), 2);
    aquarium.light.time_off.min = value > 59 ? 59 : value;

    value = extract_int((offset + 15), 2);
    aquarium.light.time_off.sec = value > 59 ? 59 : value;

    eeprom_update_byte(&(config.time_on_hour), aquarium.light.time_on.hour);
    eeprom_update_byte(&(config.time_on_min), aquarium.light.time_on.min);
    eeprom_update_byte(&(config.time_on_sec), aquarium.light.time_on.sec);

    eeprom_update_byte(&(config.time_off_hour), aquarium.light.time_off.hour);
    eeprom_update_byte(&(config.time_off_min), aquarium.light.time_off.min);
    eeprom_update_byte(&(config.time_off_sec), aquarium.light.time_off.sec);
}

/* ------------------------------------------------------------------------- *
 * Extract brightness level of light from UART buffer
 * ------------------------------------------------------------------------- */
static void extract_light_level(uint8_t offset)
{
    uint8_t value = extract_int(offset, 3);

    aquarium.light.level = value > 100 ? 100 : value;

    eeprom_update_byte(&(config.light_level), aquarium.light.level);
}

/* ------------------------------------------------------------------------- *
 * Extract light rise time from UART buffer
 * ------------------------------------------------------------------------- */
static void extract_light_risetime(uint8_t offset)
{
    uint8_t value = extract_int(offset, 2);

    aquarium.light.risetime = value > 30 ? 30 : value;

    eeprom_update_byte(&(config.light_rise_time), aquarium.light.risetime);
}

void aquarium_init(void)
{
    // Initialize I/O
    HEAT_AS_OUT;
    HEAT_OFF;
    DS18B20_PWR_AS_OUT;
    DS18B20_PWR_ON;
    SENSORS_PWR_AS_OUT;
    SENSORS_PWR_ON;

    // Initialize peripheries
    pwm_init();
    adc_init();
    display_init();
    ds1302_init();
    uart_init(UART_BAUD_SELECT(9600, F_CPU));

    // Enable interrupts
    sei();

    // Initialize aquarium data
    aquarium.temperature = DS18B20_ERR;
    aquarium.uart.index = 0;
    // Restore parameters from EEPROM
    aquarium.heater.temp_l = eeprom_read_byte(&(config.temp_l));
    aquarium.heater.temp_h = eeprom_read_byte(&(config.temp_h));

    aquarium.light.level = eeprom_read_byte(&(config.light_level));
    aquarium.light.risetime = eeprom_read_byte(&(config.light_rise_time));

    aquarium.light.time_on.hour = eeprom_read_byte(&(config.time_on_hour));
    aquarium.light.time_on.min = eeprom_read_byte(&(config.time_on_min));
    aquarium.light.time_on.sec = eeprom_read_byte(&(config.time_on_sec));

    aquarium.light.time_off.hour = eeprom_read_byte(&(config.time_off_hour));
    aquarium.light.time_off.min = eeprom_read_byte(&(config.time_off_min));
    aquarium.light.time_off.sec = eeprom_read_byte(&(config.time_off_sec));

    // Sign of the time correction is stored In AMPM
    aquarium.clock.correction.AMPM = eeprom_read_byte(&(config.daily_corr_sign));
    aquarium.clock.correction.sec = eeprom_read_byte(&(config.daily_corr_sec));

    aquarium.heater.mode = eeprom_read_byte(&(config.heat_mode));
    aquarium.light.mode = eeprom_read_byte(&(config.light_mode));
    aquarium.display = eeprom_read_byte(&(config.display_mode));

    // Calibrate sensors
    SENSORS_PWR_OFF;
    _delay_ms(10);
    SENSORS_PWR_ON;

    // Test display - all segments is on by default
    _delay_ms(500);

    // Read date and time of the last time correction from RAM of DS1302
    ds1302_read_datetime_from_ram(&(aquarium.clock.adjusted), 0);

    // Setup PWM
    pwm_setup(aquarium.light.level, aquarium.light.risetime);
}

void aquarium_process_time(void)
{
    ds1302_read_datetime(&(aquarium.clock.now));

    if (aquarium.display == SHOW_TIME)
    {
        display_time((time_t *)&(aquarium.clock.now));
    }

    // If current date greater then date of last correction more then one day
    // and if the current time greater than time of the setup time...
    if (date_is_longer(&(aquarium.clock.now), &(aquarium.clock.adjusted))
        && time_is_longer((time_t *)&(aquarium.clock.now), (time_t *)&(aquarium.clock.adjusted)))
    {
        // ... apply time correction.
        // Sign of time correction is stored in AMPM
        switch (aquarium.clock.correction.AMPM)
        {
            case '+':
                datetime_add_time(&(aquarium.clock.now), &(aquarium.clock.correction));
                break;
            case '-':
                datetime_sub_time(&(aquarium.clock.now), &(aquarium.clock.correction));
                break;
        }
        ds1302_write_datetime(&(aquarium.clock.now));
        // Save date and time of last correction to RAM of RTC
        aquarium.clock.adjusted.year = aquarium.clock.now.year;
        aquarium.clock.adjusted.month = aquarium.clock.now.month;
        aquarium.clock.adjusted.day = aquarium.clock.now.day;
        aquarium.clock.adjusted.weekday = aquarium.clock.now.weekday;
        ds1302_write_datetime_to_ram(&(aquarium.clock.adjusted), 0);
    }
}

void aquarium_process_sensors(void)
{
    uint8_t sensor1;
    static uint8_t prev_sensor1 = 0;
    uint8_t sensor2;

    sensor1 = adc_sensor_state(1);
    if (sensor1 != prev_sensor1)
    {
        prev_sensor1 = sensor1;
        if (sensor1 == 1)
        {
            sensor2 = adc_sensor_state(2);
            if (sensor2 == 0)
            {
                switch (aquarium.display)
                {
                    case SHOW_TIME:
                        aquarium.display = SHOW_TEMP;
                        display_temp(aquarium.temperature);
                        break;
                    case SHOW_TEMP:
                        aquarium.display = SHOW_TIME;
                        display_time((time_t *)&(aquarium.clock.now));
                        break;
                }
                eeprom_update_byte(&(config.display_mode), aquarium.display);
            }
            else
            {
                if (aquarium.display == SHOW_TIME)
                {
                    // Switch through lighting modes
                    if (aquarium.light.mode == MODE_AUTO)
                    {
                        // Switch mode to manual on
                        display_message_on();
                        aquarium.light.mode = MODE_MANUAL;
                        pwm_on();
                    }
                    else // MODE_MANUAL
                    {
                        if (pwm_status() & 0x80) // manual on
                        {
                            // Switch mode to manual off
                            display_message_off();
                            aquarium.light.mode = MODE_MANUAL;
                            pwm_off();
                        }
                        else // manual off
                        {
                            // Switch mode to auto
                            display_message_auto();
                            aquarium.light.mode = MODE_AUTO;
                        }
                    }
                    eeprom_update_byte(&(config.light_mode), aquarium.light.mode);
                }
                else // SHOW_TEMP
                {
                    // Switch through heating modes
                    if (aquarium.heater.mode == MODE_AUTO)
                    {
                        // Switch mode to manual on
                        display_message_on();
                        aquarium.heater.mode = MODE_MANUAL;
                        HEAT_ON;
                    }
                    else // MODE_MANUAL
                    {
                        if (HEAT_STATE) // manual on
                        {
                            // Switch mode to manual off
                            display_message_off();
                            aquarium.heater.mode = MODE_MANUAL;
                            HEAT_OFF;
                        }
                        else // manual off
                        {
                            // Switch mode to auto
                            display_message_auto();
                            aquarium.heater.mode = MODE_AUTO;
                        }
                    }
                    eeprom_update_byte(&(config.heat_mode), aquarium.heater.mode);
                }
            }
        }
    }
}

void aquarium_process_heat(void)
{
    int8_t temp;
    static uint8_t temp_fail_counter = 0;

    temp = ds18b20_get_temp();
    if (temp == DS18B20_ERR)
    {
        if (temp_fail_counter++ > 3)
        {
            aquarium.temperature = DS18B20_ERR;
            temp_fail_counter = 0;
            ds18b20_hard_reset();
        }
    }
    else if (temp != DS18B20_BUSY)
    {
        // DS18B20 returns default value "85" after powering - skip it!
        if (!(aquarium.temperature == DS18B20_ERR && temp == 85))
        {
            aquarium.temperature = temp;
        }
    }

    if (aquarium.display == SHOW_TEMP)
    {
        display_temp(aquarium.temperature);
    }

    if (aquarium.heater.mode == MODE_AUTO)
    {
        if (aquarium.temperature < aquarium.heater.temp_l)
        {
            HEAT_ON;
        }
        if (aquarium.temperature > aquarium.heater.temp_h)
        {
            HEAT_OFF;
        }
    }

    // Prevent overheating in any mode
    if (aquarium.temperature == DS18B20_ERR || aquarium.temperature > 35)
    {
        HEAT_OFF;
    }
}

void aquarium_process_light(void)
{
    if (aquarium.light.mode == MODE_AUTO)
    {
        if (time_is_longer((time_t *)&(aquarium.clock.now), &(aquarium.light.time_on))
            && time_is_longer(&(aquarium.light.time_off), (time_t *)&(aquarium.clock.now)))
        {
            pwm_rise();
        }
        else
        {
            pwm_fall();
        }
    }
}

void aquarium_process_uart(void)
{
    char const *cmd = (char *)aquarium.uart.buffer; // Just short alias

    uint16_t uart_chr;
    uint8_t value; // Variable for storing extracted from UART buffer values

    uart_chr = uart_getc();
    while ((uart_chr & 0xff00) == 0)
    {
        // Put char to buffer
        aquarium.uart.buffer[aquarium.uart.index++] = (uart_chr & 0xff);
        if (aquarium.uart.index >= UART_RX_BUFFER_SIZE)
        {
            aquarium.uart.index = 0;
            uart_puts("\r\nERROR\r\n");
            break;
        }

        // Backspace correction
        if (aquarium.uart.buffer[aquarium.uart.index - 1] == 0x08
            || aquarium.uart.buffer[aquarium.uart.index - 1] == 0x7f)
        {
            if (aquarium.uart.index < 2)
            {
                aquarium.uart.index = 0;
            }
            else
            {
                aquarium.uart.index -= 2;
                // Echo backspace - erase previous character
                uart_puts("\b \b");
            }
            break;
        }

        // Echo
        uart_putc(uart_chr);

        // Command processing
        // NOTE: cmd is the alias to aquarium.uart.buffer !!!
        if (cmd[aquarium.uart.index - 1] == '\n'
            || cmd[aquarium.uart.index - 1] == '\r')
        {
            if (cmd[aquarium.uart.index - 1] == '\n')
            {
                uart_putc('\r');
            }
            else
            {
                uart_putc('\n');
            }

            if (strncmp(cmd, "status", 6) == 0)
            {
                    uart_puts("Date: ");
                    uart_puti(aquarium.clock.now.day, 2);
                    uart_putc('.');
                    uart_puti(aquarium.clock.now.month, 2);
                    uart_putc('.');
                    uart_puti(aquarium.clock.now.year, 2);
                    uart_putc(' ');
                    switch (aquarium.clock.now.weekday)
                    {
                        case 1: uart_puts("Monday"); break;
                        case 2: uart_puts("Tuesday"); break;
                        case 3: uart_puts("Wednesday"); break;
                        case 4: uart_puts("Thursday"); break;
                        case 5: uart_puts("Friday"); break;
                        case 6: uart_puts("Saturday"); break;
                        case 7: uart_puts("Sunday"); break;
                    }

                    uart_puts("\r\nTime: ");
                    uart_puti(aquarium.clock.now.hour, 2);
                    uart_putc(':');
                    uart_puti(aquarium.clock.now.min, 2);
                    uart_putc(':');
                    uart_puti(aquarium.clock.now.sec, 2);
                    uart_puts(" (");
                    uart_putc(aquarium.clock.correction.AMPM);
                    uart_puti(aquarium.clock.correction.sec, 0);
                    uart_puts(" sec at ");
                    uart_puti(aquarium.clock.adjusted.hour, 2);
                    uart_putc(':');
                    uart_puti(aquarium.clock.adjusted.min, 2);
                    uart_putc(':');
                    uart_puti(aquarium.clock.adjusted.sec, 2);
                    uart_putc(')');

                    uart_puts("\r\nTemp: ");
                    if (aquarium.temperature == DS18B20_ERR)
                    {
                        uart_puts("--");
                    }
                    else
                    {
                        uart_puti(aquarium.temperature, 0);
                    }

                    uart_puts("\r\nHeat: ");
                    if (HEAT_STATE)
                    {
                        uart_puts("ON");
                    }
                    else
                    {
                        uart_puts("OFF");
                    }
                    switch (aquarium.heater.mode)
                    {
                        case MODE_AUTO: uart_puts(" auto "); break;
                        case MODE_MANUAL: uart_puts(" manual "); break;
                    }
                    uart_putc('(');
                    uart_puti(aquarium.heater.temp_l, 0);
                    uart_putc('-');
                    uart_puti(aquarium.heater.temp_h, 0);
                    uart_putc(')');

                    uart_puts("\r\nLight: ");
                    if (pwm_status() & 0x80)
                    {
                        uart_puts("ON");
                    }
                    else
                    {
                        uart_puts("OFF");
                    }
                    switch (aquarium.light.mode)
                    {
                        case MODE_AUTO: uart_puts(" auto "); break;
                        case MODE_MANUAL: uart_puts(" manual "); break;
                    }
                    uart_putc('(');
                    uart_puti(aquarium.light.time_on.hour, 2);
                    uart_putc(':');
                    uart_puti(aquarium.light.time_on.min, 2);
                    uart_putc(':');
                    uart_puti(aquarium.light.time_on.sec, 2);
                    uart_putc('-');
                    uart_puti(aquarium.light.time_off.hour, 2);
                    uart_putc(':');
                    uart_puti(aquarium.light.time_off.min, 2);
                    uart_putc(':');
                    uart_puti(aquarium.light.time_off.sec, 2);
                    uart_putc(')');
                    uart_putc(' ');
                    uart_puti(pwm_status() & 0x7f, 0);
                    uart_putc('/');
                    uart_puti(aquarium.light.level, 0);
                    uart_putc('%');
                    uart_putc(' ');
                    uart_puti(aquarium.light.risetime, 0);
                    uart_puts("min");

                    uart_puts("\r\nDisplay: ");
                    switch (aquarium.display)
                    {
                        case SHOW_TIME: uart_puts("time"); break;
                        case SHOW_TEMP: uart_puts("temp"); break;
                    }
                    uart_puts("\r\n");
            }
            else if (strncmp(cmd, "date ", 5) == 0 &&
                     chr_is_digit(cmd[5]) &&
                     chr_is_digit(cmd[6]) &&
                     cmd[7] == '.' &&
                     chr_is_digit(cmd[8]) &&
                     chr_is_digit(cmd[9]) &&
                     cmd[10] == '.' &&
                     chr_is_digit(cmd[11]) &&
                     chr_is_digit(cmd[12]) &&
                     cmd[13] == ' ' &&
                     chr_is_digit(cmd[14]))
            {
                value = extract_int(5, 2);
                aquarium.clock.now.day = value > 31 ? 31 : value;

                value = extract_int(8, 2);
                aquarium.clock.now.month = value > 12 ? 12 : value;

                aquarium.clock.now.year = extract_int(11, 2);

                value = extract_int(14, 1);
                aquarium.clock.now.weekday = value > 7 ? 7 : value;

                aquarium.clock.adjusted = aquarium.clock.now;

                ds1302_write_datetime(&(aquarium.clock.now));
                ds1302_write_datetime_to_ram(&(aquarium.clock.adjusted), 0);

                uart_response(OK);
            }
            else if (strncmp(cmd, "time ", 5) == 0)
            {
                if (chr_is_digit(cmd[5]) &&
                    chr_is_digit(cmd[6]) &&
                    cmd[7] == ':' &&
                    chr_is_digit(cmd[8]) &&
                    chr_is_digit(cmd[9]) &&
                    cmd[10] == ':' &&
                    chr_is_digit(cmd[11]) &&
                    chr_is_digit(cmd[12]))
                {
                    if (cmd[13] == ' ' &&
                        (cmd[14] == '+'|| cmd[14] == '-') &&
                        chr_is_digit(cmd[15]) &&
                        chr_is_digit(cmd[16]))
                    {
                        extract_time(5);
                        extract_time_correction(14);

                        uart_response(OK);
                    }
                    else if (cmd[13] == '\r' || cmd[13] == '\n')
                    {
                        extract_time(5);

                        uart_response(OK);
                    }
                    else
                    {
                        uart_response(ERROR);
                    }
                }
                else if ((cmd[5] == '+'|| cmd[5] == '-') &&
                         chr_is_digit(cmd[6]) &&
                         chr_is_digit(cmd[7]))
                {
                    extract_time_correction(5);

                    uart_response(OK);
                }
                else
                {
                    uart_response(ERROR);
                }
            }
            else if (strncmp(cmd, "heat ", 5) == 0)
            {
                if (chr_is_digit(cmd[5]) &&
                    chr_is_digit(cmd[6]) &&
                    cmd[7] == '-' &&
                    chr_is_digit(cmd[8]) &&
                    chr_is_digit(cmd[9]))
                {
                    value = extract_int(5, 2);
                    aquarium.heater.temp_l = value < 18 ? 18 : value;

                    value = extract_int(8, 2);
                    aquarium.heater.temp_h = value > 35 ? 35 : value;

                    eeprom_update_byte(&(config.temp_l), aquarium.heater.temp_l);
                    eeprom_update_byte(&(config.temp_h), aquarium.heater.temp_h);

                    uart_response(OK);
                }
                else if (strncmp(cmd+5, "on", 2) == 0)
                {
                    aquarium.heater.mode = MODE_MANUAL;
                    eeprom_update_byte(&(config.heat_mode), aquarium.heater.mode);
                    HEAT_ON;

                    uart_response(OK);
                }
                else if (strncmp(cmd+5, "off", 3) == 0)
                {
                    aquarium.heater.mode = MODE_MANUAL;
                    eeprom_update_byte(&(config.heat_mode), aquarium.heater.mode);
                    HEAT_OFF;

                    uart_response(OK);
                }
                else if (strncmp(cmd+5, "auto", 4) == 0)
                {
                    aquarium.heater.mode = MODE_AUTO;
                    eeprom_update_byte(&(config.heat_mode), aquarium.heater.mode);
                    uart_response(OK);
                }
                else
                {
                    uart_response(ERROR);
                }
            }
            else if (strncmp(cmd, "light ", 6) == 0)
            {
                if (chr_is_digit(cmd[6]) &&
                    chr_is_digit(cmd[7]) &&
                    cmd[8] == ':' &&
                    chr_is_digit(cmd[9]) &&
                    chr_is_digit(cmd[10]) &&
                    cmd[11] == ':' &&
                    chr_is_digit(cmd[12]) &&
                    chr_is_digit(cmd[13]) &&
                    cmd[14] == '-' &&
                    chr_is_digit(cmd[15]) &&
                    chr_is_digit(cmd[16]) &&
                    cmd[17] == ':' &&
                    chr_is_digit(cmd[18]) &&
                    chr_is_digit(cmd[19]) &&
                    cmd[20] == ':' &&
                    chr_is_digit(cmd[21]) &&
                    chr_is_digit(cmd[22]))
                {
                    if (cmd[23] == ' ' &&
                        chr_is_digit(cmd[24]) &&
                        chr_is_digit(cmd[25]) &&
                        chr_is_digit(cmd[26]) &&
                        cmd[27] == ' ' &&
                        chr_is_digit(cmd[28]) &&
                        chr_is_digit(cmd[29]))
                    {
                        extract_light_thresholds(6);
                        extract_light_level(24);
                        extract_light_risetime(28);

                        pwm_setup(aquarium.light.level, aquarium.light.risetime);

                        uart_response(OK);
                    }
                    else if (cmd[23] == '\r' || cmd[23] == '\n')
                    {
                        extract_light_thresholds(6);

                        uart_response(OK);
                    }
                    else
                    {
                        uart_response(ERROR);
                    }
                }
                else if (strncmp(cmd+6, "on", 2) == 0)
                {
                    aquarium.light.mode = MODE_MANUAL;
                    eeprom_update_byte(&(config.light_mode), aquarium.light.mode);
                    pwm_on();

                    uart_response(OK);
                }
                else if (strncmp(cmd+6, "off", 3) == 0)
                {
                    aquarium.light.mode = MODE_MANUAL;
                    eeprom_update_byte(&(config.light_mode), aquarium.light.mode);
                    pwm_off();

                    uart_response(OK);
                }
                else if (strncmp(cmd+6, "auto", 4) == 0)
                {
                    aquarium.light.mode = MODE_AUTO;
                    eeprom_update_byte(&(config.light_mode), aquarium.light.mode);

                    uart_response(OK);
                }
                else if (strncmp(cmd+6, "level ", 6) == 0 &&
                         chr_is_digit(cmd[12]) &&
                         chr_is_digit(cmd[13]) &&
                         chr_is_digit(cmd[14]))
                {
                    extract_light_level(12);

                    pwm_setup(aquarium.light.level, aquarium.light.risetime);

                    uart_response(OK);
                }
                else if (strncmp(cmd+6, "rise ", 5) == 0 &&
                         chr_is_digit(cmd[11]) &&
                         chr_is_digit(cmd[12]))
                {
                    extract_light_risetime(11);

                    pwm_setup(aquarium.light.level, aquarium.light.risetime);

                    uart_response(OK);
                }
                else
                {
                    uart_response(ERROR);
                }
            }
            else if (strncmp(cmd, "display ", 8) == 0)
            {
                if (strncmp(cmd+8, "time", 4) == 0)
                {
                    aquarium.display = SHOW_TIME;
                    eeprom_update_byte(&(config.display_mode), aquarium.display);

                    display_time((time_t *)&(aquarium.clock.now));

                    uart_response(OK);
                }
                else if (strncmp(cmd+8, "temp", 4) == 0)
                {
                    aquarium.display = SHOW_TEMP;
                    eeprom_update_byte(&(config.display_mode), aquarium.display);

                    display_temp(aquarium.temperature);

                    uart_response(OK);
                }
                else
                {
                    uart_response(ERROR);
                }
            }
            else if (strncmp(cmd, "reboot", 6) == 0)
            {
                uart_response(OK);

                HEAT_OFF;

                while (1); // watchdog will do the job
            }
            else if (strncmp(cmd, "help", 4) == 0)
            {
                uart_puts_p(help_msg);
            }
            else
            {
                uart_response(UNKNOWN);
            }
            aquarium.uart.index = 0;
        }
        uart_chr = uart_getc();
    }
}
