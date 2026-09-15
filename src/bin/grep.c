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

static long string_length(const char *string)
{
    long length = 0;

    while (string[length])
    {
        length++;
    }

    return length;
}

static int contains(
    const char *line,
    long line_length,
    const char *pattern,
    long pattern_length
)
{
    if (pattern_length == 0)
    {
        return 1;
    }

    if (pattern_length > line_length)
    {
        return 0;
    }

    for (long i = 0;
         i <= line_length - pattern_length;
         i++)
    {
        int match = 1;

        for (long j = 0; j < pattern_length; j++)
        {
            if (line[i + j] != pattern[j])
            {
                match = 0;
                break;
            }
        }

        if (match)
        {
            return 1;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 3)
    {
        log_error("grep: usage: grep <pattern> [file]\n");
        return 2;
    }

    const char *pattern = argv[1];
    long pattern_length = string_length(pattern);

    int fd = 0;

    if (argc == 3)
    {
        fd = sys_openat(
            -100,
            argv[2],
            0,
            0
        );

        if (fd < 0)
        {
            log_error("grep: failed to open file\n");
            return 2;
        }
    }

    char line[4096];
    long line_length = 0;

    while (1)
    {
        char character;

        long result = sys_read(
            fd,
            &character,
            1
        );

        if (result <= 0)
        {
            if (line_length > 0)
            {
                if (contains(
                    line,
                    line_length,
                    pattern,
                    pattern_length))
                {
                    sys_write(
                        1,
                        line,
                        line_length
                    );
                }
            }

            break;
        }

        if (character == '\n')
        {
            if (contains(
                line,
                line_length,
                pattern,
                pattern_length))
            {
                sys_write(
                    1,
                    line,
                    line_length
                );

                sys_write(
                    1,
                    "\n",
                    1
                );
            }

            line_length = 0;
            continue;
        }

        if (line_length < (long)sizeof(line))
        {
            line[line_length++] = character;
        }
    }

    if (argc == 3)
    {
        sys_close(fd);
    }

    return 0;
}