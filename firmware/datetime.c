/* Name: datetime.c
 * Project: aquarium
 * Author: Baranovskiy Konstantin
 * Creation Date: 2015-12-28
 * Tabsize: 4
 * Copyright: (c) 2015 Baranovskiy Konstantin
 * License: GNU GPL v3 (see License.txt)
 * This Revision: 1
 */

#include "datetime.h"

/*
 * Get count of days for datetime.
 * Years counts from 00 to 99 (see ds1302 datasheet).
 */
int32_t datetime_days_count (datetime_t *datetime)
{
    uint8_t i;
    int32_t days = 0;
    // Days of full years
    days += datetime->year * 365;
    // Leap-days for all years except current
    if (datetime->year > 0)
        days += (datetime->year - 1) / 4 + 1;
    // Days of months
    for (i=1; i < datetime->month; i++)
    {
        days += days_in_month[i];
        if (i == 2 && (datetime->year == 0 || (datetime->year % 4) == 0))
            days++;
    }
    // Days of the current month
    days += datetime->day;

    return days;
}

/*
 * Subtraction of two datetimes in days.
 */
int32_t datetime_diff_in_days (datetime_t *datetime1, datetime_t *datetime2)
{
    int32_t datetime1_days = datetime_days_count(datetime1);
    int32_t datetime2_days = datetime_days_count(datetime2);
    return datetime1_days - datetime2_days;
}

/*
 * Add the time to the datetime.
 */
void datetime_add_time (datetime_t *datetime, time_t *time)
{
    uint8_t tmp;
    datetime->sec += time->sec;
    if (datetime->sec >= 60)
    {
        datetime->sec -= 60;
        datetime->min += 1;
    }

    datetime->min += time->min;
    if (datetime->min >= 60)
    {
        datetime->min -= 60;
        datetime->hour += 1;
    }

    datetime->hour += time->hour;
    if (datetime->hour >= 24)
    {
        datetime->hour -= 24;
        datetime->weekday += 1;
        datetime->day += 1;
    }

    if (datetime->weekday > 7)
        datetime->weekday = 1;

    // Days count in current month
    tmp = days_in_month[datetime->month];
    // Leap-day
    if (datetime->month == 2 && ((datetime->year % 4) == 0 || datetime->year == 0))
        tmp++;
    if (datetime->day > tmp)
    {
        datetime->day = 1;
        datetime->month += 1;
    }

    if (datetime->month > 12)
    {
        datetime->month = 1;
        datetime->year += 1;
    }

    if (datetime->year > 99)
        datetime->year = 0;
}

/*
 * Subtract the time from the datetime.
 */
void datetime_sub_time (datetime_t *datetime, time_t *time)
{
    uint8_t tmp;
    if (datetime->sec < time->sec)
    {
        datetime->sec = 60 + datetime->sec - time->sec;
        tmp = 1;
    }
    else
    {
        datetime->sec -= time->sec;
        tmp = 0;
    }

    if (datetime->min < (time->min + tmp))
    {
        datetime->min = 60 + datetime->min - time->min - tmp;
        tmp = 1;
    }
    else
    {
        datetime->min -= time->min + tmp;
        tmp = 0;
    }

    if (datetime->hour < (time->hour + tmp))
    {
        datetime->hour = 24 + datetime->hour - time->hour - tmp;
        tmp = 1;
    }
    else
    {
        datetime->hour -= time->hour + tmp;
        tmp = 0;
    }

    if (tmp)
    {
        if (datetime->day > 1)
        {
            datetime->day -= 1;
        }
        else
        {
            if (datetime->month > 1)
            {
                datetime->month -= 1;
            }
            else
            {
                datetime->month = 12;

                if (datetime->year > 0)
                    datetime->year -= 1;
                else
                    datetime->year = 99;
            }

            // Days count in current month
            tmp = days_in_month[datetime->month];
            // Leap-day
            if (datetime->month == 2 && ((datetime->year % 4) == 0 || datetime->year == 0))
            tmp++;

            datetime->day = tmp;
        }
    }
}

