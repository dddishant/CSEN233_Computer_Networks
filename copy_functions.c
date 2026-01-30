#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <src> <dest>\n", argv[0]);
        return 1;
    }

    const char *src = argv[1];
    const char *dest = argv[2];

    FILE *in = fopen(src, "rb");
    if (!in) {
        perror("fopen src");
        return 1;
    }

    FILE *out = fopen(dest, "wb");
    if (!out) {
        perror("fopen dest");
        fclose(in);
        return 1;
    }

    char buffer[BUF_SIZE];
    size_t n;

    clock_t start = clock();

    while ((n = fread(buffer, 1, BUF_SIZE, in)) > 0) {
        if (fwrite(buffer, 1, n, out) != n) {
            perror("fwrite");
            fclose(in);
            fclose(out);
            return 1;
        }
    }

    if (ferror(in)) {
        perror("fread");
        fclose(in);
        fclose(out);
        return 1;
    }

    clock_t end = clock();
    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    fclose(in);
    fclose(out);

    printf("Copied (functions) %s -> %s\n", src, dest);
    printf("CPU time (functions): %.6f seconds\n", cpu_time_used);

    return 0;
}

