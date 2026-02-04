#include <sys/types.h>
#include <unistd.h>

/* Minimal static hello world for x86 32-bit */

/* Syscall for write() - Haiku syscall number for write */
static ssize_t haiku_write(int fd, const void *buf, size_t count)
{
    ssize_t ret;
    __asm__ __volatile__(
        "int $0x25"  /* Haiku syscall */
        : "=a" (ret)
        : "a" (4), "b" (fd), "c" (buf), "d" (count)
        : "memory"
    );
    return ret;
}

/* Syscall for exit() */
static void haiku_exit(int status)
{
    __asm__ __volatile__(
        "int $0x25"
        :
        : "a" (1), "b" (status)
    );
    while(1);
}

int main(void)
{
    const char msg[] = "Hello from static x86 Haiku binary!\n";
    haiku_write(1, msg, sizeof(msg) - 1);
    haiku_exit(0);
    return 0;
}
