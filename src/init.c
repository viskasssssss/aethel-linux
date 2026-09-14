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

int main(void)
{
    long result = sys_mount(
        "proc",
        "/proc",
        "proc",
        0,
        0
    );

    if (result < 0)
    {
        const char message[] =
            "init: failed to mount /proc\n";

        sys_write(
            1,
            message,
            sizeof(message) - 1
        );
    }

    const char message[] = "Hello from Aethel Linux!\n";

    sys_write(
        1,
        message,
        sizeof(message) - 1
    );

    while (1)
    {
        long pid = sys_fork();

        if (pid == 0)
        {
            const char *argv[] = {
                "/shell",
                0
            };

            const char *envp[] = {
                0
            };

            sys_execve(
                "/shell",
                (char *const *)argv,
                (char *const *)envp
            );

            sys_exit(1);
        }

        if (pid > 0)
        {
            int status;

            sys_waitpid(
                pid,
                &status,
                0
            );

            continue;
        }

        sys_pause();
    }

    return 0;
}