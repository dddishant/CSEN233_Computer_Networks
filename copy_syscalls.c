#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <src> <dest>\n", argv[0]);
        return 1;
    }

    const char *src = argv[1];
    const char *dest = argv[2];

    int fd_src = open(src, O_RDONLY);
    if (fd_src < 0) {
        perror("open src");
        return 1;
    }

    int fd_dest = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_dest < 0) {
        perror("open dest");
        close(fd_src);
        return 1;
    }

    char buffer[BUF_SIZE];
    ssize_t nread;

    clock_t start = clock();

    while ((nread = read(fd_src, buffer, BUF_SIZE)) > 0) {
        ssize_t total_written = 0;
        while (total_written < nread) {
            ssize_t nw = write(fd_dest, buffer + total_written, (size_t)(nread - total_written));
            if (nw < 0) {
                perror("write");
                close(fd_src);
                close(fd_dest);
                return 1;
            }
            total_written += nw;
        }
    }

    if (nread < 0) {
        perror("read");
        close(fd_src);
        close(fd_dest);
        return 1;
    }

    clock_t end = clock();
    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    close(fd_src);
    close(fd_dest);

    printf("Copied (syscalls) %s -> %s\n", src, dest);
    printf("CPU time (syscalls): %.6f seconds\n", cpu_time_used);

    return 0;
}

