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

#include "deflate.h"

static const int length_base[] =
{
    3, 4, 5, 6, 7, 8, 9, 10,
    11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115,
    131, 163, 195, 227, 258
};

static const int length_extra[] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4,
    5, 5, 5, 5, 0
};

static const int distance_base[] =
{
    1, 2, 3, 4,
    5, 7, 9, 13,
    17, 25, 33, 49,
    65, 97, 129, 193,
    257, 385, 513, 769,
    1025, 1537, 2049, 3073,
    4097, 6145, 8193, 12289,
    16385, 24577
};

static const int distance_extra[] =
{
    0, 0, 0, 0,
    1, 1, 2, 2,
    3, 3, 4, 4,
    5, 5, 6, 6,
    7, 7, 8, 8,
    9, 9, 10, 10,
    11, 11, 12, 12,
    13, 13
};

int deflate_read_block_header(
    struct bit_reader *reader,
    int *final,
    enum deflate_block_type *type
)
{
    int bfinal = bit_read(reader);

    if (bfinal < 0)
    {
        return 0;
    }

    unsigned long btype = bit_read_bits(
        reader,
        2
    );

    *final = bfinal;

    switch (btype)
    {
        case 0:
            *type = DEFLATE_BLOCK_STORED;
            break;

        case 1:
            *type = DEFLATE_BLOCK_FIXED;
            break;

        case 2:
            *type = DEFLATE_BLOCK_DYNAMIC;
            break;

        default:
            *type = DEFLATE_BLOCK_INVALID;
            return 0;
    }

    return 1;
}

int deflate_decode_length(
    struct bit_reader *reader,
    int symbol
)
{
    if (symbol < 257 ||
        symbol > 285)
    {
        return -1;
    }

    int index = symbol - 257;

    int length = length_base[index];

    int extra = length_extra[index];

    if (extra > 0)
    {
        unsigned long bits = bit_read_bits(
            reader,
            extra
        );

        length += bits;
    }

    return length;
}

int deflate_decode_distance(
    struct bit_reader *reader,
    int symbol
)
{
    if (symbol < 0 ||
        symbol > 29)
    {
        return -1;
    }

    int distance = distance_base[symbol];

    int extra = distance_extra[symbol];

    if (extra > 0)
    {
        unsigned long bits = bit_read_bits(
            reader,
            extra
        );

        distance += bits;
    }

    return distance;
}

int deflate_read_dynamic_header(
    struct bit_reader *reader,
    struct deflate_dynamic_header *header
)
{
    unsigned long hlit = bit_read_bits(
        reader,
        5
    );

    unsigned long hdist = bit_read_bits(
        reader,
        5
    );

    unsigned long hclen = bit_read_bits(
        reader,
        4
    );

    header->literal_count =
        hlit + 257;

    header->distance_count =
        hdist + 1;

    header->code_length_count =
        hclen + 4;

    return 1;
}

int deflate_read_code_lengths(
    struct bit_reader *reader,
    int count,
    unsigned char *lengths
)
{
    static const int order[] =
    {
        16, 17, 18,
        0, 8, 7, 9,
        6, 10, 5, 11,
        4, 12, 3, 13,
        2, 14, 1, 15
    };

    for (int i = 0; i < count; i++)
    {
        unsigned long value = bit_read_bits(
            reader,
            3
        );

        lengths[order[i]] = value;
    }

    return 1;
}

int deflate_read_dynamic_lengths(
    struct bit_reader *reader,
    const struct huffman_code *codes,
    int count,
    unsigned char *lengths
)
{
    int position = 0;
    int previous = 0;

    while (position < count)
    {
        int symbol = huffman_decode(
            reader,
            codes,
            19
        );

        if (symbol < 0)
        {
            return 0;
        }

        if (symbol <= 15)
        {
            lengths[position++] = symbol;
            previous = symbol;
        }
        else if (symbol == 16)
        {
            unsigned long extra = bit_read_bits(
                reader,
                2
            );

            int repeat = 3 + extra;

            for (int i = 0; i < repeat; i++)
            {
                if (position >= count)
                {
                    return 0;
                }

                lengths[position++] = previous;
            }
        }
        else if (symbol == 17)
        {
            unsigned long extra = bit_read_bits(
                reader,
                3
            );

            int repeat = 3 + extra;

            for (int i = 0; i < repeat; i++)
            {
                if (position >= count)
                {
                    return 0;
                }

                lengths[position++] = 0;
            }

            previous = 0;
        }
        else if (symbol == 18)
        {
            unsigned long extra = bit_read_bits(
                reader,
                7
            );

            int repeat = 11 + extra;

            for (int i = 0; i < repeat; i++)
            {
                if (position >= count)
                {
                    return 0;
                }

                lengths[position++] = 0;
            }

            previous = 0;
        }
        else
        {
            return 0;
        }
    }

    return 1;
}