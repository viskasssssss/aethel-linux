#include "log.h"
#include "syscall.h"

int main(int argc, char **argv) {
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