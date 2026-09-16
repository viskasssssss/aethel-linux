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

/*
B.A.T. (Basic Archive Tool) is a built-in Aethel Linux utility that provides a convenient interface for working with archives.
*/

#include "archive.h"
#include "log.h"

#define BAT_VERSION_MAJOR 0
#define BAT_VERSION_MINOR 1
#define BAT_VERSION_PATCH 0

#define BAT_VERSION_STRING \
    BAT_STRINGIFY(BAT_VERSION_MAJOR) "." \
    BAT_STRINGIFY(BAT_VERSION_MINOR) "." \
    BAT_STRINGIFY(BAT_VERSION_PATCH)

#define BAT_STRINGIFY(x) BAT_STRINGIFY_IMPL(x)
#define BAT_STRINGIFY_IMPL(x) #x

int string_comparsion(const char* s1, const char* s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        i++;
    }
    return (unsigned char)s1[i] - (unsigned char)s2[i];
}

enum bat_operation
{
    BAT_OPERATION_NONE,
    BAT_OPERATION_EXTRACT,
    BAT_OPERATION_VERSION
};

enum bat_archive_type
{
    BAT_ARCHIVE_GZIP,
    BAT_ARCHIVE_TAR
};

enum bat_compression
{
    BAT_COMPRESSION_GZIP,
    BAT_COMPRESSION_NONE
};

struct bat_arguments
{
    enum bat_operation operation;

    enum bat_archive_type archive;
    enum bat_compression compression;

    const char *input;
    const char *output;
};

int bat_parse_arguments(
    int argc,
    char **argv,
    struct bat_arguments *arguments
)
{
    for (int i = 1; i < argc; i++)
    {
        const char *argument = argv[i];

        if (argument[0] != '-')
        {
            log_error("bat: unexpected argument.\n");
            return 0;
        }

        if (string_comparsion(argument, "-e") == 0)
        {
            if (i + 1 >= argc)
            {
                log_error("bat: -e requires an archive.\n");
                return 0;
            }

            arguments->operation =
                BAT_OPERATION_EXTRACT;

            arguments->input = argv[++i];

            continue;
        }

        if (string_comparsion(argument, "-o") == 0)
        {
            if (i + 1 >= argc)
            {
                log_error("bat: -o requires a directory.\n");
                return 0;
            }

            arguments->output = argv[++i];

            continue;
        }

        if (string_comparsion(argument, "-tar") == 0)
        {
            arguments->archive =
                BAT_ARCHIVE_TAR;

            continue;
        }

        if (string_comparsion(argument, "-nc") == 0)
        {
            arguments->compression =
                BAT_COMPRESSION_NONE;

            continue;
        }

        if (string_comparsion(argument, "--version") == 0)
        {
            if (i + 1 < argc && argv[i + 1][0] != '-')
            {
                log_error("bat: --version doesn't require any arguments.\n");
                return 0;
            }

            arguments->operation = BAT_OPERATION_VERSION;
            continue;
        }

        log_error("bat: unknown option.\n");
        return 0;
    }

    return 1;
}

int bat_validate_arguments(
    const struct bat_arguments *arguments
)
{
    if (arguments->operation == BAT_OPERATION_NONE)
    {
        log_error("bat: no operation specified.\n");
        return 0;
    }

    if (arguments->operation == BAT_OPERATION_VERSION)
    {
        return 1;
    }

    if (arguments->archive == BAT_ARCHIVE_GZIP &&
        arguments->compression == BAT_COMPRESSION_NONE)
    {
        log_error(
            "bat: -nc cannot be used without -tar.\n"
        );

        return 0;
    }

    if (arguments->input == 0)
    {
        log_error("bat: no archive specified.\n");
        return 0;
    }

    if (arguments->output == 0)
    {
        log_error("bat: no output directory specified.\n");
        return 0;
    }

    return 1;
}

int bat_execute(
    const struct bat_arguments *arguments
)
{
    switch (arguments->operation)
    {
        case BAT_OPERATION_EXTRACT:
        {
            log_write("Extracting '");
            log_info(arguments->input);
            log_write("'\n...\n");

            int result;

            if (arguments->archive == BAT_ARCHIVE_GZIP)
            {
                result = archive_extract_gzip(
                    arguments->input,
                    arguments->output
                );
            }
            else if (arguments->archive == BAT_ARCHIVE_TAR)
            {
                if (arguments->compression ==
                    BAT_COMPRESSION_GZIP)
                {
                    result = archive_extract(
                        arguments->input,
                        arguments->output
                    );
                }
                else
                {
                    result = archive_extract_tar(
                        arguments->input,
                        arguments->output
                    );
                }
            }
            else
            {
                log_error("bat: unsupported archive type.\n");
                return 0;
            }

            if (result)
            {
                log_success(
                    "Archive extracted successfully.\n"
                );

                log_write("Output directory: '");
                log_info(arguments->output);
                log_write("'\n");
            }

            return result;
        }
        case BAT_OPERATION_VERSION:
            log_info("Aethel B.A.T. ");
            log_write(BAT_VERSION_STRING);
            log_info("\n");
            log_write("License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>\n");
            log_write("This is free software: you are free to change and redistribute it.\n");
            log_write("There is NO WARRANTY, to the extent permitted by law.\n");
            return 1;
        default:
            log_error("bat: unsupported operation.\n");
            return 0;
    }
}

int main(int argc, char **argv)
{
    struct bat_arguments arguments = {
        .operation = BAT_OPERATION_NONE,
        .archive = BAT_ARCHIVE_GZIP,
        .compression = BAT_COMPRESSION_GZIP,
        .input = 0,
        .output = 0
    };

    if (!bat_parse_arguments(
        argc,
        argv,
        &arguments
    ))
    {
        return 1;
    }

    if (!bat_validate_arguments(
        &arguments
    ))
    {
        return 1;
    }

    if (!bat_execute(
        &arguments
    ))
    {
        log_error("bat: operation failed\n");
        return 1;
    }

    return 0;
}