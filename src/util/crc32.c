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

#include "crc32.h"

unsigned long crc32(
    const unsigned char *data,
    long size
)
{
    unsigned long crc = 0xFFFFFFFF;

    for (long i = 0; i < size; i++)
    {
        crc ^= data[i];

        for (int bit = 0; bit < 8; bit++)
        {
            if (crc & 1)
            {
                crc =
                    (crc >> 1) ^
                    0xEDB88320;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFF;
}