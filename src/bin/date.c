/*
Copyright (C) 2026 viskasssssss

This file is part of Aethel.

Aethel is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published
by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

Aethel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Aethel. If not, see <https://www.gnu.org/licenses/>.
*/

#include "log.h"
#include "syscall.h"

int main(void)
{
    struct timespec time;

    long result = sys_clock_gettime(
        CLOCK_REALTIME,
        &time
    );

    if (result < 0)
    {
        log_error("date: clock_gettime failed\n");
        return 1;
    }

    long seconds = time.seconds;

    long second = seconds % 60;
    long minute = (seconds / 60) % 60;
    long hour = (seconds / 3600) % 24;

    long days = seconds / 86400;

    long weekday = (days + 4) % 7;

    const char *weekdays[] = {
        "Sun",
        "Mon",
        "Tue",
        "Wed",
        "Thu",
        "Fri",
        "Sat"
    };

    const char *months[] = {
        "Jan",
        "Feb",
        "Mar",
        "Apr",
        "May",
        "Jun",
        "Jul",
        "Aug",
        "Sep",
        "Oct",
        "Nov",
        "Dec"
    };

    long year = 1970;

    while (1)
    {
        long leap = 0;

        if (year % 4 == 0)
        {
            leap = 1;
        }

        if (year % 100 == 0)
        {
            leap = 0;
        }

        if (year % 400 == 0)
        {
            leap = 1;
        }

        long days_in_year = 365 + leap;

        if (days < days_in_year)
        {
            break;
        }

        days -= days_in_year;
        year++;
    }

    long month_days[] = {
        31,
        28,
        31,
        30,
        31,
        30,
        31,
        31,
        30,
        31,
        30,
        31
    };

    if (
        year % 4 == 0 &&
        (year % 100 != 0 || year % 400 == 0)
    )
    {
        month_days[1] = 29;
    }

    long month = 0;

    while (days >= month_days[month])
    {
        days -= month_days[month];
        month++;
    }

    long day = days + 1;

    log_write(weekdays[weekday]);
    log_write(" ");

    log_write(months[month]);
    log_write(" ");

    log_number(day);
    log_write(" ");

    log_number_padded(hour, 2);
    log_write(":");

    log_number_padded(minute, 2);
    log_write(":");

    log_number_padded(second, 2);

    log_write(" ");

    log_number(year);
    log_write("\n");

    return 0;
}