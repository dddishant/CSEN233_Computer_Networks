#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>

#define MAXN 64
#define INFINITE 1000
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

typedef struct routers {
    char name[50];
    char ip[50];
    int port;
} ROUTERS;

ROUTERS routers[MAXN];
int costs[MAXN][MAXN];
int distances[MAXN];

int myid, nodes;
int sockfd;

struct sockaddr_in addr;
pthread_mutex_t lock;

void print_costs(void) {
    pthread_mutex_lock(&lock);
    printf("\n--- Cost Table ---\n");
    for (int i = 0; i < nodes; i++) {
        for (int j = 0; j < nodes; j++) {
            printf("%4d ", costs[i][j]);
        }
        printf("\n");
    }
    printf("------------------\n");
    pthread_mutex_unlock(&lock);
}

void *receive_info(void *arg) {
    int packet[3];
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);

    while (1) {
        ssize_t n = recvfrom(sockfd, packet, sizeof(packet), 0,
                             (struct sockaddr *)&sender, &sender_len);

        if (n == sizeof(packet)) {
            int x = ntohl(packet[0]);
            int y = ntohl(packet[1]);
            int c = ntohl(packet[2]);

            pthread_mutex_lock(&lock);
            costs[x][y] = c;
            costs[y][x] = c;
            pthread_mutex_unlock(&lock);

            printf("Received update: (%d <-> %d) cost=%d\n", x, y, c);
        }
    }
    return NULL;
}

void dijkstra() {
    int taken[MAXN];

    pthread_mutex_lock(&lock);
    for (int i = 0; i < nodes; i++) {
        distances[i] = costs[myid][i];
        taken[i] = 0;
    }
    pthread_mutex_unlock(&lock);

    distances[myid] = 0;
    taken[myid] = 1;

    for (int i = 1; i < nodes; i++) {
        int min = INFINITE;
        int spot = -1;

        for (int j = 0; j < nodes; j++) {
            if (!taken[j] && distances[j] < min) {
                min = distances[j];
                spot = j;
            }
        }

        if (spot == -1) break;

        taken[spot] = 1;

        for (int j = 0; j < nodes; j++) {
            if (!taken[j]) {
                pthread_mutex_lock(&lock);
                int c = costs[spot][j];
                pthread_mutex_unlock(&lock);

                if (c < INFINITE && distances[spot] + c < distances[j]) {
                    distances[j] = distances[spot] + c;
                }
            }
        }
    }
}

void *run_link_state(void *arg) {
    while (1) {
        int r = 10 + rand() % 11;
        sleep(r);

        dijkstra();

        printf("\nRouter %d - Least Costs:\n", myid);
        for (int i = 0; i < nodes; i++) {
            if (distances[i] >= INFINITE)
                printf("INF ");
            else
                printf("%d ", distances[i]);
        }
        printf("\n");
    }
    return NULL;
}

void send_update(int x, int y, int cost) {
    int packet[3];
    packet[0] = htonl(x);
    packet[1] = htonl(y);
    packet[2] = htonl(cost);

    for (int j = 0; j < nodes; j++) {
        if (j == myid) continue;

        struct sockaddr_in otheraddr;
        memset(&otheraddr, 0, sizeof(otheraddr));
        otheraddr.sin_family = AF_INET;
        otheraddr.sin_port = htons(routers[j].port);
        inet_pton(AF_INET, routers[j].ip, &otheraddr.sin_addr);

        sendto(sockfd, packet, sizeof(packet), 0,
               (struct sockaddr *)&otheraddr, sizeof(otheraddr));
    }
}

int main(int argc, char *argv[]) {
    FILE *fp;
    pthread_t recv_thread, ls_thread;

    if (argc != 5) {
        printf("Usage: %s <myid> <N> <routers.txt> <costs.txt>\n", argv[0]);
        return 1;
    }

    myid = atoi(argv[1]);
    nodes = atoi(argv[2]);

    srand(time(NULL) + myid);

    fp = fopen(argv[3], "r");
    for (int i = 0; i < nodes; i++) {
        fscanf(fp, "%s %s %d", routers[i].name,
               routers[i].ip, &routers[i].port);
    }
    fclose(fp);

    fp = fopen(argv[4], "r");
    for (int i = 0; i < nodes; i++) {
        for (int j = 0; j < nodes; j++) {
            fscanf(fp, "%d", &costs[i][j]);
        }
    }
    fclose(fp);

    pthread_mutex_init(&lock, NULL);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(routers[myid].port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));

    pthread_create(&recv_thread, NULL, receive_info, NULL);
    pthread_create(&ls_thread, NULL, run_link_state, NULL);

    for (int i = 0; i < 2; i++) {
        sleep(10);
        int id, cost;
        printf("Enter change (neighbor cost): ");
        scanf("%d %d", &id, &cost);

        pthread_mutex_lock(&lock);
        costs[myid][id] = cost;
        costs[id][myid] = cost;
        pthread_mutex_unlock(&lock);

        print_costs();
        send_update(myid, id, cost);
    }

    sleep(30);
    return 0;
}
