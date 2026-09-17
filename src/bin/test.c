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

#include "gzip.h"
#include "syscall.h"
#include "log.h"

static int test_output_flush(
    const unsigned char *data,
    long size,
    void *context
)
{
    int fd = *(int *)context;

    long written = sys_write(
        fd,
        data,
        size
    );

    return written == size;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        log_error("usage: test <file.gz>\n");
        return 1;
    }

    int fd = sys_openat(
        -100,
        argv[1],
        O_RDONLY,
        0
    );

    if (fd < 0)
    {
        log_error("failed to open file\n");
        return 1;
    }

    unsigned char buffer[8];

    int output_fd = sys_openat(
        -100,
        "test-output",
        O_WRONLY | O_CREAT | O_TRUNC,
        0644
    );

    if (output_fd < 0)
    {
        log_error("failed to create output\n");
        return 1;
    }

    struct deflate_output output;

    output.buffer = buffer;
    output.capacity = sizeof(buffer);
    output.size = 0;
    output.window_size = 0;
    output.window_position = 0;
    output.crc = 0xFFFFFFFF;
    output.flush = test_output_flush;
    output.context = &output_fd;

    long result = gzip_read_file(
        &fd,
        &output,
        1
    );

    sys_close(output_fd);

    if (result < 0)
    {
        log_error("decompression failed\n");
        sys_close(fd);
        return 1;
    }

    log_info("decompression success\n");

    log_write("output size: ");
    log_number(result);
    log_write("\n");

    sys_close(fd);

    return 0;
}