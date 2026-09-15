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

static int is_number(const char *string)
{
    if (!string[0])
    {
        return 0;
    }

    for (long i = 0; string[i]; i++)
    {
        if (string[i] < '0' || string[i] > '9')
        {
            return 0;
        }
    }

    return 1;
}

static long string_length(const char *string)
{
    long length = 0;

    while (string[length])
    {
        length++;
    }

    return length;
}

static void print_number(long number)
{
    char buffer[32];
    long length = 0;

    if (number == 0)
    {
        sys_write(1, "0", 1);
        return;
    }

    while (number > 0)
    {
        buffer[length++] = '0' + (number % 10);
        number /= 10;
    }

    for (long i = length - 1; i >= 0; i--)
    {
        sys_write(1, &buffer[i], 1);
    }
}

int main(void)
{
    int fd = sys_openat(
        -100,
        "/proc",
        0,
        0
    );

    if (fd < 0)
    {
        sys_write(
            2,
            "ps: cannot open /proc\n",
            22
        );

        return 1;
    }

    char buffer[4096];

    long bytes_read = sys_getdents64(
        fd,
        buffer,
        sizeof(buffer)
    );

    if (bytes_read < 0)
    {
        sys_close(fd);
        return 1;
    }

    sys_write(
        1,
        "PID\tPPID\tSTATE\tNAME\n",
        20
    );

    long offset = 0;

    while (offset < bytes_read)
    {
        struct linux_dirent64 *entry =
            (struct linux_dirent64 *)(buffer + offset);

        if (entry->type == DT_DIR &&
            is_number(entry->name))
        {
            /*
             * Skip kernel threads.
             */

            char cmdline_path[128];
            long cmdline_path_length = 0;

            const char cmdline_prefix[] = "/proc/";
            const char cmdline_suffix[] = "/cmdline";

            for (long i = 0; cmdline_prefix[i]; i++)
            {
                cmdline_path[cmdline_path_length++] =
                    cmdline_prefix[i];
            }

            for (long i = 0; entry->name[i]; i++)
            {
                cmdline_path[cmdline_path_length++] =
                    entry->name[i];
            }

            for (long i = 0; cmdline_suffix[i]; i++)
            {
                cmdline_path[cmdline_path_length++] =
                    cmdline_suffix[i];
            }

            cmdline_path[cmdline_path_length] = '\0';

            int cmdline_fd = sys_openat(
                -100,
                cmdline_path,
                0,
                0
            );

            if (cmdline_fd < 0)
            {
                offset += entry->record_length;
                continue;
            }

            char cmdline[1];

            long cmdline_length = sys_read(
                cmdline_fd,
                cmdline,
                sizeof(cmdline)
            );

            sys_close(cmdline_fd);

            if (cmdline_length <= 0)
            {
                offset += entry->record_length;
                continue;
            }

            /*
             * Read process name.
             */

            char path[128];
            long path_length = 0;

            const char prefix[] = "/proc/";
            const char suffix[] = "/comm";

            for (long i = 0; prefix[i]; i++)
            {
                path[path_length++] = prefix[i];
            }

            for (long i = 0; entry->name[i]; i++)
            {
                path[path_length++] = entry->name[i];
            }

            for (long i = 0; suffix[i]; i++)
            {
                path[path_length++] = suffix[i];
            }

            path[path_length] = '\0';

            int comm_fd = sys_openat(
                -100,
                path,
                0,
                0
            );

            if (comm_fd >= 0)
            {
                char name[256];

                long name_length = sys_read(
                    comm_fd,
                    name,
                    sizeof(name) - 1
                );

                sys_close(comm_fd);

                if (name_length > 0)
                {
                    if (name[name_length - 1] == '\n')
                    {
                        name_length--;
                    }

                    /*
                     * Read process state and parent PID.
                     */

                    path_length = 0;

                    const char status_suffix[] = "/status";

                    for (long i = 0; prefix[i]; i++)
                    {
                        path[path_length++] = prefix[i];
                    }

                    for (long i = 0; entry->name[i]; i++)
                    {
                        path[path_length++] = entry->name[i];
                    }

                    for (long i = 0; status_suffix[i]; i++)
                    {
                        path[path_length++] =
                            status_suffix[i];
                    }

                    path[path_length] = '\0';

                    int status_fd = sys_openat(
                        -100,
                        path,
                        0,
                        0
                    );

                    char state = '?';
                    long ppid = 0;

                    if (status_fd >= 0)
                    {
                        char status[4096];

                        long status_length = sys_read(
                            status_fd,
                            status,
                            sizeof(status) - 1
                        );

                        sys_close(status_fd);

                        if (status_length > 0)
                        {
                            for (
                                long i = 0;
                                i < status_length - 7;
                                i++
                            )
                            {
                                /*
                                 * State:
                                 */

                                if (status[i] == 'S' &&
                                    status[i + 1] == 't' &&
                                    status[i + 2] == 'a' &&
                                    status[i + 3] == 't' &&
                                    status[i + 4] == 'e' &&
                                    status[i + 5] == ':')
                                {
                                    state = status[i + 7];
                                }

                                /*
                                 * PPid:
                                 */

                                if (status[i] == 'P' &&
                                    status[i + 1] == 'P' &&
                                    status[i + 2] == 'i' &&
                                    status[i + 3] == 'd' &&
                                    status[i + 4] == ':')
                                {
                                    long position = i + 5;

                                    while (
                                        position < status_length &&
                                        (status[position] == ' ' ||
                                         status[position] == '\t')
                                    )
                                    {
                                        position++;
                                    }

                                    while (
                                        position < status_length &&
                                        status[position] >= '0' &&
                                        status[position] <= '9'
                                    )
                                    {
                                        ppid =
                                            ppid * 10 +
                                            (status[position] - '0');

                                        position++;
                                    }
                                }
                            }
                        }
                    }

                    /*
                     * Print process information.
                     */

                    sys_write(
                        1,
                        entry->name,
                        string_length(entry->name)
                    );

                    sys_write(1, "\t", 1);

                    print_number(ppid);

                    sys_write(1, "\t", 1);

                    sys_write(
                        1,
                        &state,
                        1
                    );

                    sys_write(1, "\t", 1);

                    sys_write(
                        1,
                        name,
                        name_length
                    );

                    sys_write(1, "\n", 1);
                }
            }
        }

        offset += entry->record_length;
    }

    sys_close(fd);

    return 0;
}