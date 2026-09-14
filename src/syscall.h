#pragma once

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

void sys_exit(int status);

void sys_pause(void);