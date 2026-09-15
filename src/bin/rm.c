#include "log.h"
#include "syscall.h"

int main(int argc, char **argv) {
    if (argc < 2)
    {
        log_error("rm: missing operand\n");

        return 1;
    }

    long result = sys_unlink(
        argv[1]
    );

    if (result < 0)
    {
        log_error("rm: cannot remove file\n");

        return 1;
    }

    return 0;
}