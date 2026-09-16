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
#include "syscall.h"

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

static unsigned int deflate_reverse_bits(
    unsigned int value,
    int count
)
{
    unsigned int result = 0;

    for (int i = 0; i < count; i++)
    {
        result <<= 1;
        result |= value & 1;
        value >>= 1;
    }

    return result;
}

static void deflate_fixed_code(
    int symbol,
    unsigned int *code,
    int *length
)
{
    unsigned int value;

    if (symbol <= 143)
    {
        value =
            0x30 +
            symbol;

        *length = 8;
    }
    else if (symbol <= 255)
    {
        value =
            0x190 +
            (symbol - 144);

        *length = 9;
    }
    else if (symbol <= 279)
    {
        value =
            symbol - 256;

        *length = 7;
    }
    else
    {
        value =
            0xC0 +
            (symbol - 280);

        *length = 8;
    }

    *code = deflate_reverse_bits(
        value,
        *length
    );
}

static int deflate_length_code(
    int length,
    int *code,
    int *extra,
    int *extra_bits
)
{
    if (length < 3 || length > 258)
    {
        return 0;
    }

    for (int i = 0; i < 29; i++)
    {
        int base = length_base[i];

        int next;

        if (i == 28)
        {
            next = 259;
        }
        else
        {
            next = length_base[i + 1];
        }

        if (length >= base && length < next)
        {
            *code = 257 + i;
            *extra = length - base;
            *extra_bits = length_extra[i];

            return 1;
        }
    }

    return 0;
}

static int deflate_distance_code(
    int distance,
    int *code,
    int *extra,
    int *extra_bits
)
{
    if (distance < 1 || distance > 32768)
    {
        return 0;
    }

    for (int i = 0; i < 30; i++)
    {
        int base = distance_base[i];

        int next;

        if (i == 29)
        {
            next = 32769;
        }
        else
        {
            next = distance_base[i + 1];
        }

        if (distance >= base && distance < next)
        {
            *code = i;
            *extra = distance - base;
            *extra_bits = distance_extra[i];

            return 1;
        }
    }

    return 0;
}

static int deflate_write_match(
    struct bit_writer *writer,
    int length,
    int distance
)
{
    int length_code;
    int length_extra;
    int length_extra_bits;

    if (!deflate_length_code(
        length,
        &length_code,
        &length_extra,
        &length_extra_bits
    ))
    {
        return 0;
    }

    int distance_code;
    int distance_extra;
    int distance_extra_bits;

    if (!deflate_distance_code(
        distance,
        &distance_code,
        &distance_extra,
        &distance_extra_bits
    ))
    {
        return 0;
    }

    /*
     * Length symbol
     */
    unsigned int code;
    int code_length;

    deflate_fixed_code(
        length_code,
        &code,
        &code_length
    );

    if (!bit_write(
        writer,
        code,
        code_length
    ))
    {
        return 0;
    }

    /*
     * Length extra bits
     */
    if (length_extra_bits > 0)
    {
        if (!bit_write(
            writer,
            length_extra,
            length_extra_bits
        ))
        {
            return 0;
        }
    }

    /*
     * Distance symbol
     *
     * Fixed Huffman distance codes are
     * 5 bits long.
     */
    code = deflate_reverse_bits(
        distance_code,
        5
    );

    if (!bit_write(
        writer,
        code,
        5
    ))
    {
        return 0;
    }

    /*
     * Distance extra bits
     */
    if (distance_extra_bits > 0)
    {
        if (!bit_write(
            writer,
            distance_extra,
            distance_extra_bits
        ))
        {
            return 0;
        }
    }

    return 1;
}


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

int deflate_read_stored_block(
    struct bit_reader *reader,
    unsigned char *output,
    long output_capacity,
    long *output_size
)
{
    bit_align(reader);

    unsigned char header[4];

    long result = sys_read(
        reader->fd,
        header,
        4
    );

    if (result != 4)
    {
        return 0;
    }

    unsigned short length =
        (unsigned short)header[0] |
        ((unsigned short)header[1] << 8);

    unsigned short inverted_length =
        (unsigned short)header[2] |
        ((unsigned short)header[3] << 8);

    if ((unsigned short)~length != inverted_length)
    {
        return 0;
    }

    if (*output_size + length > output_capacity)
    {
        return 0;
    }

    if (length > 0)
    {
        result = sys_read(
            reader->fd,
            output + *output_size,
            length
        );

        if (result != length)
        {
            return 0;
        }
    }

    *output_size += length;

    return 1;
}

int deflate_write_fixed_block(
    struct bit_writer *writer,
    const unsigned char *data,
    long size,
    int final
)
{
    if (!bit_write(
        writer,
        final,
        1
    ))
    {
        return 0;
    }

    /*
     * BTYPE = 01
     */
    if (!bit_write(
        writer,
        1,
        2
    ))
    {
        return 0;
    }

    long position = 0;

    while (position < size)
    {
        struct deflate_match match =
            deflate_find_match(
                data,
                position,
                size
            );

        if (match.length > 0)
        {
            if (!deflate_write_match(
                writer,
                match.length,
                match.distance
            ))
            {
                return 0;
            }

            position += match.length;
        }
        else
        {
            unsigned int code;
            int length;

            deflate_fixed_code(
                data[position],
                &code,
                &length
            );

            if (!bit_write(
                writer,
                code,
                length
            ))
            {
                return 0;
            }

            position++;
        }
    }

    /*
     * End Of Block
     */
    unsigned int code;
    int length;

    deflate_fixed_code(
        256,
        &code,
        &length
    );

    if (!bit_write(
        writer,
        code,
        length
    ))
    {
        return 0;
    }

    return 1;
}

struct deflate_match deflate_find_match(
    const unsigned char *data,
    long position,
    long size
)
{
    struct deflate_match result;

    result.length = 0;
    result.distance = 0;

    for (long start = position - 1;
         start >= 0;
         start--)
    {
        int distance = position - start;

        int length = 0;

        while (
            length < 258 &&
            position + length < size &&
            data[start + length] ==
                data[position + length]
        )
        {
            length++;
        }

        if (length > result.length)
        {
            result.length = length;
            result.distance = distance;
        }
    }

    if (result.length < 3)
    {
        result.length = 0;
        result.distance = 0;
    }

    return result;
}