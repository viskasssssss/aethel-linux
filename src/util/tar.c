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

static void tar_write_octal(
    char *buffer,
    int size,
    long value
)
{
    for (int i = size - 2; i >= 0; i--)
    {
        buffer[i] = '0' + (value & 7);
        value >>= 3;
    }

    buffer[size - 1] = '\0';
}

static unsigned long tar_calculate_checksum(
    struct tar_header *header
)
{
    unsigned long checksum = 0;

    for (int i = 0; i < 512; i++)
    {
        if (i >= 148 && i < 156)
        {
            checksum += ' ';
        }
        else
        {
            checksum += ((unsigned char *)header)[i];
        }
    }

    return checksum;
}

static int tar_write_header(
    int fd,
    struct tar_header *header
)
{
    for (int i = 0; i < 8; i++)
    {
        header->checksum[i] = ' ';
    }

    unsigned long checksum =
        tar_calculate_checksum(header);

    tar_write_octal(
        header->checksum,
        8,
        checksum
    );

    long written = sys_write(
        fd,
        header,
        512
    );

    if (written != 512)
    {
        return 0;
    }

    return 1;
}

static int tar_write_file(
    int archive,
    const char *path,
    const char *name
)
{
    struct stat information;

    long result = sys_newfstatat(
        -100,
        path,
        &information,
        0
    );

    if (result < 0)
    {
        return 0;
    }

    int input = sys_openat(
        -100,
        path,
        O_RDONLY,
        0
    );

    if (input < 0)
    {
        return 0;
    }

    struct tar_header header = { 0 };

    long i = 0;

    while (
        name[i] != '\0' &&
        i < sizeof(header.name) - 1
    )
    {
        header.name[i] = name[i];
        i++;
    }

    header.typeflag = '0';

    header.magic[0] = 'u';
    header.magic[1] = 's';
    header.magic[2] = 't';
    header.magic[3] = 'a';
    header.magic[4] = 'r';
    header.magic[5] = '\0';

    header.version[0] = '0';
    header.version[1] = '0';

    tar_write_octal(
        header.mode,
        sizeof(header.mode),
        information.mode & 07777
    );

    tar_write_octal(
        header.uid,
        sizeof(header.uid),
        information.uid
    );

    tar_write_octal(
        header.gid,
        sizeof(header.gid),
        information.gid
    );

    tar_write_octal(
        header.size,
        sizeof(header.size),
        information.size
    );

    tar_write_octal(
        header.mtime,
        sizeof(header.mtime),
        information.mtime
    );

    for (i = 0; i < sizeof(header.checksum); i++)
    {
        header.checksum[i] = ' ';
    }

    unsigned int checksum = 0;

    const unsigned char *bytes =
        (const unsigned char *)&header;

    for (i = 0; i < 512; i++)
    {
        checksum += bytes[i];
    }

    tar_write_octal(
        header.checksum,
        sizeof(header.checksum),
        checksum
    );

    header.checksum[6] = '\0';
    header.checksum[7] = ' ';

    result = sys_write(
        archive,
        &header,
        512
    );

    if (result != 512)
    {
        sys_close(input);
        return 0;
    }

    char buffer[4096];

    long remaining = information.size;

    while (remaining > 0)
    {
        long chunk = remaining;

        if (chunk > sizeof(buffer))
        {
            chunk = sizeof(buffer);
        }

        result = sys_read(
            input,
            buffer,
            chunk
        );

        if (result != chunk)
        {
            sys_close(input);
            return 0;
        }

        result = sys_write(
            archive,
            buffer,
            chunk
        );

        if (result != chunk)
        {
            sys_close(input);
            return 0;
        }

        remaining -= chunk;
    }

    long padding =
        (512 - (information.size % 512)) % 512;

    if (padding > 0)
    {
        char padding_buffer[512] = { 0 };

        result = sys_write(
            archive,
            padding_buffer,
            padding
        );

        if (result != padding)
        {
            sys_close(input);
            return 0;
        }
    }

    sys_close(input);

    return 1;
}

static int tar_write_directory(
    int archive,
    const char *name
)
{
    struct tar_header header = { 0 };

    long i = 0;

    while (
        name[i] != '\0' &&
        i < sizeof(header.name) - 2
    )
    {
        header.name[i] = name[i];
        i++;
    }

    if (i == 0 || header.name[i - 1] != '/')
    {
        header.name[i++] = '/';
    }

    header.typeflag = '5';

    header.magic[0] = 'u';
    header.magic[1] = 's';
    header.magic[2] = 't';
    header.magic[3] = 'a';
    header.magic[4] = 'r';
    header.magic[5] = '\0';

    header.version[0] = '0';
    header.version[1] = '0';

    tar_write_octal(
        header.mode,
        sizeof(header.mode),
        0755
    );

    tar_write_octal(
        header.uid,
        sizeof(header.uid),
        0
    );

    tar_write_octal(
        header.gid,
        sizeof(header.gid),
        0
    );

    tar_write_octal(
        header.size,
        sizeof(header.size),
        0
    );

    tar_write_octal(
        header.mtime,
        sizeof(header.mtime),
        0
    );

    for (i = 0; i < sizeof(header.checksum); i++)
    {
        header.checksum[i] = ' ';
    }

    unsigned int checksum = 0;

    const unsigned char *bytes =
        (const unsigned char *)&header;

    for (i = 0; i < 512; i++)
    {
        checksum += bytes[i];
    }

    tar_write_octal(
        header.checksum,
        sizeof(header.checksum),
        checksum
    );

    header.checksum[6] = '\0';
    header.checksum[7] = ' ';

    return sys_write(
        archive,
        &header,
        512
    ) == 512;
}

static int tar_is_directory(
    const struct stat *information
)
{
    return (
        (information->mode & S_IFMT) ==
        S_IFDIR
    );
}

static int tar_write_entry(
    int archive,
    const char *path,
    const char *name
)
{
    struct stat information;

    long result = sys_newfstatat(
        -100,
        path,
        &information,
        0
    );

    if (result < 0)
    {
        return 0;
    }

    if (tar_is_directory(&information))
    {
        if (!tar_write_directory(
            archive,
            name
        ))
        {
            return 0;
        }

        int directory = sys_openat(
            -100,
            path,
            O_RDONLY,
            0
        );

        if (directory < 0)
        {
            return 0;
        }

        char buffer[4096];

        while (1)
        {
            long size = sys_getdents64(
                directory,
                buffer,
                sizeof(buffer)
            );

            if (size < 0)
            {
                sys_close(directory);
                return 0;
            }

            if (size == 0)
            {
                break;
            }

            long position = 0;

            while (position < size)
            {
                struct linux_dirent64 *entry =
                    (struct linux_dirent64 *)
                    (buffer + position);

                if (entry->name[0] != '.' ||
                    (entry->name[1] != '\0' &&
                     !(entry->name[1] == '.' &&
                       entry->name[2] == '\0')))
                {
                    char child_path[512];
                    char child_name[512];

                    long path_position = 0;

                    while (
                        path[path_position] != '\0' &&
                        path_position < sizeof(child_path) - 1
                    )
                    {
                        child_path[path_position] =
                            path[path_position];

                        path_position++;
                    }

                    if (path_position > 0 &&
                        child_path[path_position - 1] != '/')
                    {
                        child_path[path_position++] = '/';
                    }

                    long entry_position = 0;

                    while (
                        entry->name[entry_position] != '\0' &&
                        path_position < sizeof(child_path) - 1
                    )
                    {
                        child_path[path_position++] =
                            entry->name[entry_position++];

                    }

                    if (path_position >= sizeof(child_path))
                    {
                        sys_close(directory);
                        return 0;
                    }

                    child_path[path_position] = '\0';

                    long name_position = 0;

                    while (
                        name[name_position] != '\0' &&
                        name_position < sizeof(child_name) - 1
                    )
                    {
                        child_name[name_position] =
                            name[name_position];

                        name_position++;
                    }

                    if (name_position > 0 &&
                        child_name[name_position - 1] != '/')
                    {
                        child_name[name_position++] = '/';
                    }

                    entry_position = 0;

                    while (
                        entry->name[entry_position] != '\0' &&
                        name_position < sizeof(child_name) - 1
                    )
                    {
                        child_name[name_position++] =
                            entry->name[entry_position++];

                    }

                    if (name_position >= sizeof(child_name))
                    {
                        sys_close(directory);
                        return 0;
                    }

                    child_name[name_position] = '\0';

                    struct stat child_information;

                    result = sys_newfstatat(
                        -100,
                        child_path,
                        &child_information,
                        0
                    );

                    if (result < 0)
                    {
                        sys_close(directory);
                        return 0;
                    }

                    if (tar_is_directory(
                        &child_information
                    ))
                    {
                        if (!tar_write_entry(
                            archive,
                            child_path,
                            child_name
                        ))
                        {
                            sys_close(directory);
                            return 0;
                        }
                    }
                    else
                    {
                        child_name[name_position] = '\0';

                        if (!tar_write_entry(
                            archive,
                            child_path,
                            child_name
                        ))
                        {
                            sys_close(directory);
                            return 0;
                        }
                    }
                }

                if (entry->record_length == 0)
                {
                    sys_close(directory);
                    return 0;
                }

                position += entry->record_length;
            }
        }

        sys_close(directory);

        return 1;
    }

    return tar_write_file(
        archive,
        path,
        name
    );
}


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

    if (result < 0 &&
        result != -EEXIST)
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

int tar_create(
    const char *source,
    const char *destination
)
{
    struct stat information;

    long result = sys_newfstatat(
        -100,
        source,
        &information,
        0
    );

    if (result < 0)
    {
        return 0;
    }

    int archive = sys_openat(
        -100,
        destination,
        O_WRONLY | O_CREAT | O_TRUNC,
        0644
    );

    if (archive < 0)
    {
        return 0;
    }

    char name[512];

    if (tar_is_directory(&information))
    {
        name[0] = '.';
        name[1] = '/';
        name[2] = '\0';
    }
    else
    {
        long position = 0;
        long start = 0;

        while (
            source[position] != '\0' &&
            position < sizeof(name) - 1
        )
        {
            if (source[position] == '/')
            {
                start = position + 1;
            }

            position++;
        }

        position = 0;

        while (
            source[start] != '\0' &&
            position < sizeof(name) - 3
        )
        {
            name[position + 2] = source[start];
            position++;
            start++;
        }

        name[0] = '.';
        name[1] = '/';
        name[position + 2] = '\0';
    }

    int success = tar_write_entry(
        archive,
        source,
        name
    );

    if (success)
    {
        char end[1024] = { 0 };

        result = sys_write(
            archive,
            end,
            sizeof(end)
        );

        if (result != sizeof(end))
        {
            success = 0;
        }
    }

    sys_close(archive);

    return success;
}