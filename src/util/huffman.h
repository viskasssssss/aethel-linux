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

struct huffman_code
{
    unsigned int code;
    int length;
    int symbol;
};

int huffman_build_fixed(
    struct huffman_code *literal_codes,
    struct huffman_code *distance_codes
);

int huffman_decode(
    struct bit_reader *reader,
    const struct huffman_code *codes,
    int count
);

int huffman_build(
    struct huffman_code *codes,
    const unsigned char *lengths,
    int count
);

int huffman_build_lengths(
    const unsigned long *frequencies,
    unsigned char *lengths,
    int count
);

int huffman_limit_lengths(
    unsigned char *lengths,
    const unsigned long *frequencies,
    int count,
    int max_length
);