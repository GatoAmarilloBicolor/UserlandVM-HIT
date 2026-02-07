#include <unistd.h>

int main() {
    write(1, "Hello from C!\n", 14);
    return 42;
}