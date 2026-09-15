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

static int string_equals(
    const char *left,
    const char *right
)
{
    int i = 0;

    while (left[i] && right[i])
    {
        if (left[i] != right[i])
        {
            return 0;
        }

        i++;
    }

    return left[i] == right[i];
}

static void print_number(long number)
{
    char buffer[32];
    int position = 0;

    if (number == 0)
    {
        char zero = '0';

        sys_write(
            1,
            &zero,
            1
        );

        return;
    }

    while (number > 0)
    {
        buffer[position++] =
            '0' + (number % 10);

        number /= 10;
    }

    for (int i = position - 1; i >= 0; i--)
    {
        sys_write(
            1,
            &buffer[i],
            1
        );
    }
}

int main(int argc, char **argv)
{
    int mode = 0;
    const char *path = 0;

    if (argc == 2)
    {
        if (string_equals(argv[1], "-l") ||
            string_equals(argv[1], "-w") ||
            string_equals(argv[1], "-c"))
        {
            if (string_equals(argv[1], "-l"))
            {
                mode = 1;
            }
            else if (string_equals(argv[1], "-w"))
            {
                mode = 2;
            }
            else
            {
                mode = 3;
            }
        }
        else
        {
            path = argv[1];
        }
    }
    else if (argc == 3)
    {
        if (string_equals(argv[1], "-l"))
        {
            mode = 1;
        }
        else if (string_equals(argv[1], "-w"))
        {
            mode = 2;
        }
        else if (string_equals(argv[1], "-c"))
        {
            mode = 3;
        }
        else
        {
            log_error("wc: unsupported option\n");
            return 1;
        }

        path = argv[2];
    }
    else if (argc != 1)
    {
        log_error("wc: usage: wc [-l|-w|-c] [file]\n");
        return 1;
    }

    int fd;

    if (path)
    {
        fd = sys_openat(
            -100,
            path,
            0,
            0
        );

        if (fd < 0)
        {
            log_error("wc: failed to open file\n");
            return 1;
        }
    }
    else
    {
        fd = 0;
    }

    long lines = 0;
    long words = 0;
    long bytes = 0;

    int inside_word = 0;

    char buffer[4096];

    while (1)
    {
        long result = sys_read(
            fd,
            buffer,
            sizeof(buffer)
        );

        if (result <= 0)
        {
            break;
        }

        bytes += result;

        for (long i = 0; i < result; i++)
        {
            char character = buffer[i];

            if (character == '\n')
            {
                lines++;
            }

            if (character == ' ' ||
                character == '\n' ||
                character == '\t' ||
                character == '\r')
            {
                inside_word = 0;
            }
            else if (!inside_word)
            {
                words++;
                inside_word = 1;
            }
        }
    }

    if (path)
    {
        sys_close(fd);
    }

    if (mode == 1)
    {
        print_number(lines);
    }
    else if (mode == 2)
    {
        print_number(words);
    }
    else if (mode == 3)
    {
        print_number(bytes);
    }
    else
    {
        print_number(lines);

        char space = ' ';
        sys_write(1, &space, 1);

        print_number(words);
        sys_write(1, &space, 1);

        print_number(bytes);
    }

    char newline = '\n';
    sys_write(1, &newline, 1);

    return 0;
}