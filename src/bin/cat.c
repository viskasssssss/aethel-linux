#include "log.h"
#include "syscall.h"

int main(int argc, char **argv) {
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