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

#include "gzip.h"
#include "bit.h"
#include "deflate.h"
#include "huffman.h"
#include "crc32.h"
#include "syscall.h"
#include "log.h"

static unsigned long read_le32(
    const unsigned char *data
)
{
    return
        ((unsigned long)data[0]) |
        ((unsigned long)data[1] << 8) |
        ((unsigned long)data[2] << 16) |
        ((unsigned long)data[3] << 24);
}

int gzip_open(const char *path)
{
    return sys_openat(
        -100,
        path,
        O_RDONLY,
        0
    );
}

int gzip_read_header(
    int fd,
    struct gzip_header *header
)
{
    long result = sys_read(
        fd,
        header,
        10
    );

    if (result != 10)
    {
        return 0;
    }

    if (header->id1 != 0x1f ||
        header->id2 != 0x8b)
    {
        return 0;
    }

    if (header->compression != 8)
    {
        return 0;
    }

    if (header->flags & GZIP_FLAG_FNAME)
    {
        unsigned char byte;

        do
        {
            result = sys_read(
                fd,
                &byte,
                1
            );

            if (result != 1)
            {
                return 0;
            }

        } while (byte != 0);
    }

    return 1;
}

int gzip_read_footer(
    int fd,
    struct gzip_footer *footer
)
{
    long result = sys_read(
        fd,
        footer,
        8
    );

    if (result != 8)
    {
        return 0;
    }

    return 1;
}

long gzip_read_file(
    int fd,
    unsigned char *output,
    long output_capacity,
    int debug
)
{
    struct gzip_header header;

    if (!gzip_read_header(fd, &header))
    {
        return -1;
    }

    if (debug)
    {
        log_success("valid gzip header\n");
    }

    struct bit_reader reader;

    bit_reader_init(
        &reader,
        fd
    );

    int final;
    enum deflate_block_type type;

    if (!deflate_read_block_header(
        &reader,
        &final,
        &type
    ))
    {
        return -1;
    }

    struct huffman_code literal_codes[288];
    struct huffman_code distance_codes[32];

    if (debug)
    {
        log_write("BFINAL: ");
        log_number(final);
        log_write("\n");

        log_write("BTYPE: ");
    }

    if (type == DEFLATE_BLOCK_FIXED)
    {
        if (debug)
        {
            log_write("fixed\n");
        }

        huffman_build_fixed(
            literal_codes,
            distance_codes
        );
    }
    else if (type == DEFLATE_BLOCK_DYNAMIC)
    {
        if (debug)
        {
            log_write("dynamic\n");
        }

        struct deflate_dynamic_header dynamic_header;

        if (!deflate_read_dynamic_header(
            &reader,
            &dynamic_header
        ))
        {
            return -1;
        }

        unsigned char code_lengths[19] = { 0 };

        if (!deflate_read_code_lengths(
            &reader,
            dynamic_header.code_length_count,
            code_lengths
        ))
        {
            return -1;
        }

        struct huffman_code code_length_codes[19];

        huffman_build(
            code_length_codes,
            code_lengths,
            19
        );

        int total_codes =
            dynamic_header.literal_count +
            dynamic_header.distance_count;

        unsigned char dynamic_lengths[320] = { 0 };

        if (!deflate_read_dynamic_lengths(
            &reader,
            code_length_codes,
            total_codes,
            dynamic_lengths
        ))
        {
            return -1;
        }

        unsigned char literal_lengths[288] = { 0 };
        unsigned char distance_lengths[32] = { 0 };

        for (int i = 0;
             i < dynamic_header.literal_count;
             i++)
        {
            literal_lengths[i] =
                dynamic_lengths[i];
        }

        for (int i = 0;
             i < dynamic_header.distance_count;
             i++)
        {
            distance_lengths[i] =
                dynamic_lengths[
                    dynamic_header.literal_count + i
                ];
        }

        huffman_build(
            literal_codes,
            literal_lengths,
            288
        );

        huffman_build(
            distance_codes,
            distance_lengths,
            32
        );

        if (debug)
        {
            log_write("HLIT: ");
            log_number(dynamic_header.literal_count);
            log_write("\n");

            log_write("HDIST: ");
            log_number(dynamic_header.distance_count);
            log_write("\n");

            log_write("HCLEN: ");
            log_number(dynamic_header.code_length_count);
            log_write("\n");
        }
    }
    else
    {
        return -1;
    }

    long output_size = 0;

    while (1)
    {
        int symbol = huffman_decode(
            &reader,
            literal_codes,
            288
        );

        if (symbol < 0)
        {
            return -1;
        }

        if (symbol < 256)
        {
            if (output_size >= output_capacity)
            {
                return -1;
            }

            output[output_size++] =
                (unsigned char)symbol;

            continue;
        }

        if (symbol == 256)
        {
            break;
        }

        if (symbol >= 257 && symbol <= 285)
        {
            int length = deflate_decode_length(
                &reader,
                symbol
            );

            if (length < 0)
            {
                return -1;
            }

            int distance_symbol = huffman_decode(
                &reader,
                distance_codes,
                32
            );

            if (distance_symbol < 0)
            {
                return -1;
            }

            int distance = deflate_decode_distance(
                &reader,
                distance_symbol
            );

            if (distance < 0 ||
                distance > output_size)
            {
                return -1;
            }

            for (int i = 0; i < length; i++)
            {
                if (output_size >= output_capacity)
                {
                    return -1;
                }

                output[output_size] =
                    output[output_size - distance];

                output_size++;
            }

            continue;
        }

        return -1;
    }

    reader.bits = 0;
    reader.buffer = 0;

    struct gzip_footer footer;

    if (!gzip_read_footer(fd, &footer))
    {
        return -1;
    }

    unsigned long expected_crc =
        read_le32(footer.crc32);

    unsigned long expected_size =
        read_le32(footer.isize);

    unsigned long actual_crc =
        crc32(output, output_size);

    if (debug)
    {
        log_write("CRC32: ");

        if (actual_crc == expected_crc)
        {
            log_write("OK\n");
        }
        else
        {
            log_write("FAILED\n");
        }

        log_write("ISIZE: ");

        if (expected_size ==
            (unsigned long)output_size)
        {
            log_write("OK\n");
        }
        else
        {
            log_write("FAILED\n");
        }
    }

    if (actual_crc != expected_crc ||
        expected_size != (unsigned long)output_size)
    {
        return -1;
    }

    return output_size;
}