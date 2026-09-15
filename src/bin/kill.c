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

static int parse_number(const char *string)
{
    int number = 0;

    for (int i = 0; string[i]; i++)
    {
        if (string[i] < '0' || string[i] > '9')
        {
            return -1;
        }

        number = number * 10 + (string[i] - '0');
    }

    return number;
}

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 3)
    {
        log_error("kill: usage: kill [-9] <pid>\n");
        return 1;
    }

    int signal = 15;
    const char *pid_string = 0;

    if (argc == 2)
    {
        pid_string = argv[1];
    }
    else
    {
        if (argv[1][0] != '-' ||
            argv[1][1] != '9' ||
            argv[1][2] != '\0')
        {
            log_error("kill: unsupported signal\n");
            return 1;
        }

        signal = 9;
        pid_string = argv[2];
    }

    int pid = parse_number(pid_string);

    if (pid <= 0)
    {
        log_error("kill: invalid pid\n");
        return 1;
    }

    long result = sys_kill(pid, signal);

    if (result < 0)
    {
        log_error("kill: failed to send signal\n");
        return 1;
    }

    return 0;
}