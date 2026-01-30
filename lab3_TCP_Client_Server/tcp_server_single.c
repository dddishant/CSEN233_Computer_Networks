#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

int recv_all(int sock, void *buf, int n) {
    int total = 0;
    while (total < n) {
        int r = recv(sock, (char*)buf + total, n - total, 0);
        if (r <= 0) return r;
        total += r;
    }
    return total;
}

long long get_file_size(FILE *fp) {
    fseek(fp, 0, SEEK_END);
    long long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return sz;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 0;
    }

    int port = atoi(argv[1]);

    // 1) Create TCP socket
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2) Bind server address
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(listenfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind");
        exit(1);
    }

    // 3) Listen
    if (listen(listenfd, 5) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Server listening on port %d...\n", port);

    while (1) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);

        // 4) Accept client
        int connfd = accept(listenfd, (struct sockaddr*)&clientAddr, &clientLen);
        if (connfd < 0) { perror("accept"); continue; }

        printf("Connection Established with client IP: %s and Port: %d\n",
               inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

        // 5) Receive filename (length + name)
        int name_len_net;
        if (recv_all(connfd, &name_len_net, sizeof(name_len_net)) <= 0) {
            close(connfd);
            continue;
        }

        int name_len = ntohl(name_len_net);
        if (name_len <= 0 || name_len > 1000) {
            close(connfd);
            continue;
        }

        char filename[1024];
        memset(filename, 0, sizeof(filename));

        if (recv_all(connfd, filename, name_len) <= 0) {
            close(connfd);
            continue;
        }

        filename[name_len] = '\0';
        printf("Client requested file: %s\n", filename);

        // 6) Open file
        FILE *fp = fopen(filename, "rb");
        if (!fp) {
            int status = htonl(1); // not found
            send(connfd, &status, sizeof(status), 0);
            printf("File not found.\n");
            close(connfd);
            continue;
        }

        // 7) Send status OK
        int status = htonl(0);
        send(connfd, &status, sizeof(status), 0);

        // 8) Send file size
        long long file_size = get_file_size(fp);
        send(connfd, &file_size, sizeof(file_size), 0);

        // 9) Send file data
        char buffer[BUF_SIZE];
        long long sent = 0;

        while (sent < file_size) {
            int r = fread(buffer, 1, BUF_SIZE, fp);
            if (r <= 0) break;
            send(connfd, buffer, r, 0);
            sent += r;
        }

        fclose(fp);
        close(connfd);

        printf("File transfer complete.\n");
    }

    close(listenfd);
    return 0;
}

