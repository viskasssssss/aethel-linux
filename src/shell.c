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

#include "syscall.h"
#include "shell.h"
#include "log.h"

#define TCGETS 0x5401
#define TCSETS 0x5402

#define ICANON 0x00000002
#define ECHO   0x00000008

#define VMIN 6
#define VTIME 5

#define DT_DIR 4

static char *path_value;

static char profile[256];

static char environment_storage[32][256];

static char *environment[32] = {
    0
};

static int environment_count = 0;

static int last_status = 0;

struct termios
{
    unsigned int input_flags;
    unsigned int output_flags;
    unsigned int control_flags;
    unsigned int local_flags;

    unsigned char line;

    unsigned char control_characters[19];

    unsigned int input_speed;
    unsigned int output_speed;
};

static long string_length(
    const char *string
)
{
    long length = 0;

    while (string[length] != '\0')
        length++;

    return length;
}

static int string_equals(
    const char *a,
    const char *b
)
{
    long i = 0;

    while (
        a[i] != '\0' &&
        b[i] != '\0'
    )
    {
        if (a[i] != b[i])
            return 0;

        i++;
    }

    return a[i] == b[i];
}

static void enable_raw_input(void)
{
    struct termios terminal;

    sys_ioctl(
        0,
        TCGETS,
        &terminal
    );

    terminal.local_flags &= ~ICANON;
    terminal.local_flags &= ~ECHO;

    terminal.control_characters[VMIN] = 1;
    terminal.control_characters[VTIME] = 0;

    sys_ioctl(
        0,
        TCSETS,
        &terminal
    );
}

static void load_profile(
    char *path,
    long size
)
{
    long fd = sys_openat(
        -100,
        "/etc/profile",
        0,
        0
    );

    if (fd < 0)
        return;

    long count = sys_read(
        fd,
        path,
        size - 1
    );

    sys_close(fd);

    if (count <= 0)
        return;

    path[count] = '\0';
}

static char *get_path_value(
    char *profile
)
{
    const char prefix[] = "PATH=";

    for (int i = 0; prefix[i] != '\0'; i++)
    {
        if (profile[i] != prefix[i])
            return 0;
    }

    return profile + 5;
}

static char *find_next_path(
    char **path
)
{
    if (*path == 0)
        return 0;

    char *start = *path;

    for (long i = 0; start[i] != '\0'; i++)
    {
        if (start[i] == ':')
        {
            start[i] = '\0';
            *path = start + i + 1;

            return start;
        }
    }

    *path = 0;

    return start;
}


static int environment_name_equals(
    const char *environment,
    const char *name
)
{
    long i = 0;

    while (name[i] != '\0')
    {
        if (environment[i] != name[i])
            return 0;

        i++;
    }

    return environment[i] == '=';
}

static int set_environment(
    const char *value
)
{
    long equals = -1;

    for (long i = 0; value[i] != '\0'; i++)
    {
        if (value[i] == '=')
        {
            equals = i;
            break;
        }
    }

    if (equals <= 0)
        return -1;

    long length = string_length(value);

    if (length >= 256)
        return -1;

    for (int i = 0; i < environment_count; i++)
    {
        int same_name = 1;

        for (long j = 0; j < equals; j++)
        {
            if (environment[i][j] != value[j])
            {
                same_name = 0;
                break;
            }
        }

        if (same_name && environment[i][equals] == '=')
        {
            for (long j = 0; j <= length; j++)
            {
                environment_storage[i][j] = value[j];
            }

            return 0;
        }
    }

    if (environment_count >= 32)
        return -1;

    for (long i = 0; i <= length; i++)
    {
        environment_storage[environment_count][i] = value[i];
    }

    environment[environment_count] =
        environment_storage[environment_count];

    environment_count++;

    return 0;
}

// BASIC

static int command_help(
    int argc,
    char **argv
)
{
    const char message[] =
        "Aethel shell\n"
        "Commands:\n"
        "  help\n"
        "  echo <text>\n"
        "  ls <dir>\n"
        "  cd <dir>\n"
        "  pwd\n"
        "  cat <file>\n"
        "  mkdir <file>\n"
        "  rmdir <file>\n"
        "  touch <file>\n"
        "  rm <file>\n"
        "  cp <a> <b>\n"
        "  mv <a> <b>\n"
        "  env\n"
        "  export <var>\n"
        "  unset <var>\n"
        "  exit\n";

    log_info(message);

    return 0;
}

static int command_echo(
    int argc,
    char **argv
)
{
    for (int i = 1; i < argc; i++)
    {
        log_write(argv[i]);

        if (i + 1 < argc)
            log_write(" ");
    }

    log_write("\n");

    return 0;
}

static int command_exit(
    int argc,
    char **argv
)
{
    // init process will restart shell automatically when it exits, so this is useless
    // TODO: remove this command

    sys_exit(0);

    return 0;
}

// FILE SYSTEM

struct linux_dirent64
{
    unsigned long inode;
    long offset;

    unsigned short record_length;
    unsigned char type;

    char name[];
};

static int command_ls(
    int argc,
    char **argv
)
{
    const char *path = ".";

    if (argc > 1)
        path = argv[1];

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

    while (position < count)
    {
        struct linux_dirent64 *entry =
            (struct linux_dirent64 *)
            (buffer + position);

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

static int command_cd(
    int argc,
    char **argv
)
{
    if (argc < 2)
    {
        log_error("cd: missing argument\n");

        return 1;
    }

    long result = sys_chdir(argv[1]);

    if (result < 0)
    {
        log_error("cd: cannot change directory\n");

        return 1;
    }

    return 0;
}

static int command_pwd(
    int argc,
    char **argv
)
{
    char buffer[256];

    long count = sys_readlink(
        "/proc/self/cwd",
        buffer,
        sizeof(buffer) - 1
    );

    if (count < 0)
    {
        log_error("pwd: cannot get current directory\n");

        return 1;
    }

    log_write(buffer);
    log_write("\n");

    return 0;
}

static int command_cat(
    int argc,
    char **argv
)
{
    int fd = 0;

    if (argc == 2)
    {
        fd = sys_openat(
            -100,
            argv[1],
            0,
            0
        );

        if (fd < 0)
        {
            return 1;
        }
    }

    char buffer[512];

    while (1)
    {
        long count = sys_read(
            fd,
            buffer,
            sizeof(buffer)
        );

        if (count <= 0)
        {
            break;
        }

        log_write(buffer);
    }

    if (argc == 2)
    {
        sys_close(fd);
    }

    return 0;
}

static int command_mkdir(
    int argc,
    char **argv
)
{
    if (argc < 2)
    {
        log_error("mkdir: missing operand\n");

        return 1;
    }

    long result = sys_mkdir(
        argv[1],
        0755
    );

    if (result < 0)
    {
        log_error("mkdir: cannot create directory\n");

        return 1;
    }

    return 0;
}

static int command_touch(
    int argc,
    char **argv
)
{
    if (argc < 2)
    {
        log_error("touch: missing operand\n");

        return 1;
    }

    int fd = sys_openat(
        -100,
        argv[1],
        64,
        0644
    );

    if (fd < 0)
    {
        log_error("touch: cannot create file\n");

        return 1;
    }

    sys_close(fd);

    return 0;
}

static int command_rm(
    int argc,
    char **argv
)
{
    if (argc < 2)
    {
        log_error("rm: missing operand\n");

        return 1;
    }

    long result = sys_unlink(
        argv[1]
    );

    if (result < 0)
    {
        log_error("rm: cannot remove file\n");

        return 1;
    }

    return 0;
}

static int command_cp(
    int argc,
    char **argv
)
{
    if (argc < 3)
    {
        log_error("cp: missing operand\n");

        return 1;
    }

    int source = sys_openat(
        -100,
        argv[1],
        0,
        0
    );

    if (source < 0)
    {
        log_error("cp: cannot open source\n");

        return 1;
    }

    int destination = sys_openat(
        -100,
        argv[2],
        577,
        0644
    );

    if (destination < 0)
    {
        log_error("cp: cannot create destination\n");

        sys_close(source);

        return 1;
    }

    char buffer[4096];

    while (1)
    {
        long count = sys_read(
            source,
            buffer,
            sizeof(buffer)
        );

        if (count <= 0)
            break;

        long written = sys_write(
            destination,
            buffer,
            count
        );

        if (written < 0)
            break;
    }

    sys_close(source);
    sys_close(destination);

    return 0;
}

static int command_mv(
    int argc,
    char **argv
)
{
    if (argc < 3)
    {
        log_error("mv: missing operand\n");

        return 1;
    }

    long result = sys_rename(
        argv[1],
        argv[2]
    );

    if (result < 0)
    {
        log_error("mv: cannot move file\n");

        return 1;
    }

    return 0;
}

static int command_rmdir(
    int argc,
    char **argv
)
{
    if (argc < 2)
    {
        log_error("rmdir: missing operand\n");

        return 1;
    }

    long result = sys_rmdir(
        argv[1]
    );

    if (result < 0)
    {
        log_error("rmdir: cannot remove directory\n");

        return 1;
    }

    return 0;
}

static int command_env(
    int argc,
    char **argv
)
{
    (void)argc;
    (void)argv;

    for (long i = 0;
         environment[i] != 0;
         i++)
    {
        log_write(environment[i]);
        log_write("\n");
    }

    return 0;
}

static int command_export(
    int argc,
    char **argv
)
{
    for (int i = 1; i < argc; i++)
    {
        if (set_environment(argv[i]) < 0)
        {
            log_error("export: invalid variable\n");

            return 1;
        }
    }

    return 0;
}

static int command_unset(
    int argc,
    char **argv
)
{
    for (int i = 1; i < argc; i++)
    {
        for (int j = 0; j < environment_count; j++)
        {
            if (!environment_name_equals(
                    environment[j],
                    argv[i]))
            {
                continue;
            }

            for (int k = j;
                 k < environment_count - 1;
                 k++)
            {
                for (long l = 0; l < 256; l++)
                {
                    environment_storage[k][l] =
                        environment_storage[k + 1][l];
                }

                environment[k] =
                    environment_storage[k];
            }

            environment_count--;

            environment[environment_count] = 0;

            break;
        }
    }

    return 0;
}

static command commands[] =
{
    {
        "help",
        command_help,
        0,
        0
    },

    {
        "echo",
        command_echo,
        0,
        -1
    },

    {
        "ls",
        command_ls,
        0,
        1
    },

    {
        "cd",
        command_cd,
        1,
        1
    },

    {
        "pwd",
        command_pwd,
        0,
        0
    },

    {
        "cat",
        command_cat,
        0,
        1
    },

    {
        "mkdir",
        command_mkdir,
        1,
        1
    },

    {
        "touch",
        command_touch,
        1,
        1
    },

    {
        "rm",
        command_rm,
        1,
        1
    },

    {
        "cp",
        command_cp,
        2,
        2
    },

    {
        "mv",
        command_mv,
        2,
        2
    },

    {
        "rmdir",
        command_rmdir,
        1,
        1
    },

    {
        "env",
        command_env,
        0,
        0
    },

    {
        "export",
        command_export,
        1,
        -1
    },

    {
        "unset",
        command_unset,
        1,
        -1
    },

    {
        "exit",
        command_exit,
        0,
        0
    }
};

static int command_count =
    sizeof(commands) / sizeof(commands[0]);

enum command_separator
{
    COMMAND_SEPARATOR_NONE,
    COMMAND_SEPARATOR_ALWAYS,
    COMMAND_SEPARATOR_AND,
    COMMAND_SEPARATOR_OR,
    COMMAND_SEPARATOR_PIPE
};

static char *find_command_separator(
    char *input,
    enum command_separator *separator
)
{
    int in_quotes = 0;
    char quote = 0;

    *separator = COMMAND_SEPARATOR_NONE;

    while (*input != '\0')
    {
        if (in_quotes)
        {
            if (*input == quote)
            {
                in_quotes = 0;
            }

            input++;
            continue;
        }

        if (*input == '"' ||
            *input == '\'')
        {
            in_quotes = 1;
            quote = *input;
            input++;
            continue;
        }

        if (*input == '&' &&
            input[1] == '&')
        {
            *separator = COMMAND_SEPARATOR_AND;
            return input;
        }

        if (*input == '|' &&
            input[1] == '|')
        {
            *separator = COMMAND_SEPARATOR_OR;
            return input;
        }

        if (*input == '|')
        {
            *separator = COMMAND_SEPARATOR_PIPE;
            return input;
        }

        if (*input == ';')
        {
            *separator = COMMAND_SEPARATOR_ALWAYS;
            return input;
        }

        input++;
    }

    return 0;
}

static int parse_command(
    char *input,
    char **argv,
    int max_arguments
)
{
    int argc = 0;

    while (*input != '\0')
    {
        while (*input == ' ' ||
               *input == '\t' ||
               *input == '\n')
        {
            input++;
        }

        if (*input == '\0')
            break;

        if (argc >= max_arguments)
            break;

        argv[argc++] = input;

        int in_quotes = 0;
        char quote = 0;

        char *read = input;
        char *write = input;

        while (*read != '\0')
        {
            if (in_quotes)
            {
                *write = *read;

                if (*read == quote)
                {
                    in_quotes = 0;
                }

                write++;
                read++;

                continue;
            }

            if (*read == '"' ||
                *read == '\'')
            {
                in_quotes = 1;
                quote = *read;

                *write = *read;

                write++;
                read++;

                continue;
            }

            if (*read == ' ' ||
                *read == '\t' ||
                *read == '\n')
            {
                read++;
                break;
            }

            *write = *read;
            write++;
            read++;
        }

        *write = '\0';

        while (*read == ' ' ||
               *read == '\t' ||
               *read == '\n')
        {
            read++;
        }

        input = read;
    }

    argv[argc] = 0;

    return argc;
}

static long spawn_external(
    char **argv,
    int input,
    int output
)
{
    long pid = sys_fork();

    if (pid < 0)
    {
        return -1;
    }

    if (pid == 0)
    {
        if (input >= 0)
        {
            sys_dup2(
                input,
                0
            );
        }

        if (output >= 0)
        {
            sys_dup2(
                output,
                1
            );
        }

        sys_execve(
            argv[0],
            argv,
            environment
        );

        log_error("shell: exec failed\n");

        sys_exit(1);
    }

    return pid;
}

static int is_append_redirection(
    const char *operator
)
{
    return string_equals(
        operator,
        ">>"
    );
}

static void expand_variables(
    char *argument,
    char *output,
    long output_size
)
{
    long input_index = 0;
    long output_index = 0;

    char quote = 0;

    while (argument[input_index] != '\0' &&
           output_index < output_size - 1)
    {
        char current = argument[input_index];

        if (quote != 0)
        {
            if (current == quote)
            {
                quote = 0;
                input_index++;

                continue;
            }

            if (quote == '\'')
            {
                output[output_index++] = current;
                input_index++;

                continue;
            }
        }

        if (current == '"' ||
            current == '\'')
        {
            quote = current;
            input_index++;

            continue;
        }

        if (current != '$')
        {
            output[output_index++] = current;
            input_index++;

            continue;
        }

        input_index++;

        if (argument[input_index] == '?')
        {
            char status[32];
            int status_length = 0;

            int value = last_status;

            if (value == 0)
            {
                status[status_length++] = '0';
            }
            else
            {
                char reversed[32];
                int reversed_length = 0;

                if (value < 0)
                {
                    reversed[reversed_length++] = '-';
                    value = -value;
                }

                while (value > 0)
                {
                    reversed[reversed_length++] =
                        '0' + (value % 10);

                    value /= 10;
                }

                for (int i = reversed_length - 1;
                     i >= 0;
                     i--)
                {
                    status[status_length++] =
                        reversed[i];
                }
            }

            for (int i = 0;
                 i < status_length &&
                 output_index < output_size - 1;
                 i++)
            {
                output[output_index++] = status[i];
            }

            input_index++;

            continue;
        }

        char name[64];
        long name_length = 0;

        while (argument[input_index] != '\0' &&
               ((argument[input_index] >= 'a' &&
                 argument[input_index] <= 'z') ||
                (argument[input_index] >= 'A' &&
                 argument[input_index] <= 'Z') ||
                (argument[input_index] >= '0' &&
                 argument[input_index] <= '9') ||
                argument[input_index] == '_'))
        {
            if (name_length < 63)
            {
                name[name_length++] =
                    argument[input_index];
            }

            input_index++;
        }

        name[name_length] = '\0';

        for (int i = 0; i < environment_count; i++)
        {
            if (environment_name_equals(
                    environment[i],
                    name))
            {
                long value_start =
                    name_length + 1;

                long value_length =
                    string_length(environment[i]) -
                    value_start;

                for (long j = 0;
                     j < value_length &&
                     output_index < output_size - 1;
                     j++)
                {
                    output[output_index++] =
                        environment[i][value_start + j];
                }

                break;
            }
        }
    }

    output[output_index] = '\0';
}

static int find_redirection(
    int argc,
    char **argv
)
{
    for (int i = 1; i < argc; i++)
    {
        if (string_equals(argv[i], ">") ||
            string_equals(argv[i], ">>"))
            return i;
    }

    return -1;
}

static void print_prompt(void)
{
    char path[128];

    long count = sys_readlink(
        "/proc/self/cwd",
        path,
        sizeof(path) - 1
    );

    if (count < 0)
    {
        log_info("aethel> ");

        return;
    }

    log_info(path);
    log_info("> ");
}

static int execute_external(
    int argc,
    char **argv
)
{
    long pid = sys_fork();

    if (pid < 0)
    {
        log_error("shell: fork failed\n");

        return 1;
    }

    if (pid == 0)
    {
        sys_execve(
            argv[0],
            argv,
            environment
        );

        log_error("shell: exec failed\n");

        sys_exit(1);
    }

    int status;

    long result = sys_waitpid(
        pid,
        &status,
        0
    );

    if (result < 0)
    {
        return 1;
    }

    return (status >> 8) & 0xff;
}

static int contains_slash(
    const char *string
)
{
    for (long i = 0; string[i] != '\0'; i++)
    {
        if (string[i] == '/')
            return 1;
    }

    return 0;
}

static int find_executable(
    const char *name,
    char *path,
    long size,
    const char *path_value
)
{
    char paths[256];

    long length = string_length(path_value);

    if (length >= sizeof(paths))
        return 0;

    for (long i = 0; i < length; i++)
        paths[i] = path_value[i];

    paths[length] = '\0';

    char *current = paths;

    while (1)
    {
        char *directory = find_next_path(
            &current
        );

        if (directory == 0)
            break;

        long position = 0;

        for (long i = 0;
             directory[i] != '\0';
             i++)
        {
            if (position >= size - 1)
                return 0;

            path[position++] = directory[i];
        }

        if (position >= size - 1)
            return 0;

        path[position++] = '/';

        for (long i = 0;
             name[i] != '\0';
             i++)
        {
            if (position >= size - 1)
                return 0;

            path[position++] = name[i];
        }

        path[position] = '\0';

        long fd = sys_openat(
            -100,
            path,
            0,
            0
        );

        if (fd >= 0)
        {
            sys_close(fd);
            return 1;
        }
        return 1;
    }

    return 0;
}

static int execute_command(
    int argc,
    char **argv
)
{
    int redirection = find_redirection(
        argc,
        argv
    );

    int command_argc = argc;

    if (redirection >= 0)
    {
        if (redirection + 1 >= argc)
        {
            log_error("shell: missing output file\n");

            return 1;
        }

        command_argc = redirection;
    }

    int command_index = -1;

    for (int i = 0; i < command_count; i++)
    {
        if (string_equals(
            argv[0],
            commands[i].name
        ))
        {
            command_index = i;
            break;
        }
    }

    int external = 0;

    if (command_index < 0)
    {
        if (contains_slash(argv[0]))
        {
            external = 1;
        }
        else
        {
            char path[128];

            if (!find_executable(
                argv[0],
                path,
                sizeof(path),
                path_value
            ))
            {
                log_error("Command not found\n");

                return 1;
            }

            argv[0] = path;

            external = 1;
        }
    }

    if (!external)
    {
        int argument_count = command_argc - 1;

        if (
            argument_count < commands[command_index].min_arguments ||
            (
                commands[command_index].max_arguments >= 0 &&
                argument_count > commands[command_index].max_arguments
            )
        )
        {
            log_error("Invalid arguments\n");

            return 1;
        }
    }

    int output = -1;
    int saved_stdout = -1;

    if (redirection >= 0)
    {
        int flags = 1 | 64 | 512;

        if (is_append_redirection(
            argv[redirection]
        ))
        {
            flags = 1 | 64 | 1024;
        }

        output = sys_openat(
            -100,
            argv[redirection + 1],
            flags,
            0644
        );

        if (output < 0)
        {
            log_error("shell: cannot open output file\n");

            return 1;
        }

        saved_stdout = sys_dup(1);

        sys_dup2(
            output,
            1
        );

        argv[redirection] = 0;
    }

    int result = 0;

    if (external)
    {
        result = execute_external(
            command_argc,
            argv
        );
    }
    else
    {
        result = commands[command_index].function(
            command_argc,
            argv
        );
    }

    if (redirection >= 0)
    {
        sys_dup2(
            saved_stdout,
            1
        );

        sys_close(saved_stdout);
        sys_close(output);
    }

    return result;
}

static long spawn_command(
    int argc,
    char **argv,
    int input,
    int output,
    int pipe_read,
    int pipe_write
)
{
    long pid = sys_fork();

    if (pid < 0)
    {
        return -1;
    }

    if (pid == 0)
    {
        if (input >= 0)
        {
            sys_dup2(
                input,
                0
            );
        }

        if (output >= 0)
        {
            sys_dup2(
                output,
                1
            );
        }

        if (pipe_read >= 0)
        {
            sys_close(pipe_read);
        }

        if (pipe_write >= 0)
        {
            sys_close(pipe_write);
        }

        if (input >= 0 &&
            input != pipe_read)
        {
            sys_close(input);
        }

        if (output >= 0 &&
            output != pipe_write)
        {
            sys_close(output);
        }

        int status = execute_command(
            argc,
            argv
        );

        sys_exit(status);
    }

    return pid;
}

static int execute_pipeline(
    char *command,
    enum command_separator *separator,
    char **next_command
)
{
    *separator = COMMAND_SEPARATOR_NONE;
    *next_command = 0;

    char *argv[16];

    int input = -1;

    long pids[16];
    int pid_count = 0;

    *separator = COMMAND_SEPARATOR_NONE;

    while (command != 0 &&
           *command != '\0')
    {
        enum command_separator separator_type;

        char *current_separator =
            find_command_separator(
                command,
                &separator_type
            );

        if (current_separator != 0)
        {
            *current_separator = '\0';
        }

        int argc = parse_command(
            command,
            argv,
            16
        );

        if (argc == 0)
        {
            if (input >= 0)
            {
                sys_close(input);
            }

            return 1;
        }

        char expanded[16][256];

        for (int i = 0; i < argc; i++)
        {
            expand_variables(
                argv[i],
                expanded[i],
                sizeof(expanded[i])
            );

            argv[i] = expanded[i];
        }

        int output = -1;
        int pipefd[2];

        if (current_separator != 0 &&
            separator_type == COMMAND_SEPARATOR_PIPE)
        {
            if (sys_pipe(pipefd) < 0)
            {
                if (input >= 0)
                {
                    sys_close(input);
                }

                return 1;
            }

            output = pipefd[1];
        }

        long pid = spawn_command(
            argc,
            argv,
            input,
            output,
            output >= 0 ? pipefd[0] : -1,
            output >= 0 ? pipefd[1] : -1
        );

        if (pid < 0)
        {
            if (input >= 0)
            {
                sys_close(input);
            }

            if (output >= 0)
            {
                sys_close(pipefd[0]);
                sys_close(pipefd[1]);
            }

            return 1;
        }

        pids[pid_count++] = pid;

        if (input >= 0)
        {
            sys_close(input);
        }

        if (output >= 0)
        {
            sys_close(output);

            input = pipefd[0];
        }
        else
        {
            input = -1;
        }

        if (current_separator == 0)
        {
            break;
        }

        if (separator_type != COMMAND_SEPARATOR_PIPE)
        {
            *separator = separator_type;

            if (separator_type == COMMAND_SEPARATOR_AND ||
                separator_type == COMMAND_SEPARATOR_OR)
            {
                *next_command = current_separator + 2;
            }
            else
            {
                *next_command = current_separator + 1;
            }

            break;
        }

        command = current_separator + 1;
    }

    if (input >= 0)
    {
        sys_close(input);
    }

    int status = 0;

    for (int i = 0; i < pid_count; i++)
    {
        int current_status;

        sys_waitpid(
            pids[i],
            &current_status,
            0
        );

        if (i == pid_count - 1)
        {
            status =
                (current_status >> 8) & 0xff;
        }
    }

    return status;
}

int main(void)
{
    char buffer[128];
    char *argv[16];

    load_profile(
        profile,
        sizeof(profile)
    );

    long profile_length = string_length(profile);

    for (long i = 0; i <= profile_length; i++)
    {
        environment_storage[0][i] = profile[i];
    }

    environment[0] = environment_storage[0];
    environment_count = 1;

    path_value = get_path_value(profile);

    char *current = path_value;

    while (1)
    {
        print_prompt();

        long count = sys_read(
            0,
            buffer,
            sizeof(buffer) - 1
        );

        if (count <= 0)
            continue;

        buffer[count] = '\0';

        char *command = buffer;

        enum command_separator previous_separator =
            COMMAND_SEPARATOR_ALWAYS;

        while (command != 0 &&
            *command != '\0')
        {
            enum command_separator separator_type;

            char *separator =
                find_command_separator(
                    command,
                    &separator_type
                );

            if (separator != 0)
            {
                if (separator_type == COMMAND_SEPARATOR_PIPE)
                {
                    enum command_separator pipeline_separator;
                    char *next_command = 0;

                    last_status = execute_pipeline(
                        command,
                        &pipeline_separator,
                        &next_command
                    );

                    if (pipeline_separator == COMMAND_SEPARATOR_NONE)
                    {
                        break;
                    }

                    previous_separator = pipeline_separator;
                    command = next_command;

                    continue;
                }

                *separator = '\0';

                if (separator_type == COMMAND_SEPARATOR_AND)
                {
                    separator[1] = '\0';
                }
            }

            int argc = parse_command(
                command,
                argv,
                16
            );

            char expanded[16][256];

            for (int i = 0; i < argc; i++)
            {
                expand_variables(
                    argv[i],
                    expanded[i],
                    sizeof(expanded[i])
                );

                argv[i] = expanded[i];
            }

            if (argc > 0)
            {
                int execute = 1;

                if (previous_separator == COMMAND_SEPARATOR_AND &&
                    last_status != 0)
                {
                    execute = 0;
                }

                if (previous_separator == COMMAND_SEPARATOR_OR &&
                    last_status == 0)
                {
                    execute = 0;
                }

                if (execute)
                {
                    last_status = execute_command(
                        argc,
                        argv
                    );
                }
            }

            if (separator == 0)
            {
                break;
            }

            previous_separator = separator_type;

            if (separator_type == COMMAND_SEPARATOR_AND ||
                separator_type == COMMAND_SEPARATOR_OR)
            {
                command = separator + 2;
            }
            else
            {
                command = separator + 1;
            }
        }
    }

    return 0;
}