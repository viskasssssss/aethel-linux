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
#define O_DIRECTORY 00200000

static long string_length(
    const char *string
)
{
    long length = 0;

    while (string[length])
    {
        length++;
    }

    return length;
}

static int string_equals(
    const char *a,
    const char *b
)
{
    long i = 0;

    while (a[i] && b[i])
    {
        if (a[i] != b[i])
        {
            return 0;
        }

        i++;
    }

    return a[i] == b[i];
}

static int pattern_match(
    const char *string,
    const char *pattern
)
{
    if (*pattern == '\0')
    {
        return *string == '\0';
    }

    if (*pattern == '*')
    {
        pattern++;

        if (*pattern == '\0')
        {
            return 1;
        }

        while (*string)
        {
            if (pattern_match(string, pattern))
            {
                return 1;
            }

            string++;
        }

        return pattern_match(string, pattern);
    }

    if (*pattern == '?')
    {
        if (!*string)
        {
            return 0;
        }

        return pattern_match(
            string + 1,
            pattern + 1
        );
    }

    if (*string != *pattern)
    {
        return 0;
    }

    return pattern_match(
        string + 1,
        pattern + 1
    );
}

static void find_directory(
    const char *path,
    const char *pattern,
    char type
)
{
    long path_length =
        string_length(path);

    const char *name = path;

    for (long i = path_length - 1; i >= 0; i--)
    {
        if (path[i] == '/')
        {
            name = &path[i + 1];
            break;
        }
    }

    int type_matches = 1;

    if (type == 'f')
    {
        type_matches = 0;
    }
    else if (type == 'd')
    {
        type_matches = 0;
    }

    long fd = sys_openat(
        AT_FDCWD,
        path,
        O_RDONLY | O_DIRECTORY,
        0
    );

    if (fd < 0)
    {
        return;
    }

    char buffer[4096];

    while (1)
    {
        long size = sys_getdents64(
            fd,
            buffer,
            sizeof(buffer)
        );

        if (size <= 0)
        {
            break;
        }

        long offset = 0;

        while (offset < size)
        {
            struct linux_dirent64 *entry =
                (struct linux_dirent64 *)(buffer + offset);

            if (
                !string_equals(entry->name, ".") &&
                !string_equals(entry->name, "..")
            )
            {
                char child[512];

                long name_length =
                    string_length(entry->name);

                long index = 0;

                for (long i = 0; i < path_length; i++)
                {
                    child[index++] = path[i];
                }

                if (
                    path_length > 0 &&
                    path[path_length - 1] != '/'
                )
                {
                    child[index++] = '/';
                }

                for (long i = 0; i < name_length; i++)
                {
                    child[index++] = entry->name[i];
                }

                child[index] = '\0';

                int matches = 1;

                if (type == 'f')
                {
                    matches = entry->type != DT_DIR;
                }
                else if (type == 'd')
                {
                    matches = entry->type == DT_DIR;
                }

                if (
                    matches &&
                    (
                        pattern == 0 ||
                        pattern_match(entry->name, pattern)
                    )
                )
                {
                    log_write(child);
                    log_write("\n");
                }

                if (entry->type == DT_DIR)
                {
                    find_directory(
                        child,
                        pattern,
                        type
                    );
                }
            }

            offset += entry->record_length;
        }
    }

    sys_close(fd);
}

int main(
    int argc,
    char **argv
)
{
    const char *path = ".";
    const char *pattern = 0;
    char type = 0;

    if (argc >= 2)
    {
        path = argv[1];
    }

    long i = 2;

    while (i < argc)
    {
        if (
            string_equals(argv[i], "-name") &&
            i + 1 < argc
        )
        {
            pattern = argv[i + 1];
            i += 2;
        }
        else if (
            string_equals(argv[i], "-type") &&
            i + 1 < argc
        )
        {
            type = argv[i + 1][0];

            if (
                type != 'f' &&
                type != 'd'
            )
            {
                log_error(
                    "find: invalid type\n"
                );

                return 1;
            }

            i += 2;
        }
        else
        {
            log_error(
                "usage: find [path] [-name pattern] [-type f|d]\n"
            );

            return 1;
        }
    }

    find_directory(
        path,
        pattern,
        type
    );

    return 0;
}