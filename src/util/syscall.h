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

#define CLOCK_REALTIME 0
#define DT_DIR 4

struct linux_dirent64
{
    unsigned long inode;
    long offset;

    unsigned short record_length;
    unsigned char type;

    char name[];
};

struct utsname
{
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

struct timespec
{
    long seconds;
    long nanoseconds;
};

struct stat
{
    unsigned long dev;
    unsigned long inode;
    unsigned long nlink;

    unsigned int mode;
    unsigned int uid;
    unsigned int gid;

    unsigned long rdev;
    long size;
    long block_size;
    long blocks;

    long atime;
    long atime_nsec;

    long mtime;
    long mtime_nsec;

    long ctime;
    long ctime_nsec;

    long unused[3];
};

long sys_read(
    int fd,
    void *buffer,
    long size
);

long sys_write(
    int fd,
    const void *buffer,
    long size
);

long sys_fork(void);

long sys_execve(
    const char *path,
    char *const argv[],
    char *const envp[]
);

long sys_waitpid(
    int pid,
    int *status,
    int options
);

long sys_openat(
    int dirfd,
    const char *path,
    int flags,
    int mode
);

long sys_close(
    int fd
);

long sys_getdents64(
    int fd,
    void *buffer,
    long size
);

long sys_chdir(
    const char *path
);

long sys_readlink(
    const char *path,
    char *buffer,
    long size
);

long sys_mount(
    const char *source,
    const char *target,
    const char *filesystem,
    unsigned long flags,
    const void *data
);

long sys_mkdir(
    const char *path,
    unsigned int mode
);

long sys_unlink(
    const char *path
);

long sys_rename(
    const char *old_path,
    const char *new_path
);

long sys_rmdir(
    const char *path
);

long sys_dup2(
    int old_fd,
    int new_fd
);

long sys_dup(
    int fd
);

long sys_ioctl(
    int fd,
    unsigned long request,
    void *argument
);

long sys_uname(struct utsname *buffer);

long sys_sethostname(const char *name, long length);

long sys_nanosleep(
    const struct timespec *request,
    struct timespec *remaining
);

long sys_clock_gettime(
    int clock_id,
    struct timespec *time
);

long sys_newfstatat(
    int dirfd,
    const char *path,
    struct stat *buffer,
    int flags
);

long sys_pipe(int pipefd[2]);

long sys_kill(int pid, int signal);

void sys_exit(int status);

void sys_pause(void);