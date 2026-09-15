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
    char buffer[256];

    long count = sys_readlink(
        "/proc/self/cwd", 
        buffer, 
        sizeof(buffer) - 1
    );

    if (count < 0)
    {
        log_error("pwd: cannot get current directory\n");

        return 1;
    }

    log_write(buffer);
    log_write("\n");

    return 0;
}