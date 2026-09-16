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
#include "log.h"

struct huffman_node
{
    unsigned long frequency;

    int symbol;

    int left;
    int right;
};

struct huffman_frequency
{
    unsigned long frequency;
    int symbol;
};

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

static void huffman_count_lengths(
    const unsigned char *lengths,
    int count,
    int *counts,
    int maximum
)
{
    for (int i = 0; i <= maximum; i++)
    {
        counts[i] = 0;
    }

    for (int i = 0; i < count; i++)
    {
        if (lengths[i] > 0)
        {
            counts[lengths[i]]++;
        }
    }
}

static int huffman_fix_lengths(
    int *counts,
    int maximum
)
{
    unsigned long limit =
        1UL << maximum;

    unsigned long used = 0;

    /*
     * Calculate how much of the available
     * Huffman tree space is used when all
     * codes longer than maximum are clamped
     * to maximum.
     */
    for (int length = 1;
         length <= 255;
         length++)
    {
        if (counts[length] == 0)
        {
            continue;
        }

        int effective_length = length;

        if (effective_length > maximum)
        {
            effective_length = maximum;
        }

        used +=
            (unsigned long)counts[length] <<
            (maximum - effective_length);
    }

    if (used < limit)
    {
        return 0;
    }

    /*
     * Move all codes longer than maximum
     * to maximum.
     */
    for (int length = maximum + 1;
         length < 256;
         length++)
    {
        counts[maximum] += counts[length];
        counts[length] = 0;
    }

    /*
     * Every correction below decreases the
     * used tree space by exactly one
     * maximum-length code.
     */
    unsigned long overflow =
        used - limit;

    unsigned long correction =
        overflow;

    while (correction > 0)
    {
        int bits = maximum - 1;

        while (bits > 0 &&
               counts[bits] == 0)
        {
            bits--;
        }

        if (bits == 0 ||
            counts[maximum] == 0)
        {
            return 0;
        }

        counts[bits]--;

        counts[bits + 1] += 2;

        counts[maximum]--;

        correction--;
    }

    return 1;
}

static void huffman_sort_frequencies(
    struct huffman_frequency *items,
    int count
)
{
    for (int i = 0; i < count - 1; i++)
    {
        for (int j = i + 1; j < count; j++)
        {
            if (items[j].frequency <
                items[i].frequency)
            {
                struct huffman_frequency temp =
                    items[i];

                items[i] = items[j];
                items[j] = temp;
            }
        }
    }
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

int huffman_build_lengths(
    const unsigned long *frequencies,
    unsigned char *lengths,
    int count
)
{
    struct huffman_node nodes[512];

    int node_count = 0;

    for (int i = 0; i < count; i++)
    {
        lengths[i] = 0;

        if (frequencies[i] == 0)
        {
            continue;
        }

        nodes[node_count].frequency =
            frequencies[i];

        nodes[node_count].symbol =
            i;

        nodes[node_count].left =
            -1;

        nodes[node_count].right =
            -1;

        node_count++;
    }

    if (node_count == 0)
    {
        return 0;
    }

    if (node_count == 1)
    {
        lengths[nodes[0].symbol] = 1;

        return 1;
    }

    int active[512];

    int active_count = node_count;

    for (int i = 0; i < node_count; i++)
    {
        active[i] = i;
    }

    while (active_count > 1)
    {
        int first = -1;
        int second = -1;

        for (int i = 0; i < active_count; i++)
        {
            int index = active[i];

            if (first < 0 ||
                nodes[index].frequency <
                nodes[first].frequency)
            {
                second = first;
                first = index;
            }
            else if (second < 0 ||
                     nodes[index].frequency <
                     nodes[second].frequency)
            {
                second = index;
            }
        }

        int parent = node_count++;

        nodes[parent].frequency =
            nodes[first].frequency +
            nodes[second].frequency;

        nodes[parent].symbol = -1;
        nodes[parent].left = first;
        nodes[parent].right = second;

        int new_active_count = 0;

        for (int i = 0; i < active_count; i++)
        {
            if (active[i] == first ||
                active[i] == second)
            {
                continue;
            }

            active[new_active_count++] =
                active[i];
        }

        active[new_active_count++] =
            parent;

        active_count = new_active_count;
    }

    int root = active[0];

    int stack_nodes[512];
    int stack_depths[512];

    int stack_count = 0;

    stack_nodes[stack_count] = root;
    stack_depths[stack_count] = 0;
    stack_count++;

    while (stack_count > 0)
    {
        stack_count--;

        int node =
            stack_nodes[stack_count];

        int depth =
            stack_depths[stack_count];

        if (nodes[node].symbol >= 0)
        {
            lengths[nodes[node].symbol] =
                depth;

            continue;
        }

        if (nodes[node].left >= 0)
        {
            stack_nodes[stack_count] =
                nodes[node].left;

            stack_depths[stack_count] =
                depth + 1;

            stack_count++;
        }

        if (nodes[node].right >= 0)
        {
            stack_nodes[stack_count] =
                nodes[node].right;

            stack_depths[stack_count] =
                depth + 1;

            stack_count++;
        }
    }

    return 1;
}

int huffman_limit_lengths(
    unsigned char *lengths,
    const unsigned long *frequencies,
    int count,
    int max_length
)
{
    int length_counts[256];

    huffman_count_lengths(
        lengths,
        count,
        length_counts,
        255
    );

    if (!huffman_fix_lengths(
        length_counts,
        max_length
    ))
    {
        return 0;
    }

    struct huffman_frequency
        items[512];

    int item_count = 0;

    for (int i = 0; i < count; i++)
    {
        if (frequencies[i] == 0)
        {
            lengths[i] = 0;
            continue;
        }

        items[item_count].frequency =
            frequencies[i];

        items[item_count].symbol =
            i;

        item_count++;
    }

    huffman_sort_frequencies(
        items,
        item_count
    );

    int position = 0;

    /*
     * The least frequent symbols receive
     * the longest codes.
     */
    for (int length = max_length;
         length >= 1;
         length--)
    {
        for (int i = 0;
             i < length_counts[length];
             i++)
        {
            if (position >= item_count)
            {
                return 0;
            }

            lengths[items[position].symbol] =
                length;

            position++;
        }
    }

    return position == item_count;
}