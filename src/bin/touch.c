#include "log.h"
#include "syscall.h"

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++)
    {
        int fd = sys_openat(
            -100, 
            argv[i], 
            64, 
            0644
        );

        if (fd < 0)
        {
            return 1;
        }

        sys_close(fd);
    }

    return 0;
}