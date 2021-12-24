#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <getopt.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

void perror_exit(char *s);
void int_handler();


int main(int argc, char *argv[]) {
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = int_handler;
    sigaction(SIGINT, &act, NULL);
    int opt;
    char *ip_addr;
    int port;
    int source_node;
    int dest_node;
    struct timespec begin, end;


    if (argc != 9) {
        printf("Usage: ./client -a 127.0.0.1 -p PORT -s 768 -d 979\n");
        exit(EXIT_FAILURE);
    }

    while ((opt = getopt(argc, argv, "a:p:o:s:d:")) != -1) {
        switch (opt) {
            case 'a':
                ip_addr = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 's':
                source_node = atoi(optarg);
                break;
            case 'd':
                dest_node = atoi(optarg);
                break;
            case ':':
                printf("Usage: ./client -a 127.0.0.1 -p PORT -s 768 -d 979\n");
                exit(EXIT_FAILURE);
                break;
            case '?':
                printf("unknown option: %c\n", optopt);
                printf("Usage: ./client -a 127.0.0.1 -p PORT -s 768 -d 979\n");
                exit(EXIT_FAILURE);
                break;
            default:
                printf("Usage: ./client -a 127.0.0.1 -p PORT -s 768 -d 979\n");
                exit(EXIT_FAILURE);
        }
    }

    int s;
    int bytes;
    struct sockaddr_in sa;
    char buffer[BUFSIZ+1];

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }
    bzero(&sa, sizeof sa);

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = inet_addr(ip_addr);
    printf("\nClient (%d) connecting to %s:%d", getpid(), ip_addr, port);
    clock_gettime(CLOCK_REALTIME, &begin);
    if (connect(s, (struct sockaddr *)&sa, sizeof sa) < 0) {
        perror("connect");
        close(s);
        return 2;
    }

    printf("\nClient (%d) connected and requesting a path from node %d to %d", getpid(), source_node, dest_node);

    write(s, &source_node, sizeof(int ));
    write(s, &dest_node, sizeof(int ));

    while ((bytes = read(s, buffer, BUFSIZ)) > 0);

    clock_gettime(CLOCK_REALTIME, &end);

    double time = (double)(end.tv_nsec - begin.tv_nsec) / 1000000000.0 + (double)(end.tv_sec  - begin.tv_sec);

    if(buffer[0] == '-'){
        printf("\nServer’s response to (%d): NO PATH, arrived in %.4f seconds, shutting down", getpid(), time);
    } else {
        printf("\nServer’s response to (%d): %s, arrived in %.4f seconds, shutting down", getpid(), buffer, time);
    }

    close(s);
    return 0;


}

void int_handler() {
    printf("\nInterrupt received exiting...\n");
    exit(0);
}

void perror_exit(char *s){
    perror(s);
    exit(-1);
}