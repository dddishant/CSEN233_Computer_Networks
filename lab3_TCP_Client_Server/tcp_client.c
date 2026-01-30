#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

// helper: receive exactly n bytes
int recv_all(int sock, void *buf, int n) {
    int total = 0;
    while (total < n) {
        int r = recv(sock, (char*)buf + total, n - total, 0);
        if (r <= 0) return r;
        total += r;
    }
    return total;
}

int main(int argc, char *argv[]) {
    // Usage: ./tcp_client <server_ip> <port> <requested_filename> <save_as>
    if (argc != 5) {
        printf("Usage: %s <server_ip> <port> <requested_filename> <save_as>\n", argv[0]);
        return 0;
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    char *requested_file = argv[3];
    char *save_as = argv[4];

    // 1) Create TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    // 2) Setup server address
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &serverAddr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

    // 3) Connect to server
    if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Connected to server %s:%d\n", server_ip, port);

    // 4) Send requested filename (send length + data)
    int name_len = (int)strlen(requested_file);
    int name_len_net = htonl(name_len);

    send(sockfd, &name_len_net, sizeof(name_len_net), 0);
    send(sockfd, requested_file, name_len, 0);

    printf("Requested file: %s\n", requested_file);

    // 5) Receive status (0 = OK, 1 = NOT FOUND)
    int status_net;
    if (recv_all(sockfd, &status_net, sizeof(status_net)) <= 0) {
        printf("Server disconnected.\n");
        close(sockfd);
        return 0;
    }
    int status = ntohl(status_net);

    if (status != 0) {
        printf("Server says file not found.\n");
        close(sockfd);
        return 0;
    }

    // 6) Receive file size
    long long file_size_net;
    if (recv_all(sockfd, &file_size_net, sizeof(file_size_net)) <= 0) {
        printf("Failed to receive file size.\n");
        close(sockfd);
        return 0;
    }

    long long file_size;
    memcpy(&file_size, &file_size_net, sizeof(file_size)); // same machine ok
    printf("Receiving file of size: %lld bytes\n", file_size);

    // 7) Receive file data and write to output file
    FILE *fp = fopen(save_as, "wb");
    if (!fp) { perror("fopen"); close(sockfd); exit(1); }

    char buffer[BUF_SIZE];
    long long received = 0;

    while (received < file_size) {
        int to_read = BUF_SIZE;
        if (file_size - received < BUF_SIZE)
            to_read = (int)(file_size - received);

        int r = recv(sockfd, buffer, to_read, 0);
        if (r <= 0) break;

        fwrite(buffer, 1, r, fp);
        received += r;
    }

    fclose(fp);
    close(sockfd);

    printf("File transfer complete. Saved as: %s\n", save_as);
    return 0;
}

