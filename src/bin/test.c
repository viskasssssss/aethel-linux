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
#include "gzip.h"
#include "log.h"

static int test_huffman(void)
{
    unsigned long literal_frequencies[286];

    for (int i = 0; i < 286; i++)
    {
        literal_frequencies[i] = 0;
    }

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

    if (!huffman_build(
        literal_codes,
        literal_lengths,
        286
    ))
    {
        return 0;
    }

    int kraft = 0;

    for (int i = 0; i < 286; i++)
    {
        if (literal_lengths[i] == 0)
            continue;

        kraft +=
            1 << (15 - literal_lengths[i]);
    }

    if (kraft != 32768)
    {
        return 0;
    }

    for (int i = 0; i < 286; i++)
    {
        if (literal_lengths[i] == 0)
            continue;

        for (int j = i + 1; j < 286; j++)
        {
            if (literal_lengths[j] == 0)
                continue;

            if (literal_lengths[i] ==
                    literal_lengths[j] &&
                literal_codes[i].code ==
                    literal_codes[j].code)
            {
                return 0;
            }
        }
    }

    return 1;
}

static int test_dynamic_header(void)
{
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
        deflate_get_literal_count(
            test_literal_lengths
        );

    int distance_count =
        deflate_get_distance_count(
            test_distance_lengths
        );

    if (!deflate_encode_code_lengths(
        test_literal_lengths,
        test_distance_lengths,
        literal_count,
        distance_count,
        symbols,
        &symbol_count
    ))
    {
        return 0;
    }

    unsigned long code_length_frequencies[19];

    if (!deflate_build_code_length_frequencies(
        symbols,
        symbol_count,
        code_length_frequencies
    ))
    {
        return 0;
    }

    unsigned char code_length_lengths[19];

    int code_length_count = 19;

    if (!huffman_build_lengths(
        code_length_frequencies,
        code_length_lengths,
        code_length_count
    ))
    {
        return 0;
    }

    if (!huffman_limit_lengths(
        code_length_lengths,
        code_length_frequencies,
        code_length_count,
        7
    ))
    {
        return 0;
    }

    struct huffman_code code_length_codes[19];

    if (!huffman_build(
        code_length_codes,
        code_length_lengths,
        code_length_count
    ))
    {
        return 0;
    }

    code_length_count =
        deflate_get_code_length_count(
            code_length_lengths
        );

    int fd = sys_openat(
        -100,
        "/tmp/dynamic-header",
        O_WRONLY | O_CREAT | O_TRUNC,
        0644
    );

    if (fd < 0)
        return 0;

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
        return 0;
    }

    if (!bit_writer_flush(&writer))
    {
        sys_close(fd);
        return 0;
    }

    sys_close(fd);

    fd = sys_openat(
        -100,
        "/tmp/dynamic-header",
        O_RDONLY,
        0
    );

    if (fd < 0)
        return 0;

    struct bit_reader reader;

    bit_reader_init(
        &reader,
        fd
    );

    unsigned long hlit =
        bit_read_bits(
            &reader,
            5
        );

    unsigned long hdist =
        bit_read_bits(
            &reader,
            5
        );

    unsigned long hclen =
        bit_read_bits(
            &reader,
            4
        );

    if (hlit != (unsigned long)(literal_count - 257) ||
        hdist != (unsigned long)(distance_count - 1) ||
        hclen != (unsigned long)(code_length_count - 4))
    {
        sys_close(fd);
        return 0;
    }

    static const int order[19] =
    {
        16, 17, 18,
        0, 8, 7, 9,
        6, 10, 5, 11,
        4, 12, 3, 13,
        2, 14, 1, 15
    };

    unsigned char read_code_length_lengths[19] = {0};

    for (int i = 0; i < (int)hclen + 4; i++)
    {
        unsigned long length =
            bit_read_bits(
                &reader,
                3
            );

        read_code_length_lengths[order[i]] =
            length;
    }

    for (int i = 0; i < 19; i++)
    {
        if (read_code_length_lengths[i] !=
            code_length_lengths[i])
        {
            sys_close(fd);
            return 0;
        }
    }

    unsigned char decoded_lengths[286 + 30] = {0};
    int decoded_count = 0;

    while (decoded_count <
           literal_count + distance_count)
    {
        int symbol =
            huffman_decode(
                &reader,
                code_length_codes,
                19
            );

        if (symbol < 0)
        {
            sys_close(fd);
            return 0;
        }

        if (symbol <= 15)
        {
            decoded_lengths[decoded_count++] =
                symbol;
        }
        else if (symbol == 16)
        {
            unsigned long extra =
                bit_read_bits(
                    &reader,
                    2
                );

            int repeat = extra + 3;

            if (decoded_count == 0)
            {
                sys_close(fd);
                return 0;
            }

            unsigned char length =
                decoded_lengths[decoded_count - 1];

            for (int i = 0; i < repeat; i++)
            {
                if (decoded_count >=
                    literal_count + distance_count)
                {
                    sys_close(fd);
                    return 0;
                }

                decoded_lengths[decoded_count++] =
                    length;
            }
        }
        else if (symbol == 17)
        {
            unsigned long extra =
                bit_read_bits(
                    &reader,
                    3
                );

            int repeat = extra + 3;

            for (int i = 0; i < repeat; i++)
            {
                if (decoded_count >=
                    literal_count + distance_count)
                {
                    sys_close(fd);
                    return 0;
                }

                decoded_lengths[decoded_count++] = 0;
            }
        }
        else if (symbol == 18)
        {
            unsigned long extra =
                bit_read_bits(
                    &reader,
                    7
                );

            int repeat = extra + 11;

            for (int i = 0; i < repeat; i++)
            {
                if (decoded_count >=
                    literal_count + distance_count)
                {
                    sys_close(fd);
                    return 0;
                }

                decoded_lengths[decoded_count++] = 0;
            }
        }
        else
        {
            sys_close(fd);
            return 0;
        }
    }

    for (int i = 0; i < literal_count; i++)
    {
        if (decoded_lengths[i] !=
            test_literal_lengths[i])
        {
            sys_close(fd);
            return 0;
        }
    }

    for (int i = 0; i < distance_count; i++)
    {
        if (decoded_lengths[literal_count + i] !=
            test_distance_lengths[i])
        {
            sys_close(fd);
            return 0;
        }
    }

    sys_close(fd);

    return 1;
}

static int test_dynamic_data(void)
{
    unsigned char literal_lengths[286] = {0};
    unsigned char distance_lengths[30] = {0};

    literal_lengths[256] = 1;
    literal_lengths[257] = 1;

    distance_lengths[0] = 1;

    struct huffman_code literal_codes[286];
    struct huffman_code distance_codes[30];

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

    int fd = sys_openat(
        -100,
        "/tmp/dynamic-data",
        O_WRONLY | O_CREAT | O_TRUNC,
        0644
    );

    if (fd < 0)
        return 0;

    struct bit_writer writer;

    bit_writer_init(
        &writer,
        fd
    );

    if (!deflate_write_dynamic_match(
        &writer,
        literal_codes,
        distance_codes,
        3,
        1
    ))
    {
        sys_close(fd);
        return 0;
    }

    if (!deflate_write_end(
        &writer,
        literal_codes
    ))
    {
        sys_close(fd);
        return 0;
    }

    if (!bit_writer_flush(&writer))
    {
        sys_close(fd);
        return 0;
    }

    sys_close(fd);

    fd = sys_openat(
        -100,
        "/tmp/dynamic-data",
        O_RDONLY,
        0
    );

    if (fd < 0)
        return 0;

    struct bit_reader reader;

    bit_reader_init(
        &reader,
        fd
    );

    int symbol =
        huffman_decode(
            &reader,
            literal_codes,
            286
        );

    if (symbol != 257)
    {
        sys_close(fd);
        return 0;
    }

    symbol =
        huffman_decode(
            &reader,
            distance_codes,
            30
        );

    if (symbol != 0)
    {
        sys_close(fd);
        return 0;
    }

    symbol =
        huffman_decode(
            &reader,
            literal_codes,
            286
        );

    if (symbol != 256)
    {
        sys_close(fd);
        return 0;
    }

    sys_close(fd);

    return 1;
}

static int test_dynamic_block(void)
{
    const unsigned char data[] = "AAAAAA";

    int fd = sys_openat(
        -100,
        "/tmp/dynamic-block",
        O_WRONLY | O_CREAT | O_TRUNC,
        0644
    );

    if (fd < 0)
        return 0;

    struct bit_writer writer;

    bit_writer_init(
        &writer,
        fd
    );

    if (!deflate_write_dynamic_block(
        &writer,
        data,
        6,
        1
    ))
    {
        sys_close(fd);
        return 0;
    }

    if (!bit_writer_flush(&writer))
    {
        sys_close(fd);
        return 0;
    }

    sys_close(fd);

    log_info("Dynamic block written\n");

    return 1;
}

static int test_gzip_dynamic(void)
{
    int fd = sys_openat(
        -100,
        "/tmp/gzip-input",
        O_WRONLY | O_CREAT | O_TRUNC,
        0644
    );

    if (fd < 0)
        return 0;

    const unsigned char data[] = "AAAAAA";

    if (sys_write(fd, data, 6) != 6)
    {
        sys_close(fd);
        return 0;
    }

    sys_close(fd);

    if (!gzip_create(
        "/tmp/gzip-input",
        "/tmp/gzip-output",
        GZIP_COMPRESSION_DYNAMIC
    ))
    {
        return 0;
    }

    return 1;
}

int main(
    int *argc,
    char **argv
)
{
    (void)argc;
    (void)argv;

    //if (!test_huffman())
    //    return 1;

    //if (!test_dynamic_header())
    //    return 1;

    //if (!test_dynamic_data())
    //    return 1;

    //if (!test_dynamic_block())
    //    return 1;

    if (!test_gzip_dynamic())
        return 1;

    log_info("All tests passed\n");

    return 0;
}