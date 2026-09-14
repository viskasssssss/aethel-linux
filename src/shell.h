#pragma once

typedef int (*command_function)(
    int argc,
    char **argv
);

typedef struct
{
    const char *name;

    command_function function;

    int min_arguments;
    int max_arguments;
}
command;