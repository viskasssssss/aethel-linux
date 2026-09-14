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

static long syscall2(
    long number,
    long arg1,
    long arg2
)
{
    long result;

    asm volatile (
        "syscall"
        : "=a"(result)
        : "a"(number),
          "D"(arg1),
          "S"(arg2)
        : "rcx", "r11", "memory"
    );

    return result;
}

static long syscall3(
    long number,
    long arg1,
    long arg2,
    long arg3
)
{
    long result;

    asm volatile (
        "syscall"
        : "=a"(result)
        : "a"(number),
          "D"(arg1),
          "S"(arg2),
          "d"(arg3)
        : "rcx", "r11", "memory"
    );

    return result;
}

static long syscall4(
    long number,
    long arg1,
    long arg2,
    long arg3,
    long arg4
)
{
    long result;

    asm volatile (
        "mov %5, %%r10\n"
        "syscall"
        : "=a"(result)
        : "a"(number),
          "D"(arg1),
          "S"(arg2),
          "d"(arg3),
          "r"(arg4)
        : "rcx", "r11", "r10", "memory"
    );

    return result;
}

static long syscall5(
    long number,
    long arg1,
    long arg2,
    long arg3,
    long arg4,
    long arg5
)
{
    long result;

    register long r10 asm("r10") = arg4;
    register long r8 asm("r8") = arg5;

    asm volatile (
        "syscall"
        : "=a"(result)
        : "a"(number),
          "D"(arg1),
          "S"(arg2),
          "d"(arg3),
          "r"(r10),
          "r"(r8)
        : "rcx", "r11", "memory"
    );

    return result;
}

static long syscall0(long number)
{
    long result;

    asm volatile (
        "syscall"
        : "=a"(result)
        : "a"(number)
        : "rcx", "r11", "memory"
    );

    return result;
}

static long syscall1(
    long number,
    long arg1
)
{
    long result;

    asm volatile (
        "syscall"
        : "=a"(result)
        : "a"(number),
          "D"(arg1)
        : "rcx", "r11", "memory"
    );

    return result;
}

long sys_read(
    int fd,
    void *buffer,
    long size
)
{
    return syscall3(
        0,
        fd,
        (long)buffer,
        size
    );
}

long sys_write(
    int fd,
    const void *buffer,
    long size
)
{
    return syscall3(
        1,
        fd,
        (long)buffer,
        size
    );
}

long sys_fork(void)
{
    return syscall0(57);
}

long sys_execve(
    const char *path,
    char *const argv[],
    char *const envp[]
)
{
    return syscall3(
        59,
        (long)path,
        (long)argv,
        (long)envp
    );
}

long sys_waitpid(
    int pid,
    int *status,
    int options
)
{
    return syscall4(
        61,
        pid,
        (long)status,
        options,
        0
    );
}

long sys_openat(
    int dirfd,
    const char *path,
    int flags,
    int mode
)
{
    return syscall4(
        257,
        dirfd,
        (long)path,
        flags,
        mode
    );
}

long sys_close(
    int fd
)
{
    return syscall1(
        3,
        fd
    );
}

long sys_getdents64(
    int fd,
    void *buffer,
    long size
)
{
    return syscall3(
        217,
        fd,
        (long)buffer,
        size
    );
}

long sys_chdir(
    const char *path
)
{
    return syscall1(
        80,
        (long)path
    );
}

long sys_readlink(
    const char *path,
    char *buffer,
    long size
)
{
    return syscall3(
        89,
        (long)path,
        (long)buffer,
        size
    );
}

long sys_mount(
    const char *source,
    const char *target,
    const char *filesystem,
    unsigned long flags,
    const void *data
)
{
    return syscall5(
        165,
        (long)source,
        (long)target,
        (long)filesystem,
        flags,
        (long)data
    );
}

long sys_mkdir(
    const char *path,
    unsigned int mode
)
{
    return syscall2(
        83,
        (long)path,
        mode
    );
}

long sys_unlink(
    const char *path
)
{
    return syscall1(
        87,
        (long)path
    );
}

long sys_rename(
    const char *old_path,
    const char *new_path
)
{
    return syscall2(
        82,
        (long)old_path,
        (long)new_path
    );
}

long sys_rmdir(
    const char *path
)
{
    return syscall1(
        84,
        (long)path
    );
}

long sys_dup2(
    int old_fd,
    int new_fd
)
{
    return syscall2(
        33,
        old_fd,
        new_fd
    );
}

long sys_dup(
    int fd
)
{
    return syscall1(
        32,
        fd
    );
}

long sys_ioctl(
    int fd,
    unsigned long request,
    void *argument
)
{
    return syscall3(
        16,
        fd,
        request,
        (long)argument
    );
}

void sys_exit(int status)
{
    syscall1(60, status);

    while (1)
    {
    }
}

void sys_pause(void)
{
    syscall0(34);
}