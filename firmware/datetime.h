/* Name: datetime.h
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2015 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 * This Revision: 1
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
    uint8_t     weekday;
    uint8_t     day;
    uint8_t     month;
    uint8_t     year;
    } date_t;

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

const static uint8_t days_in_month[] = {0, 31, 28, 31,30,31,30,31,31,30,31,30,31};

/*
 * Subtraction of two datetimes in days.
 */
int32_t datetime_diff_in_days (datetime_t *datetime1, datetime_t *datetime2);

/*
 * Add the time to the datetime.
 */
void datetime_add_time (datetime_t *datetime, time_t *time);

/*
 * Subtract the time from the datetime.
 */
void datetime_sub_time (datetime_t *datetime, time_t *time);

#endif /* __DATETIME_H_INCLUDED__ */