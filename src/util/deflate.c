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

static void deflate_count_code_length(
    unsigned long *frequencies,
    int symbol
)
{
    frequencies[symbol]++;
}

static int deflate_add_code_length_symbol(
    struct deflate_code_length_symbol *symbols,
    int *symbol_count,
    int symbol,
    int extra,
    int extra_bits
)
{
    symbols[*symbol_count].symbol = symbol;
    symbols[*symbol_count].extra = extra;
    symbols[*symbol_count].extra_bits = extra_bits;

    (*symbol_count)++;

    return 0;
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

int deflate_build_frequencies(
    const unsigned char *data,
    long size,
    unsigned long *literal_frequencies,
    unsigned long *distance_frequencies
)
{
    for (int i = 0; i < 286; i++)
    {
        literal_frequencies[i] = 0;
    }

    for (int i = 0; i < 30; i++)
    {
        distance_frequencies[i] = 0;
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
            int length_code;
            int length_extra;
            int length_extra_bits;

            if (!deflate_length_code(
                match.length,
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
                match.distance,
                &distance_code,
                &distance_extra,
                &distance_extra_bits
            ))
            {
                return 0;
            }

            literal_frequencies[length_code]++;
            distance_frequencies[distance_code]++;

            position += match.length;
        }
        else
        {
            literal_frequencies[data[position]]++;

            position++;
        }
    }

    /*
     * End Of Block
     */
    literal_frequencies[256]++;

    return 1;
}

int deflate_encode_code_lengths(
    const unsigned char *literal_lengths,
    const unsigned char *distance_lengths,
    int literal_count,
    int distance_count,
    struct deflate_code_length_symbol *symbols,
    int *symbol_count
)
{
    unsigned char lengths[286 + 30];

    int count = 0;
    int i = 0;

    for (int j = 0; j < literal_count; j++)
        lengths[count++] = literal_lengths[j];

    for (int j = 0; j < distance_count; j++)
        lengths[count++] = distance_lengths[j];

    *symbol_count = 0;

    while (i < count)
    {
        int length = lengths[i];
        int run_length = 1;

        while (i + run_length < count &&
               lengths[i + run_length] == length)
        {
            run_length++;
        }

        int run = run_length;

        if (length == 0)
        {
            while (run >= 11)
            {
                int repeat = run > 138 ? 138 : run;

                deflate_add_code_length_symbol(
                    symbols,
                    symbol_count,
                    18,
                    repeat - 11,
                    7
                );

                run -= repeat;
            }

            if (run >= 3)
            {
                int repeat = run > 10 ? 10 : run;

                deflate_add_code_length_symbol(
                    symbols,
                    symbol_count,
                    17,
                    repeat - 3,
                    3
                );

                run -= repeat;
            }

            while (run > 0)
            {
                deflate_add_code_length_symbol(
                    symbols,
                    symbol_count,
                    0,
                    0,
                    0
                );

                run--;
            }
        }
        else
        {
            deflate_add_code_length_symbol(
                symbols,
                symbol_count,
                length,
                0,
                0
            );

            run--;

            while (run >= 3)
            {
                int repeat = run > 6 ? 6 : run;

                deflate_add_code_length_symbol(
                    symbols,
                    symbol_count,
                    16,
                    repeat - 3,
                    2
                );

                run -= repeat;
            }

            while (run > 0)
            {
                deflate_add_code_length_symbol(
                    symbols,
                    symbol_count,
                    length,
                    0,
                    0
                );

                run--;
            }
        }

        i += run_length;
    }

    return 1;
}

int deflate_build_code_length_frequencies(
    const struct deflate_code_length_symbol *symbols,
    int symbol_count,
    unsigned long *frequencies
)
{
    for (int i = 0; i < 19; i++)
        frequencies[i] = 0;

    for (int i = 0; i < symbol_count; i++)
        frequencies[symbols[i].symbol]++;

    return 1;
}

int deflate_get_code_length_count(
    const unsigned char *lengths
)
{
    static const int order[19] =
    {
        16, 17, 18,
        0, 8, 7, 9,
        6, 10, 5, 11,
        4, 12, 3, 13,
        2, 14, 1, 15
    };

    int count = 4;

    for (int i = 18; i >= 0; i--)
    {
        if (lengths[order[i]] != 0)
        {
            count = i + 1;
            break;
        }
    }

    return count;
}

int deflate_write_code_length_lengths(
    struct bit_writer *writer,
    const unsigned char *lengths,
    int count
)
{
    static const int order[19] =
    {
        16, 17, 18,
        0, 8, 7, 9,
        6, 10, 5, 11,
        4, 12, 3, 13,
        2, 14, 1, 15
    };

    for (int i = 0; i < count; i++)
    {
        int symbol = order[i];

        if (!bit_write(
            writer,
            lengths[symbol],
            3
        ))
        {
            return 0;
        }
    }

    return 1;
}

int deflate_write_code_length_symbols(
    struct bit_writer *writer,
    const struct deflate_code_length_symbol *symbols,
    int symbol_count,
    const struct huffman_code *codes
)
{
    for (int i = 0; i < symbol_count; i++)
    {
        int symbol = symbols[i].symbol;

        if (!bit_write(
            writer,
            codes[symbol].code,
            codes[symbol].length
        ))
        {
            return 0;
        }

        if (symbols[i].extra_bits > 0)
        {
            if (!bit_write(
                writer,
                symbols[i].extra,
                symbols[i].extra_bits
            ))
            {
                return 0;
            }
        }
    }

    return 1;
}

int deflate_get_literal_count(
    const unsigned char *lengths
)
{
    int count = 286;

    while (count > 257 &&
           lengths[count - 1] == 0)
    {
        count--;
    }

    return count;
}

int deflate_get_distance_count(
    const unsigned char *lengths
)
{
    int count = 30;

    while (count > 1 &&
           lengths[count - 1] == 0)
    {
        count--;
    }

    return count;
}

int deflate_write_dynamic_header(
    struct bit_writer *writer,
    const unsigned char *literal_lengths,
    const unsigned char *distance_lengths,
    const unsigned char *code_length_lengths,
    const struct huffman_code *code_length_codes,
    const struct deflate_code_length_symbol *symbols,
    int symbol_count
)
{
    int literal_count =
        deflate_get_literal_count(
            literal_lengths
        );

    int distance_count =
        deflate_get_distance_count(
            distance_lengths
        );

    int code_length_count =
        deflate_get_code_length_count(
            code_length_lengths
        );

    unsigned long value;
    int count;

    /*
     * HLIT
     */

    value = literal_count - 257;
    count = 5;

    if (!bit_write(
        writer,
        literal_count - 257,
        5
    ))
        return 0;

    /*
     * HDIST
     */

    if (!bit_write(
        writer,
        distance_count - 1,
        5
    ))
        return 0;

    /*
     * HCLEN
     */

    if (!bit_write(
        writer,
        code_length_count - 4,
        4
    ))
        return 0;

    /*
     * Code length code lengths
     */

    if (!deflate_write_code_length_lengths(
        writer,
        code_length_lengths,
        code_length_count
    ))
        return 0;

    /*
     * RLE encoded lengths
     */

    if (!deflate_write_code_length_symbols(
        writer,
        symbols,
        symbol_count,
        code_length_codes
    ))
        return 0;

    return 1;
}

int deflate_write_literal(
    struct bit_writer *writer,
    const struct huffman_code *codes,
    int symbol
)
{
    if (symbol < 0 || symbol > 255)
    {
        return 0;
    }

    if (codes[symbol].length <= 0)
    {
        return 0;
    }

    if (!bit_write(
        writer,
        codes[symbol].code,
        codes[symbol].length
    ))
    {
        return 0;
    }

    return 1;
}

int deflate_write_end(
    struct bit_writer *writer,
    const struct huffman_code *codes
)
{
    if (codes[256].length <= 0)
        return 0;

    if (!bit_write(
        writer,
        codes[256].code,
        codes[256].length
    ))
    {
        return 0;
    }

    return 1;
}

int deflate_write_dynamic_match(
    struct bit_writer *writer,
    const struct huffman_code *literal_codes,
    const struct huffman_code *distance_codes,
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

    if (literal_codes[length_code].length <= 0)
        return 0;

    if (!bit_write(
        writer,
        literal_codes[length_code].code,
        literal_codes[length_code].length
    ))
    {
        return 0;
    }

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

    if (distance_codes[distance_code].length <= 0)
        return 0;

    if (!bit_write(
        writer,
        distance_codes[distance_code].code,
        distance_codes[distance_code].length
    ))
    {
        return 0;
    }

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

int deflate_write_dynamic_block(
    struct bit_writer *writer,
    const unsigned char *data,
    long size,
    int final
)
{
    unsigned long literal_frequencies[286];
    unsigned long distance_frequencies[30];

    if (!deflate_build_frequencies(
        data,
        size,
        literal_frequencies,
        distance_frequencies
    ))
    {
        return 0;
    }

    unsigned char literal_lengths[286] = {0};
    unsigned char distance_lengths[30] = {0};

    if (!huffman_build_lengths(
        literal_frequencies,
        literal_lengths,
        286
    ))
    {
        return 0;
    }

    if (!huffman_limit_lengths(
        literal_lengths,
        literal_frequencies,
        286,
        15
    ))
    {
        return 0;
    }

    /*
    * A distance tree with one active symbol
    * needs a one-bit code.
    */
    int distance_active = 0;

    for (int i = 0; i < 30; i++)
    {
        if (distance_frequencies[i] != 0)
        {
            distance_active++;
        }
    }

    if (distance_active == 1)
    {
        for (int i = 0; i < 30; i++)
        {
            if (distance_frequencies[i] != 0)
            {
                distance_lengths[i] = 1;
                break;
            }
        }
    }
    else
    {
        if (!huffman_build_lengths(
            distance_frequencies,
            distance_lengths,
            30
        ))
        {
            return 0;
        }

        if (!huffman_limit_lengths(
            distance_lengths,
            distance_frequencies,
            30,
            15
        ))
        {
            return 0;
        }
    }

    int literal_count =
        deflate_get_literal_count(
            literal_lengths
        );

    int distance_count =
        deflate_get_distance_count(
            distance_lengths
        );

    struct deflate_code_length_symbol symbols[320];
    int symbol_count;

    if (!deflate_encode_code_lengths(
        literal_lengths,
        distance_lengths,
        literal_count,
        distance_count,
        symbols,
        &symbol_count
    ))
    {
        return 0;
    }

    unsigned long code_length_frequencies[19] = {0};

    if (!deflate_build_code_length_frequencies(
        symbols,
        symbol_count,
        code_length_frequencies
    ))
    {
        return 0;
    }

    unsigned char code_length_lengths[19] = {0};

    if (!huffman_build_lengths(
        code_length_frequencies,
        code_length_lengths,
        19
    ))
    {
        return 0;
    }

    if (!huffman_limit_lengths(
        code_length_lengths,
        code_length_frequencies,
        19,
        7
    ))
    {
        return 0;
    }

    struct huffman_code literal_codes[286];
    struct huffman_code distance_codes[30];
    struct huffman_code code_length_codes[19];

    if (!huffman_build(
        literal_codes,
        literal_lengths,
        286
    ))
    {
        return 0;
    }

    if (!huffman_build(
        distance_codes,
        distance_lengths,
        30
    ))
    {
        return 0;
    }

    if (!huffman_build(
        code_length_codes,
        code_length_lengths,
        19
    ))
    {
        return 0;
    }

    /*
     * BFINAL
     */
    if (!bit_write(
        writer,
        final ? 1 : 0,
        1
    ))
    {
        return 0;
    }

    /*
     * BTYPE = 2
     */
    if (!bit_write(
        writer,
        2,
        2
    ))
    {
        return 0;
    }

    if (!deflate_write_dynamic_header(
        writer,
        literal_lengths,
        distance_lengths,
        code_length_lengths,
        code_length_codes,
        symbols,
        symbol_count
    ))
    {
        return 0;
    }

    /*
     * Write the actual LZ77 stream.
     */
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
            if (!deflate_write_dynamic_match(
                writer,
                literal_codes,
                distance_codes,
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
            if (!deflate_write_literal(
                writer,
                literal_codes,
                data[position]
            ))
            {
                return 0;
            }

            position++;
        }
    }

    /*
     * End Of Block.
     */
    if (!deflate_write_end(
        writer,
        literal_codes
    ))
    {
        return 0;
    }

    return 1;
}