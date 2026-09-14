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

#define TCGETS 0x5401
#define TCSETS 0x5402

#define ICANON 0x00000002
#define ECHO   0x00000008

#define VMIN 6
#define VTIME 5

static char profile[256];
static char *path_value;

static char *environment[32] = {
    profile,
    0
};

static int environment_count = 1;

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
        "  exit\n";

    sys_write(
        1,
        message,
        sizeof(message) - 1
    );

    return 0;
}

static int command_echo(
    int argc,
    char **argv
)
{
    for (int i = 1; i < argc; i++)
    {
        sys_write(
            1,
            argv[i],
            string_length(argv[i])
        );

        if (i + 1 < argc)
            sys_write(1, " ", 1);
    }

    sys_write(1, "\n", 1);

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
        sys_write(
            1,
            "ls: cannot open directory\n",
            27
        );

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
        sys_write(
            1,
            "ls: cannot read directory\n",
            27
        );

        sys_close(fd);

        return 1;
    }

    long position = 0;

    while (position < count)
    {
        struct linux_dirent64 *entry =
            (struct linux_dirent64 *)
            (buffer + position);

        sys_write(
            1,
            entry->name,
            string_length(entry->name)
        );

        sys_write(
            1,
            "\n",
            1
        );

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
        sys_write(
            1,
            "cd: missing argument\n",
            21
        );

        return 1;
    }

    long result = sys_chdir(argv[1]);

    if (result < 0)
    {
        sys_write(
            1,
            "cd: cannot change directory\n",
            29
        );

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
        sys_write(
            1,
            "pwd: cannot get current directory\n",
            34
        );

        return 1;
    }

    sys_write(
        1,
        buffer,
        count
    );

    sys_write(
        1,
        "\n",
        1
    );

    return 0;
}

static int command_cat(
    int argc,
    char **argv
)
{
    if (argc < 2)
    {
        sys_write(
            1,
            "cat: missing operand\n",
            21
        );

        return 1;
    }

    int fd = sys_openat(
        -100,
        argv[1],
        0,
        0
    );

    if (fd < 0)
    {
        sys_write(
            1,
            "cat: cannot open file\n",
            23
        );

        return 1;
    }

    char buffer[4096];

    while (1)
    {
        long count = sys_read(
            fd,
            buffer,
            sizeof(buffer)
        );

        if (count <= 0)
            break;

        sys_write(
            1,
            buffer,
            count
        );
    }

    sys_close(fd);

    return 0;
}

static int command_mkdir(
    int argc,
    char **argv
)
{
    if (argc < 2)
    {
        sys_write(
            1,
            "mkdir: missing operand\n",
            23
        );

        return 1;
    }

    long result = sys_mkdir(
        argv[1],
        0755
    );

    if (result < 0)
    {
        sys_write(
            1,
            "mkdir: cannot create directory\n",
            32
        );

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
        sys_write(
            1,
            "touch: missing operand\n",
            23
        );

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
        sys_write(
            1,
            "touch: cannot create file\n",
            27
        );

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
        sys_write(
            1,
            "rm: missing operand\n",
            21
        );

        return 1;
    }

    long result = sys_unlink(
        argv[1]
    );

    if (result < 0)
    {
        sys_write(
            1,
            "rm: cannot remove file\n",
            24
        );

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
        sys_write(
            1,
            "cp: missing operand\n",
            21
        );

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
        sys_write(
            1,
            "cp: cannot open source\n",
            24
        );

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
        sys_write(
            1,
            "cp: cannot create destination\n",
            31
        );

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
        sys_write(
            1,
            "mv: missing operand\n",
            21
        );

        return 1;
    }

    long result = sys_rename(
        argv[1],
        argv[2]
    );

    if (result < 0)
    {
        sys_write(
            1,
            "mv: cannot move file\n",
            22
        );

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
        sys_write(
            1,
            "rmdir: missing operand\n",
            23
        );

        return 1;
    }

    long result = sys_rmdir(
        argv[1]
    );

    if (result < 0)
    {
        sys_write(
            1,
            "rmdir: cannot remove directory\n",
            32
        );

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
        sys_write(
            1,
            environment[i],
            string_length(environment[i])
        );

        sys_write(
            1,
            "\n",
            1
        );
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
        1,
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
        "exit",
        command_exit,
        0,
        0
    }
};

static int command_count =
    sizeof(commands) / sizeof(commands[0]);



static int parse_command(
    char *buffer,
    char **argv,
    int max_arguments
)
{
    int argc = 0;
    int inside_argument = 0;

    for (long i = 0; buffer[i] != '\0'; i++)
    {
        if (buffer[i] == ' ' ||
            buffer[i] == '\n' ||
            buffer[i] == '\t')
        {
            buffer[i] = '\0';
            inside_argument = 0;
            continue;
        }

        if (!inside_argument)
        {
            if (argc >= max_arguments)
                break;

            argv[argc] = &buffer[i];
            argc++;

            inside_argument = 1;
        }
    }

    return argc;
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
        sys_write(
            1,
            "aethel> ",
            8
        );

        return;
    }

    sys_write(
        1,
        path,
        count
    );

    sys_write(
        1,
        ">",
        1
    );
}

static int execute_external(
    int argc,
    char **argv
)
{
    long pid = sys_fork();

    if (pid < 0)
    {
        sys_write(
            1,
            "shell: fork failed\n",
            20
        );

        return 1;
    }

    if (pid == 0)
    {
        sys_execve(
            argv[0],
            argv,
            environment
        );

        sys_write(
            1,
            "shell: exec failed\n",
            20
        );

        sys_exit(1);
    }

    int status;

    sys_waitpid(
        pid,
        &status,
        0
    );

    return 0;
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
            sys_write(
                1,
                "shell: missing output file\n",
                27
            );

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
                sys_write(
                    1,
                    "Command not found\n",
                    19
                );

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
            sys_write(
                1,
                "Invalid arguments\n",
                19
            );

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
            sys_write(
                1,
                "shell: cannot open output file\n",
                31
            );

            return 1;
        }

        saved_stdout = sys_dup(1);

        sys_dup2(
            output,
            1
        );
    }

    int result = 0;

    if (external)
    {
        result = execute_external(
            argc,
            argv
        );
    }
    else
    {
        result = commands[command_index].function(
            argc,
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

int main(void)
{
    char buffer[128];
    char *argv[16];

    load_profile(
        profile,
        sizeof(profile)
    );

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

        int argc = parse_command(
            buffer,
            argv,
            16
        );

        if (argc == 0)
            continue;

        execute_command(
            argc,
            argv
        );
    }

    return 0;
}