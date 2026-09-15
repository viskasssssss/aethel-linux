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
    if (argc < 3)
    {
        log_error("cp: missing operand\n");

        return 1;
    }

    int source = sys_openat(
        -100, 
        argv[1], 
        0, 
        0
    );

    if (source < 0)
    {
        log_error("cp: cannot open source\n");

        return 1;
    }

    int destination = sys_openat(
        -100, 
        argv[2], 
        577, 
        0644
    );

    if (destination < 0)
    {
        log_error("cp: cannot create destination\n");

        sys_close(source);

        return 1;
    }

    char buffer[4096];

    while (1)
    {
        long count = sys_read(
            source, 
            buffer, 
            sizeof(buffer)
        );

        if (count <= 0)
            break;

        long written = sys_write(
            destination, 
            buffer, 
            count
        );

        if (written < 0)
            break;
    }

    sys_close(source);
    sys_close(destination);

    return 0;
}