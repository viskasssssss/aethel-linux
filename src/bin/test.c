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

#include "tar.h"
#include "log.h"

int main(void)
{
    int fd = tar_open("/test.tar");

    if (fd < 0)
    {
        log_error("failed to open archive\n");
        return 1;
    }

    if (!tar_extract(
        fd,
        "/extracted"
    ))
    {
        log_error("failed to extract archive\n");
        sys_close(fd);
        return 1;
    }

    sys_close(fd);

    log_success("archive extracted\n");

    return 0;
}