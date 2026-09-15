#include "log.h"
#include "syscall.h"

int main(int argc, char **argv) {
    if (argc < 2)
    {
        log_error("mkdir: missing operand\n");

        return 1;
    }

    long result = sys_mkdir(
        argv[1], 
        0755
    );

    if (result < 0)
    {
        log_error("mkdir: cannot create directory\n");

        return 1;
    }

    return 0;
}