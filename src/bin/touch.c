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
    for (int i = 1; i < argc; i++)
    {
        int fd = sys_openat(
            -100, 
            argv[i], 
            64, 
            0644
        );

        if (fd < 0)
        {
            return 1;
        }

        sys_close(fd);
    }

    return 0;
}