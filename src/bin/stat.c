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

int main(
    int argc,
    char **argv
)
{
    if (argc != 2)
    {
        log_error("usage: stat <file>\n");
        return 1;
    }

    struct stat information;

    long result = sys_newfstatat(
        AT_FDCWD,
        argv[1],
        &information,
        0
    );

    if (result < 0)
    {
        log_error("stat: failed to get file information\n");
        return 1;
    }

    log_write("File: ");
    log_write(argv[1]);
    log_write("\n");

    log_write("Size: ");
    log_number(information.size);
    log_write("\n");

    log_write("Inode: ");
    log_number(information.inode);
    log_write("\n");

    log_write("Links: ");
    log_number(information.nlink);
    log_write("\n");

    log_write("UID: ");
    log_number(information.uid);
    log_write("\n");

    log_write("GID: ");
    log_number(information.gid);
    log_write("\n");

    log_write("Blocks: ");
    log_number(information.blocks);
    log_write("\n");

    return 0;
}