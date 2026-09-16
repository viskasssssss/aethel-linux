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
#include "syscall.h"
#include "log.h"

int main(
    int argc,
    char **argv
)
{
    if (argc < 3)
    {
        log_error(
            "test: usage: test <source> <archive>\n"
        );

        return 1;
    }

    const char *source = argv[1];
    const char *archive = argv[2];

    if (!tar_create(source, archive))
    {
        log_error(
            "test: tar creation failed\n"
        );

        return 1;
    }

    log_write(
        "test: tar archive created successfully\n"
    );

    return 0;
}