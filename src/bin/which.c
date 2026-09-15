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

#define AT_FDCWD -100
#define O_RDONLY 0

static int file_exists(
    const char *path
)
{
    long fd = sys_openat(
        AT_FDCWD,
        path,
        O_RDONLY,
        0
    );

    if (fd < 0)
    {
        return 0;
    }

    sys_close(fd);

    return 1;
}

int main(
    int argc,
    char **argv,
    char **envp
)
{
    if (argc != 2)
    {
        log_error("usage: which <command>\n");
        return 1;
    }

    const char *command = argv[1];

    const char *path = 0;

    for (long i = 0; envp[i]; i++)
    {
        if (
            envp[i][0] == 'P' &&
            envp[i][1] == 'A' &&
            envp[i][2] == 'T' &&
            envp[i][3] == 'H' &&
            envp[i][4] == '='
        )
        {
            path = &envp[i][5];
            break;
        }
    }

    if (!path)
    {
        return 1;
    }

    long path_length = 0;

    while (path[path_length])
    {
        path_length++;
    }

    long start = 0;

    while (start <= path_length)
    {
        long end = start;

        while (
            end < path_length &&
            path[end] != ':'
        )
        {
            end++;
        }

        char directory[256];

        long length = end - start;

        for (long i = 0; i < length; i++)
        {
            directory[i] = path[start + i];
        }

        directory[length] = '\0';

        char full_path[512];

        long index = 0;

        for (long i = 0; directory[i]; i++)
        {
            full_path[index++] = directory[i];
        }

        full_path[index++] = '/';

        for (long i = 0; command[i]; i++)
        {
            full_path[index++] = command[i];
        }

        full_path[index] = '\0';

        if (file_exists(full_path))
        {
            log_write(full_path);
            log_write("\n");

            return 0;
        }

        start = end + 1;
    }

    return 1;
}