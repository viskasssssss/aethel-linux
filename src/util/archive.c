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

#include "archive.h"
#include "gzip.h"
#include "tar.h"
#include "syscall.h"
#include "log.h"

int archive_extract(
    const char *path,
    const char *destination
)
{
    int gzip = gzip_open(path);

    if (gzip < 0)
    {
        log_error("archive: failed to open gzip\n");
        return 0;
    }

    unsigned char buffer[65536];

    long size = gzip_read_file(
        gzip,
        buffer,
        sizeof(buffer),
        0
    );

    if (size < 0)
    {
        log_error("archive: failed to decompress gzip\n");
        sys_close(gzip);
        return 0;
    }

    sys_close(gzip);

    const char *temporary_path = "/tmp/archive.tar";

    int tar_file = sys_openat(
        -100,
        temporary_path,
        O_WRONLY | O_CREAT | O_TRUNC,
        0644
    );

    if (tar_file < 0)
    {
        log_error("archive: failed to create temporary tar\n");
        return 0;
    }

    long written = sys_write(
        tar_file,
        buffer,
        size
    );

    sys_close(tar_file);

    if (written != size)
    {
        log_error("archive: failed to write temporary tar\n");
        sys_unlink(temporary_path);
        return 0;
    }

    int tar = tar_open(temporary_path);

    if (tar < 0)
    {
        log_error("archive: failed to open temporary tar\n");
        sys_unlink(temporary_path);
        return 0;
    }

    int result = tar_extract(
        tar,
        destination
    );

    sys_close(tar);

    sys_unlink(temporary_path);

    if (!result)
    {
        log_error("archive: failed to extract tar\n");
        return 0;
    }

    return 1;
}