#include <fcntl.h>      // open()
#include <unistd.h>     // write(), read(), close()
#include <cstdio>       // printf
#include <cstring>      // strlen

int main() {
    const char* filename = "test.db";
    const char* message = "Hello, disk!";

    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        printf("Failed to open file for writing\n");
        return 1;
    }

    ssize_t bytes_written = write(fd, message, strlen(message));
    printf("Wrote %ld bytes\n", bytes_written);
    close(fd);

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        printf("Failed to open file for reading\n");
        return 1;
    }

    char buffer[256] = {0};
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    printf("Read %ld bytes: %s\n", bytes_read, buffer);

    close(fd);
    return 0;
}
