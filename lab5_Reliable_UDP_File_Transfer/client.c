#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>

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

/*
 * rdt3.0 sender with:
 * - packet drop simulation (20%)
 * - checksum corruption simulation (20%)
 * - timer using select() (1s)
 * - retry limit: max 3 timeouts/failed attempts
 */
static void clientSendRDT3(int sockfd,
                           const struct sockaddr *servAddr,
                           socklen_t servLen,
                           Packet *pkt) {

    // make socket non-blocking for select+recvfrom pattern
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    unsigned retries = 0;

    while (1) {
        if (retries >= 3) {
            printf("Client: retries>=3, giving up on SEQ %d\n", pkt->header.seq_ack);
            break;
        }

        // correct checksum first
        pkt->header.cksum = getChecksum(*pkt);

        // Simulate checksum corruption (20%)
        if (rand() % 5 == 0) {
            printf("Client: CORRUPTING checksum intentionally!\n");
            pkt->header.cksum = 0;
        }

        printf("Client sending SEQ=%d len=%d cksum=%d (try %u)\n",
               pkt->header.seq_ack, pkt->header.len, pkt->header.cksum, retries + 1);

        // Simulate packet loss (20%)
        if (rand() % 5 == 0) {
            printf("Client: DROPPING packet intentionally (not sending)\n");
        } else {
            if (sendto(sockfd, pkt, sizeof(*pkt), 0, servAddr, servLen) < 0) {
                perror("sendto");
            }
        }

        // wait for ACK with select
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int rv = select(sockfd + 1, &readfds, NULL, NULL, &tv);

        if (rv == 0) {
            printf("Client: TIMEOUT -> resend SEQ %d\n", pkt->header.seq_ack);
            retries++;
            continue;
        } else if (rv < 0) {
            perror("select");
            retries++;
            continue;
        } else {
            Packet ack;
            memset(&ack, 0, sizeof(ack));
            ssize_t n = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);

            if (n < 0) {
                printf("Client: recvfrom ACK failed -> resend\n");
                retries++;
                continue;
            }

            int calc = getChecksum(ack);

            printf("Client received ACK %d cksum=%d expected=%d\n",
                   ack.header.seq_ack, ack.header.cksum, calc);

            bool corrupted = (ack.header.cksum != calc);
            bool wrongack  = (ack.header.seq_ack != pkt->header.seq_ack);

            if (corrupted) {
                printf("Client: ACK corrupted -> resend\n");
                retries++;
                continue;
            }
            if (wrongack) {
                printf("Client: wrong ACK -> resend\n");
                retries++;
                continue;
            }

            printf("Client: GOOD ACK %d\n", ack.header.seq_ack);
            break; // success
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_ip> <port> <srcfile>\n", argv[0]);
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    const char *srcfile = argv[3];

    srand((unsigned)time(NULL));

    int fp = open(srcfile, O_RDONLY);
    if (fp < 0) {
        perror("open srcfile");
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        close(fp);
        return 1;
    }

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, ip, &servAddr.sin_addr) != 1) {
        fprintf(stderr, "Bad IP\n");
        close(fp);
        close(sockfd);
        return 1;
    }

    printf("Client rdt3.0 sending file %s to %s:%d\n", srcfile, ip, port);

    int seq = 0;

    while (1) {
        Packet pkt;
        memset(&pkt, 0, sizeof(pkt));

        ssize_t bytes = read(fp, pkt.data, sizeof(pkt.data));
        if (bytes < 0) { perror("read"); break; }

        pkt.header.seq_ack = seq;
        pkt.header.len = (int)bytes;
        pkt.header.cksum = getChecksum(pkt);

        clientSendRDT3(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr), &pkt);

        if (bytes == 0) {
            printf("Client: Final packet handled. Done.\n");
            break;
        }

        seq = (seq + 1) % 2;
    }

    close(fp);
    close(sockfd);
    return 0;
}