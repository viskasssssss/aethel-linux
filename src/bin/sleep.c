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

#include "syscall.h"

static long parse_number(const char *string)
{
    long value = 0;

    for (long i = 0; string[i]; i++)
    {
        if (string[i] < '0' || string[i] > '9')
        {
            return -1;
        }

        value = value * 10 + (string[i] - '0');
    }

    return value;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        sys_write(
            2,
            "sleep: usage: sleep seconds\n",
            29
        );

        return 1;
    }

    long seconds = parse_number(argv[1]);

    if (seconds < 0)
    {
        sys_write(
            2,
            "sleep: invalid time\n",
            21
        );

        return 1;
    }

    struct timespec request;

    request.seconds = seconds;
    request.nanoseconds = 0;

    if (sys_nanosleep(&request, 0) < 0)
    {
        return 1;
    }

    return 0;
}