#include <unistd.h>

int main() {
    const char msg[] = "Hello from x86-32 Haiku!\n";
    write(1, msg, sizeof(msg) - 1);
    return 0;
}
