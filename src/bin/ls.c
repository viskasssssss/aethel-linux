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
#include "syscall.h"

static const char *months[] =
{
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
};

struct ls_entry
{
    char name[256];

    struct stat information;

    long size;
};

static long number_width(
    long number
)
{
    long width = 1;

    if (number < 0)
    {
        width++;
        number = -number;
    }

    while (number >= 10)
    {
        number /= 10;
        width++;
    }

    return width;
}

static void log_number_aligned(
    long number,
    long width
)
{
    long current_width =
        number_width(number);

    while (current_width < width)
    {
        log_char(' ');
        current_width++;
    }

    log_number(number);
}

static void print_mode(
    unsigned int mode
)
{
    char type;

    switch (mode & S_IFMT)
    {
        case S_IFREG:
            type = '-';
            break;

        case S_IFDIR:
            type = 'd';
            break;

        case S_IFLNK:
            type = 'l';
            break;

        case S_IFCHR:
            type = 'c';
            break;

        case S_IFBLK:
            type = 'b';
            break;

        case S_IFIFO:
            type = 'p';
            break;

        case S_IFSOCK:
            type = 's';
            break;

        default:
            type = '?';
            break;
    }

    log_char(type);

    log_char(mode & S_IRUSR ? 'r' : '-');
    log_char(mode & S_IWUSR ? 'w' : '-');
    log_char(mode & S_IXUSR ? 'x' : '-');

    log_char(mode & S_IRGRP ? 'r' : '-');
    log_char(mode & S_IWGRP ? 'w' : '-');
    log_char(mode & S_IXGRP ? 'x' : '-');

    log_char(mode & S_IROTH ? 'r' : '-');
    log_char(mode & S_IWOTH ? 'w' : '-');
    log_char(mode & S_IXOTH ? 'x' : '-');
}

static void print_time(
    long timestamp
)
{
    long days = timestamp / 86400;
    long seconds = timestamp % 86400;

    long hour = seconds / 3600;
    seconds %= 3600;

    long minute = seconds / 60;

    /*
     * Unix epoch starts at:
     *
     * 1970-01-01
     */
    long year = 1970;

    while (1)
    {
        long days_in_year = 365;

        if ((year % 4 == 0 &&
             year % 100 != 0) ||
            year % 400 == 0)
        {
            days_in_year = 366;
        }

        if (days < days_in_year)
            break;

        days -= days_in_year;
        year++;
    }

    long month = 0;

    static const long month_days[] =
    {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    while (month < 12)
    {
        long days_in_month =
            month_days[month];

        if (month == 1 &&
            ((year % 4 == 0 &&
              year % 100 != 0) ||
             year % 400 == 0))
        {
            days_in_month = 29;
        }

        if (days < days_in_month)
            break;

        days -= days_in_month;
        month++;
    }

    /*
     * days is now zero-based day
     * inside the current month.
     */
    log_write(months[month]);
    log_char(' ');

    log_number(days + 1);
    log_char(' ');

    log_number_padded(hour, 2);
    log_char(':');
    log_number_padded(minute, 2);
}

int main(int argc, char **argv) {
    int long_format = 0;
    const char *path = ".";

    if (argc > 1)
    {
        if (argv[1][0] == '-' &&
            argv[1][1] == 'l' &&
            argv[1][2] == '\0')
        {
            long_format = 1;
        }
        else
        {
            path = argv[1];
        }
    }

    int fd = sys_openat(
        -100, 
        path, 
        0, 
        0
    );

    if (fd < 0)
    {
        log_error("ls: cannot open directory\n");

        return 1;
    }

    char buffer[4096];

    long count = sys_getdents64(
        fd, 
        buffer, 
        sizeof(buffer)
    );

    if (count < 0)
    {
        log_error("ls: cannot read directory\n");

        sys_close(fd);

        return 1;
    }

    long position = 0;

    struct ls_entry entries[256];

    long entry_count = 0;

    if (!long_format)
    {
        while (position < count)
        {
            struct linux_dirent64 *entry =
                (struct linux_dirent64 *)
                (buffer + position);

            if (entry->name[0] == '.')
            {
                position += entry->record_length;
                continue;
            }

            if (entry->type == DT_DIR)
            {
                log_info(entry->name);
                log_write("/");
            }
            else
            {
                log_write(entry->name);
            }
            log_write("\n");

            position += entry->record_length;
        }

        sys_close(fd);
        return 0;
    }

    while (position < count)
    {
        struct linux_dirent64 *entry =
            (struct linux_dirent64 *)
            (buffer + position);
        
        if (entry->name[0] == '.')
        {
            position += entry->record_length;
            continue;
        }

        if (entry_count >= 256)
        {
            break;
        }

        struct ls_entry *item =
            &entries[entry_count];

        long length = 0;

        while (
            entry->name[length] != '\0' &&
            length < sizeof(item->name) - 1
        )
        {
            item->name[length] =
                entry->name[length];

            length++;
        }

        item->name[length] = '\0';

        if (sys_newfstatat(
            fd,
            entry->name,
            &item->information,
            0
        ) < 0)
        {
            position += entry->record_length;
            continue;
        }

        item->size = item->information.size;

        entry_count++;

        position += entry->record_length;
    }

    long max_links = 1;
    long max_uid = 1;
    long max_gid = 1;
    long max_size = 1;

    for (long i = 0; i < entry_count; i++)
    {
        struct stat *information =
            &entries[i].information;

        long width;

        width = number_width(information->nlink);

        if (width > max_links)
            max_links = width;

        width = number_width(information->uid);

        if (width > max_uid)
            max_uid = width;

        width = number_width(information->gid);

        if (width > max_gid)
            max_gid = width;

        width = number_width(information->size);

        if (width > max_size)
            max_size = width;
    }

    for (long i = 0; i < entry_count; i++)
    {
        struct stat *information =
            &entries[i].information;

        print_mode(information->mode);
        log_char(' ');

        log_number_aligned(
            information->nlink,
            max_links
        );
        log_char(' ');

        log_number_aligned(
            information->uid,
            max_uid
        );
        log_char(' ');

        log_number_aligned(
            information->gid,
            max_gid
        );
        log_char(' ');

        log_number_aligned(
            information->size,
            max_size
        );
        log_char(' ');

        print_time(information->mtime);
        log_char(' ');

        log_write(entries[i].name);

        if ((information->mode & S_IFMT) == S_IFDIR)
            log_char('/');

        log_char('\n');
    }

    sys_close(fd);

    return 0;
}