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

#include "bit.h"
#include "syscall.h"

void bit_reader_init(
    struct bit_reader *reader,
    int fd
)
{
    reader->fd = fd;
    reader->buffer = 0;
    reader->bits = 0;
}

int bit_read(
    struct bit_reader *reader
)
{
    if (reader->bits == 0)
    {
        unsigned char byte;

        long result = sys_read(
            reader->fd,
            &byte,
            1
        );

        if (result != 1)
        {
            return -1;
        }

        reader->buffer = byte;
        reader->bits = 8;
    }

    int bit = reader->buffer & 1;

    reader->buffer >>= 1;
    reader->bits--;

    return bit;
}

unsigned long bit_read_bits(
    struct bit_reader *reader,
    int count
)
{
    unsigned long result = 0;

    for (int i = 0; i < count; i++)
    {
        int bit = bit_read(reader);

        if (bit < 0)
        {
            return 0;
        }

        result |= (unsigned long)bit << i;
    }

    return result;
}

void bit_align(
    struct bit_reader *reader
)
{
    reader->buffer = 0;
    reader->bits = 0;
}

void bit_writer_init(
    struct bit_writer *writer,
    int fd
)
{
    writer->fd = fd;
    writer->buffer = 0;
    writer->bits = 0;
}

int bit_write(
    struct bit_writer *writer,
    unsigned long value,
    int count
)
{
    for (int i = 0; i < count; i++)
    {
        writer->buffer |=
            (value & 1) << writer->bits;

        value >>= 1;
        writer->bits++;

        if (writer->bits == 8)
        {
            unsigned char byte =
                writer->buffer;

            if (sys_write(
                writer->fd,
                &byte,
                1
            ) != 1)
            {
                return 0;
            }

            writer->buffer = 0;
            writer->bits = 0;
        }
    }

    return 1;
}

int bit_writer_flush(
    struct bit_writer *writer
)
{
    if (writer->bits == 0)
    {
        return 1;
    }

    unsigned char byte =
        writer->buffer;

    if (sys_write(
        writer->fd,
        &byte,
        1
    ) != 1)
    {
        return 0;
    }

    writer->buffer = 0;
    writer->bits = 0;

    return 1;
}

void bit_writer_align(
    struct bit_writer *writer
)
{
    writer->buffer = 0;
    writer->bits = 0;
}