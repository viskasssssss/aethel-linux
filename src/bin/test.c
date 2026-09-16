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

    unsigned char test_literal_lengths[286] = {0};
    unsigned char test_distance_lengths[30] = {0};

    test_literal_lengths[0] = 5;
    test_literal_lengths[1] = 5;
    test_literal_lengths[2] = 5;
    test_literal_lengths[3] = 5;
    test_literal_lengths[4] = 7;

    struct deflate_code_length_symbol
        symbols[316];

    int symbol_count;

    int literal_count =
        deflate_get_literal_count(test_literal_lengths);

    int distance_count =
        deflate_get_distance_count(test_distance_lengths);

    if (!deflate_encode_code_lengths(
        test_literal_lengths,
        test_distance_lengths,
        literal_count,
        distance_count,
        symbols,
        &symbol_count
    ))
    {
        return 1;
    }

    log_info(
        "\nCode length RLE:\n"
    );

    for (int i = 0; i < symbol_count; i++)
    {
        log_info("symbol=");

        log_number(symbols[i].symbol);

        log_info(" extra=");

        log_number(symbols[i].extra);

        log_info(" extra_bits=");

        log_number(symbols[i].extra_bits);

        log_info("\n");
    }

    unsigned long code_length_frequencies[19];

    if (!deflate_build_code_length_frequencies(
        symbols,
        symbol_count,
        code_length_frequencies
    ))
    {
        return 1;
    }

    log_info("\nCode length frequencies:\n");

    for (int i = 0; i < 19; i++)
    {
        log_info("symbol=");
        log_number(i);
        log_info(" frequency=");
        log_number(code_length_frequencies[i]);
        log_info("\n");
    }

    unsigned char code_length_lengths[19];
    int code_length_count = 19;

    if (!huffman_build_lengths(
        code_length_frequencies,
        code_length_lengths,
        code_length_count
    ))
    {
        return 1;
    }

    int max_code_length = 0;

    for (int i = 0; i < code_length_count; i++)
    {
        if (code_length_lengths[i] > max_code_length)
            max_code_length = code_length_lengths[i];
    }

    log_info("\nCode length Huffman:\n");

    log_info("Max length before: ");
    log_number(max_code_length);
    log_info("\n");

    int max_length = 7;

    if (!huffman_limit_lengths(
        code_length_lengths,
        code_length_frequencies,
        code_length_count,
        max_length
    ))
    {
        return 1;
    }

    max_code_length = 0;

    for (int i = 0; i < code_length_count; i++)
    {
        if (code_length_lengths[i] > max_code_length)
            max_code_length = code_length_lengths[i];
    }

    log_info("Max length after: ");
    log_number(max_code_length);
    log_info("\n");

    struct huffman_code code_length_codes[19];

    if (!huffman_build(
        code_length_codes,
        code_length_lengths,
        code_length_count
    ))
    {
        return 1;
    }

    for (int i = 0; i < 19; i++)
    {
        if (code_length_lengths[i] == 0)
            continue;

        log_info("symbol=");
        log_number(i);
        log_info(" length=");
        log_number(code_length_lengths[i]);
        log_info(" code=");
        log_number(code_length_codes[i].code);
        log_info("\n");
    }

    code_length_count =
    deflate_get_code_length_count(code_length_lengths);

    log_info("Code length count: ");
    log_number(code_length_count);
    log_info("\n");

    int hclen = code_length_count - 4;

    log_info("HCLEN: ");
    log_number(hclen);
    log_info("\n");

    log_info("\nDynamic counts:\n");

    log_info("Literal count: ");
    log_number(literal_count);
    log_info("\n");

    log_info("Distance count: ");
    log_number(distance_count);
    log_info("\n");

    log_info("HLIT: ");
    log_number(literal_count - 257);
    log_info("\n");

    log_info("HDIST: ");
    log_number(distance_count - 1);
    log_info("\n");

    int fd = sys_openat(
        -100,
        "/tmp/dynamic-header",
        O_WRONLY | O_CREAT | O_TRUNC,
        0644
    );

    if (fd < 0)
        return 1;

    struct bit_writer writer;

    bit_writer_init(
        &writer,
        fd
    );

    if (!deflate_write_dynamic_header(
        &writer,
        test_literal_lengths,
        test_distance_lengths,
        code_length_lengths,
        code_length_codes,
        symbols,
        symbol_count
    ))
    {
        sys_close(fd);
        return 1;
    }

    if (!bit_writer_flush(&writer))
    {
        sys_close(fd);
        return 1;
    }

    sys_close(fd);

    fd = sys_openat(
        -100,
        "/tmp/dynamic-header",
        O_RDONLY,
        0
    );

    if (fd < 0)
        return 1;

    struct bit_reader reader;

    bit_reader_init(
        &reader,
        fd
    );

    int count = 5;

    unsigned long hlit =
        bit_read_bits(
            &reader,
            count
        );

    count = 5;

    unsigned long hdist =
        bit_read_bits(
            &reader,
            count
        );

    count = 4;

    unsigned long read_hclen =
        bit_read_bits(
            &reader,
            count
        );

    log_info("\nRead Dynamic Header:\n");

    log_info("HLIT: ");
    log_number(hlit);
    log_info("\n");

    log_info("HDIST: ");
    log_number(hdist);
    log_info("\n");

    log_info("HCLEN: ");
    log_number(read_hclen);
    log_info("\n");

    static const int order[19] =
    {
        16, 17, 18,
        0, 8, 7, 9,
        6, 10, 5, 11,
        4, 12, 3, 13,
        2, 14, 1, 15
    };

    log_info("\nRead Code Length Lengths:\n");

    for (int i = 0; i < (int)read_hclen + 4; i++)
    {
        unsigned long length =
            bit_read_bits(
                &reader,
                3
            );

        log_info("symbol=");
        log_number(order[i]);

        log_info(" length=");
        log_number(length);

        log_info("\n");
    }

    log_info("\nRead Code Length RLE:\n");

    unsigned char decoded_lengths[286 + 30] = {0};
    int decoded_count = 0;

    while (decoded_count < literal_count + distance_count)
    {
        int count = 0;

        int symbol =
            huffman_decode(
                &reader,
                code_length_codes,
                19
            );

        if (symbol < 0)
        {
            sys_close(fd);
            return 1;
        }

        log_info("symbol=");
        log_number(symbol);

        if (symbol <= 15)
        {
            decoded_lengths[decoded_count++] =
                symbol;

            log_info("\n");
        }
        else if (symbol == 16)
        {
            count = 2;

            unsigned long extra =
                bit_read_bits(
                    &reader,
                    count
                );

            int repeat = extra + 3;

            if (decoded_count == 0)
            {
                sys_close(fd);
                return 1;
            }

            unsigned char length =
                decoded_lengths[decoded_count - 1];

            for (int i = 0; i < repeat; i++)
            {
                decoded_lengths[decoded_count++] =
                    length;
            }

            log_info(" repeat=");
            log_number(repeat);
            log_info("\n");
        }
        else if (symbol == 17)
        {
            count = 3;

            unsigned long extra =
                bit_read_bits(
                    &reader,
                    count
                );

            int repeat = extra + 3;

            for (int i = 0; i < repeat; i++)
            {
                decoded_lengths[decoded_count++] = 0;
            }

            log_info(" repeat=");
            log_number(repeat);
            log_info("\n");
        }
        else if (symbol == 18)
        {
            count = 7;

            unsigned long extra =
                bit_read_bits(
                    &reader,
                    count
                );

            int repeat = extra + 11;

            for (int i = 0; i < repeat; i++)
            {
                decoded_lengths[decoded_count++] = 0;
            }

            log_info(" repeat=");
            log_number(repeat);
            log_info("\n");
        }
        else
        {
            sys_close(fd);
            return 1;
        }
    }

    int valid_lengths = 1;

    for (int i = 0; i < literal_count; i++)
    {
        if (decoded_lengths[i] !=
            test_literal_lengths[i])
        {
            valid_lengths = 0;
            break;
        }
    }

    for (int i = 0; i < distance_count; i++)
    {
        if (decoded_lengths[literal_count + i] !=
            test_distance_lengths[i])
        {
            valid_lengths = 0;
            break;
        }
    }

    if (valid_lengths &&
        decoded_count == literal_count + distance_count)
    {
        log_info("RLE lengths: valid\n");
    }
    else
    {
        log_info("RLE lengths: INVALID\n");
    }

    sys_close(fd);

    return 0;
}