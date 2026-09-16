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

#pragma once

struct bit_reader
{
    int fd;
    unsigned long buffer;
    int bits;
};

void bit_reader_init(
    struct bit_reader *reader,
    int fd
);

int bit_read(
    struct bit_reader *reader
);

unsigned long bit_read_bits(
    struct bit_reader *reader,
    int count
);