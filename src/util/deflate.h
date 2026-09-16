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

#include "bit.h"
#include "huffman.h"

enum deflate_block_type
{
    DEFLATE_BLOCK_STORED,
    DEFLATE_BLOCK_FIXED,
    DEFLATE_BLOCK_DYNAMIC,
    DEFLATE_BLOCK_INVALID
};

struct deflate_dynamic_header
{
    int literal_count;
    int distance_count;
    int code_length_count;
};

struct deflate_match
{
    int length;
    int distance;
};

int deflate_read_block_header(
    struct bit_reader *reader,
    int *final,
    enum deflate_block_type *type
);

int deflate_decode_length(
    struct bit_reader *reader,
    int symbol
);

int deflate_decode_distance(
    struct bit_reader *reader,
    int symbol
);

int deflate_read_dynamic_header(
    struct bit_reader *reader,
    struct deflate_dynamic_header *header
);

int deflate_read_code_lengths(
    struct bit_reader *reader,
    int count,
    unsigned char *lengths
);

int deflate_read_dynamic_lengths(
    struct bit_reader *reader,
    const struct huffman_code *codes,
    int count,
    unsigned char *lengths
);

int deflate_read_stored_block(
    struct bit_reader *reader,
    unsigned char *output,
    long output_capacity,
    long *output_size
);

int deflate_write_fixed_block(
    struct bit_writer *writer,
    const unsigned char *data,
    long size,
    int final
);

struct deflate_match deflate_find_match(
    const unsigned char *data,
    long position,
    long size
);

int deflate_build_frequencies(
    const unsigned char *data,
    long size,
    unsigned long *literal_frequencies,
    unsigned long *distance_frequencies
);