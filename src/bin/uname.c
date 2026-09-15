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

static void print_string(const char *string)
{
    long length = 0;

    while (string[length])
    {
        length++;
    }

    sys_write(1, string, length);
}

static void print_field(const char *field, int *first)
{
    if (!*first)
    {
        sys_write(1, " ", 1);
    }

    print_string(field);

    *first = 0;
}

int main(int argc, char **argv)
{
    struct utsname system_info;

    if (sys_uname(&system_info) < 0)
    {
        return 1;
    }

    /*
     * uname without arguments is equivalent to -s.
     */

    if (argc == 1)
    {
        print_string(system_info.sysname);
        sys_write(1, "\n", 1);

        return 0;
    }

    int first = 1;
    int all = 0;

    for (int i = 1; i < argc; i++)
    {
        const char *argument = argv[i];

        if (argument[0] != '-')
        {
            continue;
        }

        for (int j = 1; argument[j]; j++)
        {
            switch (argument[j])
            {
                case 'a':
                    all = 1;
                    break;

                case 's':
                    print_field(system_info.sysname, &first);
                    break;

                case 'n':
                    print_field(system_info.nodename, &first);
                    break;

                case 'r':
                    print_field(system_info.release, &first);
                    break;

                case 'v':
                    print_field(system_info.version, &first);
                    break;

                case 'm':
                    print_field(system_info.machine, &first);
                    break;

                case 'o':
                    print_field("GNU/Linux", &first);
                    break;
            }
        }
    }

    if (all)
    {
        first = 1;

        print_field(system_info.sysname, &first);
        print_field(system_info.nodename, &first);
        print_field(system_info.release, &first);
        print_field(system_info.version, &first);
        print_field(system_info.machine, &first);
        print_field("GNU/Linux", &first);
    }

    sys_write(1, "\n", 1);

    return 0;
}