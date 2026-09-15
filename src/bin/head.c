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
    int lines = 10;
    const char *path = 0;

    if (argc == 2)
    {
        path = argv[1];
    }
    else if (argc == 4 &&
             argv[1][0] == '-' &&
             argv[1][1] == 'n' &&
             argv[1][2] == '\0')
    {
        lines = parse_number(argv[2]);

        if (lines < 0)
        {
            log_error("head: invalid number of lines\n");
            return 1;
        }

        path = argv[3];
    }
    else
    {
        log_error("head: usage: head [-n lines] <file>\n");
        return 1;
    }

    int fd = sys_openat(
        -100,
        path,
        0,
        0
    );

    if (fd < 0)
    {
        log_error("head: failed to open file\n");
        return 1;
    }

    char character;
    int current_line = 0;

    while (current_line < lines)
    {
        long result = sys_read(
            fd,
            &character,
            1
        );

        if (result <= 0)
        {
            break;
        }

        sys_write(
            1,
            &character,
            1
        );

        if (character == '\n')
        {
            current_line++;
        }
    }

    sys_close(fd);

    return 0;
}