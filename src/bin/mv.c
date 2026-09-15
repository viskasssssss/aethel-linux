#include "log.h"
#include "syscall.h"

int main(int argc, char **argv) {
    if (argc < 3)
    {
        log_error("mv: missing operand\n");

        return 1;
    }

    long result = sys_rename(
        argv[1], 
        argv[2]
    );

    if (result < 0)
    {
        log_error("mv: cannot move file\n");

        return 1;
    }

    return 0;
}