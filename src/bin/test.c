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

int main(
    int argc,
    char **argv
)
{
    (void)argc;
    (void)argv;

    unsigned long literal_frequencies[286];

    for (int i = 0; i < 286; i++)
    {
        literal_frequencies[i] = 0;
    }

    /*
     * Fibonacci-like frequencies.
     *
     * This should produce a very unbalanced
     * Huffman tree with long code lengths.
     */
    unsigned long a = 1;
    unsigned long b = 1;

    for (int i = 0; i < 25; i++)
    {
        literal_frequencies[i] = a;

        unsigned long next =
            a + b;

        a = b;
        b = next;
    }

    unsigned char literal_lengths[286];

    struct huffman_code literal_codes[286];

    if (!huffman_build_lengths(
        literal_frequencies,
        literal_lengths,
        286
    ))
    {
        return 1;
    }

    int max_length_before = 0;

    for (int i = 0; i < 286; i++)
    {
        if (literal_lengths[i] >
            max_length_before)
        {
            max_length_before =
                literal_lengths[i];
        }
    }

    log_info(
        "Max length before: "
    );

    log_number(
        max_length_before
    );

    log_info("\n");

    if (!huffman_limit_lengths(
        literal_lengths,
        literal_frequencies,
        286,
        15
    ))
    {
        return 1;
    }

    int max_length_after = 0;

    for (int i = 0; i < 286; i++)
    {
        if (literal_lengths[i] >
            max_length_after)
        {
            max_length_after =
                literal_lengths[i];
        }
    }

    log_info(
        "Max length after: "
    );

    log_number(
        max_length_after
    );

    log_info("\n");

    if (!huffman_build(
        literal_codes,
        literal_lengths,
        286
    ))
    {
        return 1;
    }

    int active_count = 0;

    for (int i = 0; i < 286; i++)
    {
        if (literal_lengths[i] > 0)
        {
            active_count++;
        }
    }

    log_info("Active symbols: ");
    log_number(active_count);
    log_info("\n");

    int kraft = 0;

    for (int i = 0; i < 286; i++)
    {
        if (literal_lengths[i] == 0)
        {
            continue;
        }

        kraft +=
            1 << (15 - literal_lengths[i]);
    }

    log_info("Kraft slots: ");
    log_number(kraft);
    log_info(" / 32768\n");

    int valid = 1;

    for (int i = 0; i < 286; i++)
    {
        if (literal_lengths[i] == 0)
        {
            continue;
        }

        for (int j = i + 1; j < 286; j++)
        {
            if (literal_lengths[j] == 0)
            {
                continue;
            }

            if (literal_lengths[i] ==
                    literal_lengths[j] &&
                literal_codes[i].code ==
                    literal_codes[j].code)
            {
                valid = 0;
            }
        }
    }

    if (valid)
    {
        log_info("Codes: valid\n");
    }
    else
    {
        log_info("Codes: INVALID\n");
    }

    //log_info(
    //    "\nLiteral/length codes:\n"
    //);

    //for (int i = 0; i < 286; i++)
    //{
    //    if (literal_lengths[i] == 0)
    //    {
    //        continue;
    //    }

    //    log_number(
    //        literal_codes[i].symbol
    //    );

    //    log_info(
    //        ": length="
    //    );

    //    log_number(
    //        literal_codes[i].length
    //    );

    //    log_info(
    //        " code="
    //    );

    //    log_number(
    //        literal_codes[i].code
    //    );

    //    log_info(
    //        "\n"
    //    );
    //}

    return 0;
}