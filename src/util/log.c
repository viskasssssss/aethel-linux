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

#include "log.h"

static const char *color_codes[] =
{
    "\033[0m",
    "\033[31m",
    "\033[32m",
    "\033[33m",
    "\033[34m",
    "\033[35m",
    "\033[36m",
    "\033[37m"
};

void log_write(const char *text)
{
    long length = 0;

    while (text[length] != '\0')
    {
        length++;
    }

    sys_write(
        1,
        text,
        length
    );
}

void log_write_length(
    const char *text,
    long length
)
{
    sys_write(
        1,
        text,
        length
    );
}

void log_color(enum log_color color)
{
    if (color < LOG_COLOR_DEFAULT ||
        color > LOG_COLOR_WHITE)
    {
        return;
    }

    const char *code = color_codes[color];

    long length = 0;

    while (code[length] != '\0')
    {
        length++;
    }

    sys_write(
        1,
        code,
        length
    );
}

void log_reset_color(void)
{
    log_color(LOG_COLOR_DEFAULT);
}

void log_info(const char *text)
{
    log_color(LOG_COLOR_CYAN);
    log_write(text);
    log_reset_color();
}

void log_success(const char *text)
{
    log_color(LOG_COLOR_GREEN);
    log_write(text);
    log_reset_color();
}

void log_warning(const char *text)
{
    log_color(LOG_COLOR_YELLOW);
    log_write(text);
    log_reset_color();
}

void log_error(const char *text)
{
    log_color(LOG_COLOR_RED);
    log_write(text);
    log_reset_color();
}

void log_reset(void)
{
    const char *code = "\033[0m";

    sys_write(
        1,
        code,
        4
    );
}