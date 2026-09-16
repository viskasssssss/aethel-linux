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

int tar_open(const char *path)
{
    return sys_openat(
        -100,
        path,
        0,
        0
    );
}

int tar_next(int fd, struct tar_header *header)
{
    long result = sys_read(
        fd,
        header,
        512
    );

    if (result != 512)
    {
        return 0;
    }

    if (header->name[0] == '\0')
    {
        return 0;
    }

    return 1;
}

long tar_parse_size(const char *size)
{
    long result = 0;

    for (int i = 0; i < 12; i++)
    {
        if (size[i] < '0' || size[i] > '7')
        {
            break;
        }

        result *= 8;
        result += size[i] - '0';
    }

    return result;
}

long tar_read(
    int fd,
    void *buffer,
    long size
)
{
    long result = 0;

    while (result < size)
    {
        long read = sys_read(
            fd,
            (char *)buffer + result,
            size - result
        );

        if (read <= 0)
        {
            return result;
        }

        result += read;
    }

    long padding = (512 - (size % 512)) % 512;

    if (padding > 0)
    {
        char buffer[512];

        sys_read(
            fd,
            buffer,
            padding
        );
    }

    return result;
}

enum tar_type tar_get_type(
    const struct tar_header *header
)
{
    switch (header->typeflag)
    {
        case '\0':
        case '0':
            return TAR_TYPE_FILE;

        case '5':
            return TAR_TYPE_DIRECTORY;

        case '2':
            return TAR_TYPE_SYMLINK;

        default:
            return TAR_TYPE_UNKNOWN;
    }
}

int tar_extract_file(
    int fd,
    const struct tar_header *header,
    const char *destination
)
{
    long size = tar_parse_size(header->size);

    int output = sys_openat(
        -100,
        destination,
        O_WRONLY | O_CREAT | O_TRUNC,
        0644
    );

    if (output < 0)
    {
        return 0;
    }

    char buffer[512];
    long remaining = size;

    while (remaining > 0)
    {
        long chunk = remaining;

        if (chunk > 512)
        {
            chunk = 512;
        }

        long read = tar_read(
            fd,
            buffer,
            chunk
        );

        if (read != chunk)
        {
            sys_close(output);
            return 0;
        }

        long written = sys_write(
            output,
            buffer,
            read
        );

        if (written != read)
        {
            sys_close(output);
            return 0;
        }

        remaining -= read;
    }

    sys_close(output);

    return 1;
}

long tar_build_path(
    char *buffer,
    long buffer_size,
    const char *destination,
    const char *name
)
{
    long position = 0;

    while (
        destination[position] != '\0' &&
        position < buffer_size - 1
    )
    {
        buffer[position] = destination[position];
        position++;
    }

    if (position >= buffer_size - 1)
    {
        buffer[0] = '\0';
        return 0;
    }

    if (position > 0 &&
        buffer[position - 1] != '/')
    {
        buffer[position++] = '/';
    }

    long name_position = 0;

    while (
        name[name_position] != '\0' &&
        position < buffer_size - 1
    )
    {
        buffer[position++] = name[name_position++];
    }

    if (position >= buffer_size)
    {
        buffer[0] = '\0';
        return 0;
    }

    buffer[position] = '\0';

    return position;
}

int tar_extract_directory(
    const struct tar_header *header,
    const char *destination
)
{
    long mode = 0755;

    long path_size = 512;
    char path[512];

    if (!tar_build_path(
        path,
        path_size,
        destination,
        header->name
    ))
    {
        return 0;
    }

    long result = sys_mkdir(
        path,
        mode
    );

    if (result < 0)
    {
        return 0;
    }

    return 1;
}

int tar_extract(
    int fd,
    const char *destination
)
{
    struct tar_header header;

    if (sys_mkdir(destination, 0755) < 0)
    {
        return 0;
    }

    while (tar_next(fd, &header))
    {
        enum tar_type type = tar_get_type(&header);

        char path[512];

        if (!tar_build_path(
            path,
            sizeof(path),
            destination,
            header.name
        ))
        {
            return 0;
        }

        if (type == TAR_TYPE_DIRECTORY)
        {
            if (tar_extract_directory(
                &header,
                destination
            ) == 0)
            {
                return 0;
            }
        }
        else if (type == TAR_TYPE_FILE)
        {
            if (tar_extract_file(
                fd,
                &header,
                path
            ) == 0)
            {
                return 0;
            }
        }
        else if (type == TAR_TYPE_UNKNOWN)
        {
            return 0;
        }
    }

    return 1;
}