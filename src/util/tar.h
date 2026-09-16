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

#pragma once

enum tar_type
{
    TAR_TYPE_FILE,
    TAR_TYPE_DIRECTORY,
    TAR_TYPE_SYMLINK,
    TAR_TYPE_UNKNOWN
};

struct tar_header
{
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
};

int tar_open(const char *path);
int tar_next(int fd, struct tar_header *header);
long tar_parse_size(const char *size);
long tar_read(int fd, void *buffer, long size);
enum tar_type tar_get_type(const struct tar_header *header);
int tar_extract_file(int fd, const struct tar_header *header, const char *destination);
long tar_build_path(
    char *buffer,
    long buffer_size,
    const char *destination,
    const char *name
);
int tar_extract_directory(
    const struct tar_header *header,
    const char *destination
);
int tar_extract(
    int fd,
    const char *destination
);