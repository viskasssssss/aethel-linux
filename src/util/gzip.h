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

#define GZIP_FLAG_FEXTRA   0x04
#define GZIP_FLAG_FNAME    0x08
#define GZIP_FLAG_FCOMMENT 0x10
#define GZIP_FLAG_FHCRC    0x02

struct gzip_header
{
    unsigned char id1;
    unsigned char id2;
    unsigned char compression;
    unsigned char flags;
    unsigned char mtime[4];
    unsigned char extra_flags;
    unsigned char os;
};

struct gzip_footer
{
    unsigned char crc32[4];
    unsigned char isize[4];
};

enum gzip_compression
{
    GZIP_COMPRESSION_NONE,
    GZIP_COMPRESSION_FIXED,
    GZIP_COMPRESSION_DYNAMIC
};

int gzip_open(const char *path);

int gzip_read_header(
    int fd,
    struct gzip_header *header
);

int gzip_read_footer(
    int fd,
    struct gzip_footer *footer
);

long gzip_read_file(
    int fd,
    unsigned char *output,
    long output_capacity,
    int debug
);

int gzip_create(
    const char *source,
    const char *destination,
    enum gzip_compression compression
);