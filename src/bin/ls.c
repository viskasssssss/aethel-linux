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

int main(int argc, char **argv) {
    const char *path = ".";

    if (argc > 1)
        path = argv[1];

    int fd = sys_openat(
        -100, 
        path, 
        0, 
        0
    );

    if (fd < 0)
    {
        log_error("ls: cannot open directory\n");

        return 1;
    }

    char buffer[4096];

    long count = sys_getdents64(
        fd, 
        buffer, 
        sizeof(buffer)
    );

    if (count < 0)
    {
        log_error("ls: cannot read directory\n");

        sys_close(fd);

        return 1;
    }

    long position = 0;

    while (position < count)
    {
        struct linux_dirent64 *entry =
            (struct linux_dirent64 *)
            (buffer + position);
        
        if (entry->name[0] == '.') {
            position += entry->record_length;
            continue;
        }

        if (entry->type == DT_DIR)
        {
            log_info(entry->name);
            log_write("/");
        }
        else
        {
            log_write(entry->name);
        }

        log_write("\n");

        position += entry->record_length;
    }

    sys_close(fd);

    return 0;
}