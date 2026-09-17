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

static int gzip_write_header(int fd)
{
    unsigned char header[10] = {
        0x1f,
        0x8b,
        0x08,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x03
    };

    return sys_write(
        fd,
        header,
        sizeof(header)
    ) == sizeof(header);
}

static int gzip_write_u16(
    int fd,
    unsigned short value
)
{
    unsigned char buffer[2];

    buffer[0] = value & 0xff;
    buffer[1] = (value >> 8) & 0xff;

    return sys_write(
        fd,
        buffer,
        2
    ) == 2;
}

static int gzip_write_stored_block(
    int archive,
    const unsigned char *buffer,
    unsigned short size,
    int final
)
{
    unsigned char header =
        final ? 0x01 : 0x00;

    if (sys_write(
        archive,
        &header,
        1
    ) != 1)
    {
        return 0;
    }

    if (!gzip_write_u16(
        archive,
        size
    ))
    {
        return 0;
    }

    if (!gzip_write_u16(
        archive,
        ~size
    ))
    {
        return 0;
    }

    if (size > 0)
    {
        if (sys_write(
            archive,
            buffer,
            size
        ) != size)
        {
            return 0;
        }
    }

    return 1;
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
    int *fd,
    struct deflate_output *output,
    int debug
)
{
    struct gzip_header header;

    if (!gzip_read_header(*fd, &header))
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
        *fd
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

    
    if (debug)
    {
        log_write("BFINAL: ");
        log_number(final);
        log_write("\n");
        
        log_write("BTYPE: ");
    }
    
    long output_size = 0;
    
    if (type == DEFLATE_BLOCK_STORED)
    {
        if (!deflate_read_stored_block(
            &reader,
            output
        ))
        {
            return -1;
        }

        output_size = output->size;
    }
    else 
    {
        struct huffman_code literal_codes[288];
        struct huffman_code distance_codes[32];

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
                if (!deflate_output_write(
                    output,
                    (unsigned char)symbol
                ))
                {
                    return -1;
                }

                output_size++;

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
                    unsigned char value;

                    if (!deflate_output_read(
                        output,
                        distance,
                        &value
                    ))
                    {
                        return -1;
                    }

                    if (!deflate_output_write(
                        output,
                        value
                    ))
                    {
                        return -1;
                    }

                    output_size++;
                }

                continue;
            }

            return -1;
        }
    }

    if (!deflate_output_flush(output))
    {
        return -1;
    }

    reader.bits = 0;
    reader.buffer = 0;

    struct gzip_footer footer;

    if (!gzip_read_footer(*fd, &footer))
    {
        return -1;
    }

    unsigned long expected_crc =
        read_le32(footer.crc32);

    unsigned long expected_size =
        read_le32(footer.isize);

    unsigned long actual_crc =
        crc32_finish(output->crc);

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

int gzip_create(
    const char *source,
    const char *destination,
    enum gzip_compression compression
)
{
    int input = sys_openat(
        -100,
        source,
        O_RDONLY,
        0
    );

    if (input < 0)
    {
        return 0;
    }

    int output = sys_openat(
        -100,
        destination,
        O_WRONLY | O_CREAT | O_TRUNC,
        0644
    );

    if (output < 0)
    {
        sys_close(input);
        return 0;
    }

    if (!gzip_write_header(output))
    {
        sys_close(input);
        sys_close(output);
        return 0;
    }

    struct bit_writer writer;

    bit_writer_init(
        &writer,
        output
    );

    unsigned char buffer_a[65535];
    unsigned char buffer_b[65535];

    unsigned char *current = buffer_a;
    unsigned char *next = buffer_b;

    long current_size = sys_read(
        input,
        current,
        sizeof(buffer_a)
    );

    if (current_size < 0)
    {
        sys_close(input);
        sys_close(output);
        return 0;
    }

    unsigned long checksum = 0xFFFFFFFF;
    unsigned long total_size = 0;

    if (current_size == 0)
    {
        int result;

        if (compression == GZIP_COMPRESSION_NONE)
        {
            result = gzip_write_stored_block(
                output,
                current,
                0,
                1
            );
        }
        else if (compression == GZIP_COMPRESSION_FIXED)
        {
            result = deflate_write_fixed_block(
                &writer,
                current,
                0,
                1
            );
        }
        else if (compression == GZIP_COMPRESSION_DYNAMIC) 
        {
            result = deflate_write_dynamic_block( 
                &writer, 
                current, 
                0, 
                1 
            );
        }
        else
        {
            result = 0;
        }

        if (!result)
        {
            sys_close(input);
            sys_close(output);
            return 0;
        }
    }
    else
    {
        while (1)
        {
            long next_size = sys_read(
                input,
                next,
                sizeof(buffer_b)
            );

            if (next_size < 0)
            {
                sys_close(input);
                sys_close(output);
                return 0;
            }

            int final = next_size == 0;

            int result;

            if (compression == GZIP_COMPRESSION_NONE)
            {
                result = gzip_write_stored_block(
                    output,
                    current,
                    (unsigned short)current_size,
                    final
                );
            }
            else if (compression == GZIP_COMPRESSION_FIXED)
            {
                result = deflate_write_fixed_block(
                    &writer,
                    current,
                    current_size,
                    final
                );
            }
            else if (compression == GZIP_COMPRESSION_DYNAMIC) 
            { 
                result = deflate_write_dynamic_block( 
                    &writer, 
                    current, 
                    current_size, 
                    final 
                );
            }
            else
            {
                result = 0;
            }

            if (!result)
            {
                sys_close(input);
                sys_close(output);
                return 0;
            }

            checksum = crc32_update(
                checksum,
                current,
                current_size
            );

            total_size += current_size;

            if (final)
            {
                break;
            }

            unsigned char *temporary = current;
            current = next;
            next = temporary;

            current_size = next_size;
        }
    }

    if (compression == GZIP_COMPRESSION_FIXED ||
        compression == GZIP_COMPRESSION_DYNAMIC)
    {
        if (!bit_writer_flush(&writer))
        {
            sys_close(input);
            sys_close(output);
            return 0;
        }
    }

    checksum = crc32_finish(checksum);

    unsigned char footer[8];

    footer[0] = checksum & 0xff;
    footer[1] = (checksum >> 8) & 0xff;
    footer[2] = (checksum >> 16) & 0xff;
    footer[3] = (checksum >> 24) & 0xff;

    footer[4] = total_size & 0xff;
    footer[5] = (total_size >> 8) & 0xff;
    footer[6] = (total_size >> 16) & 0xff;
    footer[7] = (total_size >> 24) & 0xff;

    if (sys_write(
        output,
        footer,
        sizeof(footer)
    ) != sizeof(footer))
    {
        sys_close(input);
        sys_close(output);
        return 0;
    }

    sys_close(input);
    sys_close(output);

    return 1;
}