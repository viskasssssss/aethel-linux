#include "log.h"
#include "syscall.h"

int main(int argc, char **argv) {
    const char *path = ".";

    if (argc > 1)
        path = argv[1];

    int fd = sys_openat(
        -100, 
        path, 
        0, 
        0
    );

    if (fd < 0)
    {
        log_error("ls: cannot open directory\n");

        return 1;
    }

    char buffer[4096];

    long count = sys_getdents64(
        fd, 
        buffer, 
        sizeof(buffer)
    );

    if (count < 0)
    {
        log_error("ls: cannot read directory\n");

        sys_close(fd);

        return 1;
    }

    long position = 0;

    while (position < count)
    {
        struct linux_dirent64 *entry =
            (struct linux_dirent64 *)
            (buffer + position);
        
        if (entry->name[0] == '.') {
            position += entry->record_length;
            continue;
        }

        if (entry->type == DT_DIR)
        {
            log_info(entry->name);
            log_write("/");
        }
        else
        {
            log_write(entry->name);
        }

        log_write("\n");

        position += entry->record_length;
    }

    sys_close(fd);

    return 0;
}