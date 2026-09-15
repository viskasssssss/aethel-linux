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

int main(
    int argc,
    char **argv
)
{
    if (argc < 2)
    {
        return 0;
    }

    const char *format = argv[1];
    long argument = 2;

    for (long i = 0; format[i]; i++)
    {
        if (format[i] == '\\')
        {
            i++;

            if (!format[i])
            {
                break;
            }

            if (format[i] == 'n')
            {
                log_write("\n");
            }
            else if (format[i] == 't')
            {
                log_write("\t");
            }
            else if (format[i] == '\\')
            {
                log_write("\\");
            }
            else
            {
                log_write_length(
                    "\\",
                    1
                );

                log_write_length(
                    &format[i],
                    1
                );
            }

            continue;
        }

        if (format[i] != '%')
        {
            log_write_length(
                &format[i],
                1
            );

            continue;
        }

        i++;

        if (!format[i])
        {
            break;
        }

        if (format[i] == '%')
        {
            log_write("%");
        }
        else if (format[i] == 's')
        {
            if (argument < argc)
            {
                log_write(argv[argument]);
                argument++;
            }
        }
        else if (format[i] == 'd')
        {
            if (argument < argc)
            {
                long number = 0;
                const char *value = argv[argument];

                long sign = 1;

                if (*value == '-')
                {
                    sign = -1;
                    value++;
                }

                while (*value >= '0' && *value <= '9')
                {
                    number = number * 10 + (*value - '0');
                    value++;
                }

                log_number(number * sign);

                argument++;
            }
        }
        else if (format[i] == 'c')
        {
            if (argument < argc)
            {
                log_write_length(
                    argv[argument],
                    1
                );

                argument++;
            }
        }
        else
        {
            log_write_length(
                "%",
                1
            );

            log_write_length(
                &format[i],
                1
            );
        }
    }

    return 0;
}