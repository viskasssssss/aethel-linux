#include "log.h"
#include "syscall.h"

int main(int argc, char **argv) {
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