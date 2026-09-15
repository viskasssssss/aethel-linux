#include "log.h"
#include "syscall.h"

int main(int argc, char **argv) {
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