/* Name: datetime.h
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2015-2019 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 */

#ifndef __DATETIME_H_INCLUDED__
#define __DATETIME_H_INCLUDED__

#include <avr/io.h>

#define AM 0
#define PM 0b00100000

#define H12 0b10000000
#define H24 0

typedef struct
{
    uint8_t     sec;
    uint8_t     min;
    uint8_t     hour;
    uint8_t     AMPM;
    uint8_t     H12_24;
} time_t;

typedef struct
{
    uint8_t     sec;
    uint8_t     min;
    uint8_t     hour;
    uint8_t     AMPM;
    uint8_t     H12_24;
    uint8_t     weekday;
    uint8_t     day;
    uint8_t     month;
    uint8_t     year;
} datetime_t;

/*
 * Add time to datetime.
 */
extern void datetime_add_time(datetime_t *datetime, time_t *time);

/*
 * Subtract time from datetime.
 */
extern void datetime_sub_time(datetime_t *datetime, time_t *time);

/*
 * Check if date of datetime1 is longer than date of datetime2.
 */
extern uint8_t date_is_longer(datetime_t *datetime1, datetime_t *datetime2);

/*
 * Check if time1 is longer than time2.
 */
extern uint8_t time_is_longer(time_t *time1, time_t *time2);

#endif /* __DATETIME_H_INCLUDED__ */
