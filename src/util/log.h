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

#include "syscall.h"

enum log_color
{
    LOG_COLOR_DEFAULT,
    LOG_COLOR_RED,
    LOG_COLOR_GREEN,
    LOG_COLOR_YELLOW,
    LOG_COLOR_BLUE,
    LOG_COLOR_MAGENTA,
    LOG_COLOR_CYAN,
    LOG_COLOR_WHITE
};

void log_write(const char *text);
void log_write_length(const char *text, long length);
void log_number(long number);
void log_number_padded(long number, long width);
void log_char(char character);

void log_color(enum log_color color);
void log_reset_color(void);

void log_info(const char *text);
void log_success(const char *text);
void log_warning(const char *text);
void log_error(const char *text);

void log_reset(void);