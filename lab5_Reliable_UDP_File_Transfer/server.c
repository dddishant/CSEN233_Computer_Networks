#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>

#define PLOSTMSG 5   // 1/5 = 20% loss probability

typedef struct {
    int seq_ack;
    int len;
    int cksum;
} Header;

typedef struct {
    Header header;
    char data[10];
} Packet;

static int getChecksum(Packet packet) {
    packet.header.cksum = 0;
    int checksum = 0;
    unsigned char *ptr = (unsigned char *)&packet;
    unsigned char *end = ptr + sizeof(Header) + packet.header.len;
    while (ptr < end) checksum ^= *ptr++;
    return checksum;
}

static void printPacket(Packet packet) {
    printf("Packet{ header:{ seq_ack:%d len:%d cksum:%d } data:\"",
           packet.header.seq_ack, packet.header.len, packet.header.cksum);
    fwrite(packet.data, (size_t)packet.header.len, 1, stdout);
    printf("\" }\n");
}

static void serverSendACK(int sockfd,
                          const struct sockaddr *clientAddr,
                          socklen_t clientLen,
                          int acknum) {
    // Simulate ACK loss (20%)
    if (rand() % PLOSTMSG == 0) {
        printf("Server: DROPPING ACK %d intentionally\n", acknum);
        return;
    }

    Packet ack;
    memset(&ack, 0, sizeof(ack));
    ack.header.seq_ack = acknum;
    ack.header.len = 0;
    ack.header.cksum = getChecksum(ack);

    printf("Server: Sending ACK %d (cksum=%d)\n", ack.header.seq_ack, ack.header.cksum);

    if (sendto(sockfd, &ack, sizeof(ack), 0, clientAddr, clientLen) < 0) {
        perror("sendto(ACK)");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <outfile>\n", argv[0]);
        return 1;
    }

    srand((unsigned)time(NULL));

    int port = atoi(argv[1]);
    const char *outFile = argv[2];

    int fp = open(outFile, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (fp < 0) { perror("open outfile"); return 1; }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket"); close(fp); return 1; }

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons((uint16_t)port);

    if (bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
        perror("bind");
        close(fp);
        close(sockfd);
        return 1;
    }

    printf("Server writing to %s, listening on UDP port %d...\n", outFile, port);

    int expected_seq = 0;
    int last_ack = 1; // if expected=0, last_ack should be 1

    while (1) {
        Packet pkt;
        memset(&pkt, 0, sizeof(pkt));

        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);

        ssize_t n = recvfrom(sockfd, &pkt, sizeof(pkt), 0,
                             (struct sockaddr *)&clientAddr, &clientLen);
        if (n < 0) { perror("recvfrom"); continue; }

        // Optional: simulate dropping the received DATA packet (20%)
        if (rand() % PLOSTMSG == 0) {
            printf("\nServer: DROPPING received DATA packet intentionally (not processing)\n");
            continue;
        }

        printf("\nServer received %zd bytes from %s:%d\n",
               n, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        printPacket(pkt);

        int calc = getChecksum(pkt);

        bool corrupted = (pkt.header.cksum != calc);
        bool wrongseq  = (pkt.header.seq_ack != expected_seq);

        if (corrupted) {
            printf("Server: BAD checksum (got %d expected %d). Resend last ACK %d\n",
                   pkt.header.cksum, calc, last_ack);
            serverSendACK(sockfd, (struct sockaddr *)&clientAddr, clientLen, last_ack);
            continue;
        }

        if (wrongseq) {
            printf("Server: DUP/Wrong SEQ (got %d expected %d). Resend last ACK %d\n",
                   pkt.header.seq_ack, expected_seq, last_ack);
            serverSendACK(sockfd, (struct sockaddr *)&clientAddr, clientLen, last_ack);
            continue;
        }

        printf("Server: GOOD packet SEQ %d\n", pkt.header.seq_ack);

        // final packet
        if (pkt.header.len == 0) {
            serverSendACK(sockfd, (struct sockaddr *)&clientAddr, clientLen, expected_seq);
            printf("Server: FINAL received. Closing.\n");
            break;
        }

        if (write(fp, pkt.data, (size_t)pkt.header.len) < 0) {
            perror("write");
            serverSendACK(sockfd, (struct sockaddr *)&clientAddr, clientLen, expected_seq);
            break;
        }

        // ACK this packet
        serverSendACK(sockfd, (struct sockaddr *)&clientAddr, clientLen, expected_seq);

        // move state forward
        last_ack = expected_seq;
        expected_seq = (expected_seq + 1) % 2;
    }

    close(fp);
    close(sockfd);
    return 0;
}