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

#include "huffman.h"

static unsigned int reverse_bits(
    unsigned int value,
    int length
)
{
    unsigned int result = 0;

    for (int i = 0; i < length; i++)
    {
        result <<= 1;
        result |= value & 1;
        value >>= 1;
    }

    return result;
}

int huffman_build_fixed(
    struct huffman_code *literal_codes,
    struct huffman_code *distance_codes
)
{
    int code = 0;

    /*
     * 7-bit codes: 256-279
     */

    code = 0;

    for (int symbol = 256; symbol <= 279; symbol++)
    {
        literal_codes[symbol].code =
            reverse_bits(code, 7);

        literal_codes[symbol].length = 7;
        literal_codes[symbol].symbol = symbol;

        code++;
    }

    /*
     * 8-bit codes: 0-143
     */

    code = 0x30;

    for (int symbol = 0; symbol <= 143; symbol++)
    {
        literal_codes[symbol].code =
            reverse_bits(code, 8);

        literal_codes[symbol].length = 8;
        literal_codes[symbol].symbol = symbol;

        code++;
    }

    /*
     * 8-bit codes: 280-287
     */

    code = 0xC0;

    for (int symbol = 280; symbol <= 287; symbol++)
    {
        literal_codes[symbol].code =
            reverse_bits(code, 8);

        literal_codes[symbol].length = 8;
        literal_codes[symbol].symbol = symbol;

        code++;
    }

    /*
     * 9-bit codes: 144-255
     */

    code = 0x190;

    for (int symbol = 144; symbol <= 255; symbol++)
    {
        literal_codes[symbol].code =
            reverse_bits(code, 9);

        literal_codes[symbol].length = 9;
        literal_codes[symbol].symbol = symbol;

        code++;
    }

    /*
     * Distance codes: 0-31
     */

    for (int symbol = 0; symbol < 32; symbol++)
    {
        distance_codes[symbol].code =
            reverse_bits(symbol, 5);

        distance_codes[symbol].length = 5;
        distance_codes[symbol].symbol = symbol;
    }

    return 1;
}

int huffman_decode(
    struct bit_reader *reader,
    const struct huffman_code *codes,
    int count
)
{
    unsigned int code = 0;

    for (int length = 1; length <= 15; length++)
    {
        int bit = bit_read(reader);

        if (bit < 0)
        {
            return -1;
        }

        code |= (unsigned int)bit << (length - 1);

        for (int i = 0; i < count; i++)
        {
            if (codes[i].length == length &&
                codes[i].code == code)
            {
                return codes[i].symbol;
            }
        }
    }

    return -1;
}

int huffman_build(
    struct huffman_code *codes,
    const unsigned char *lengths,
    int count
)
{
    int bl_count[16] = { 0 };
    int next_code[16] = { 0 };

    for (int i = 0; i < count; i++)
    {
        if (lengths[i] > 0)
        {
            bl_count[lengths[i]]++;
        }
    }

    int code = 0;

    for (int bits = 1; bits <= 15; bits++)
    {
        code =
            (code + bl_count[bits - 1]) << 1;

        next_code[bits] = code;
    }

    for (int i = 0; i < count; i++)
    {
        codes[i].symbol = i;
        codes[i].length = lengths[i];

        if (lengths[i] == 0)
        {
            codes[i].code = 0;
            continue;
        }

        codes[i].code =
            reverse_bits(
                next_code[lengths[i]],
                lengths[i]
            );

        next_code[lengths[i]]++;
    }

    return 1;
}