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

static long string_length(const char *string)
{
    long length = 0;

    while (string[length])
    {
        length++;
    }

    return length;
}

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        struct utsname system_info;

        if (sys_uname(&system_info) < 0)
        {
            return 1;
        }

        sys_write(
            1,
            system_info.nodename,
            string_length(system_info.nodename)
        );

        sys_write(1, "\n", 1);

        return 0;
    }

    if (argc == 2)
    {
        const char *hostname = argv[1];
        long length = string_length(hostname);

        if (sys_sethostname(hostname, length) < 0)
        {
            return 1;
        }

        return 0;
    }

    sys_write(
        2,
        "hostname: too many arguments\n",
        29
    );

    return 1;
}