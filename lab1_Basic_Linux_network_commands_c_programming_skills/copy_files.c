#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 4096

void copy_file(const char *src, const char *dest) {
    int fd_src, fd_dest;
    ssize_t bytes_read;
    char buffer[BUFFER_SIZE];

    fd_src = open(src, O_RDONLY);
    if (fd_src < 0) {
        perror("Error opening source file");
        exit(1);
    }

    fd_dest = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_dest < 0) {
        perror("Error opening destination file");
        close(fd_src);
        exit(1);
    }

    while ((bytes_read = read(fd_src, buffer, BUFFER_SIZE)) > 0) {
        write(fd_dest, buffer, bytes_read);
    }

    close(fd_src);
    close(fd_dest);
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc % 2 == 0) {
        printf("Usage: %s src1 dest1 [src2 dest2 ...]\n", argv[0]);
        exit(1);
    }

    for (int i = 1; i < argc; i += 2) {
        printf("Copying %s -> %s\n", argv[i], argv[i + 1]);
        copy_file(argv[i], argv[i + 1]);
    }

    printf("File copy completed successfully.\n");
    return 0;
}

