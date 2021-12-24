#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "tpool.h"
#include "graph.h"
#include "cache.h"
#include "list.h"
#include <semaphore.h>
#include <fcntl.h>

#define SEM_NAME "mehmetHafif171044042"

void int_handler();
void perror_exit(char *s);
void show_parameters(char* input_file, int port, char* log_file, int start_thread, int max_thread);
void job_fun(int client_fd, int id);


FILE *fp;
FILE *log_fp;
graphptr graph;
Cache cache;
ThreadPool *pool;
sem_t *sem;

int main(int argc, char *argv[]) {
    time_t ltime;
    struct tm result;
    char stime[32];
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = int_handler;
    sigaction(SIGINT, &act, NULL);
    int opt;
    char *file_path;
    char *log_file_path;
    int port;
    int startup_thread_count;
    int max_thread_count;
    clock_t t;
    t = clock();

    if (argc != 11) {
        printf("Usage: ./server -i pathToFile -p PORT -o pathToLogFile -s 4 -x 24\n");
        exit(EXIT_FAILURE);
    }

    while ((opt = getopt(argc, argv, "i:p:o:s:x:")) != -1) {
        switch (opt) {
            case 'i':
                file_path = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'o':
                log_file_path = optarg;
                break;
            case 's':
                startup_thread_count = atoi(optarg);
                break;
            case 'x':
                max_thread_count = atoi(optarg);
                break;
            case ':':
                printf("Usage: ./server -i pathToFile -p PORT -o pathToLogFile -s 4 -x 24\n");
                exit(EXIT_FAILURE);
                break;
            case '?':
                printf("unknown option: %c\n", optopt);
                printf("Usage: ./server -i pathToFile -p PORT -o pathToLogFile -s 4 -x 24\n");
                exit(EXIT_FAILURE);
                break;
            default:
                printf("Usage: ./server -i pathToFile -p PORT -o pathToLogFile -s 4 -x 24\n");
                exit(EXIT_FAILURE);
        }
    }

    sem = sem_open(SEM_NAME, O_CREAT, S_IRWXU, 1);
    if(sem == SEM_FAILED){
        perror_exit("Only one instance can run");
    }
    /*
    int singleton;
    singleton = sem_trywait(sem);
    if(singleton == 0){
        printf("Obtained lock !!!\n");
    }else{
        printf("Lock not obtained\n");
        exit(-1);
    }
     */

    if (startup_thread_count < 2){
        perror_exit("Startup thread count cant be smaller than 2\n");
    }

    fp = fopen(file_path, "r");
    if (fp == NULL) {
        perror_exit("graph file can not be opened\n");
    }

    log_fp = fopen(log_file_path, "w");
    if (fp == NULL) {
        perror_exit("log file can not be opened\n");
    }

    pid_t pid;
    if ((pid = fork()) < 0){
        perror_exit("fork error");
    } else if (pid != 0){ /* parent */
        exit(0);
    }
    setsid();
    if ((pid = fork()) < 0){
        perror_exit("fork error");
    } else if (pid != 0){ /* parent */
        exit(0);
    }
    fclose(stdout);
    fclose(stdin);
    fclose(stderr);

    show_parameters(file_path, port, log_file_path, startup_thread_count, max_thread_count);
    ltime = time(NULL);
    localtime_r(&ltime, &result);
    fprintf(log_fp, "%.24s Loading graph…\n", asctime_r(&result, stime));
    fflush(log_fp);


    graph =  createGraph();
    cache = newCache();

    char buffer[255];
    int ni, nj;
    while (fgets(buffer, 254, fp) != NULL){
        if (buffer[0] != '#'){
            sscanf(buffer,"%d\t%d", &ni, &nj);
            insertEdge(graph, ni, nj, 1);
        }
    }
    t = clock() - t;
    fclose(fp);
    ltime = time(NULL);
    localtime_r(&ltime, &result);
    fprintf(log_fp, "%.24s Graph loaded in %.1f seconds with %d nodes and %d edges.",asctime_r(&result, stime), ((double)t)/CLOCKS_PER_SEC, graph->node_count, graph->edge_count);
    fflush(log_fp);

    pool = createPool(startup_thread_count, max_thread_count, log_fp);

    int s;
    int s_accept;
    struct sockaddr_in sa;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }
    bzero(&sa, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_port   = htons(port);
    if (INADDR_ANY)
        sa.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (struct sockaddr *)&sa, sizeof sa) < 0) {
        perror("bind");
        return 2;
    }

    listen(s, max_thread_count);

    while (1){
        socklen_t size = sizeof sa;
        if ((s_accept = accept(s, (struct sockaddr *)&sa, &size)) < 0) {
            perror("accept");
            return 4;
        }

        while (pool->waitingThreads == 0){
            ltime = time(NULL);
            localtime_r(&ltime, &result);
            fprintf(log_fp, "\n%.24s No thread is available! Waiting for one.", asctime_r(&result, stime));
            fflush(log_fp);
            pthread_cond_wait(&pool->readycond, &pool->readymutex);
        }
        addJobToPool(pool, &job_fun, s_accept);
        fflush(log_fp);
    }
}

void int_handler() {
    time_t ltime;
    struct tm result;
    char stime[32];
    sem_close(sem);
    sem_unlink(SEM_NAME);
    ltime = time(NULL);
    localtime_r(&ltime, &result);
    fprintf(log_fp, "\n%.24s Termination signal received, waiting for ongoing threads to complete.", asctime_r(&result, stime));
    fflush(log_fp);
    waitToComplete(pool);
    destroyPool(pool);
    freeCache(&cache);
    exitProgram(graph);
    ltime = time(NULL);
    localtime_r(&ltime, &result);
    fprintf(log_fp, "\n%.24s All threads have terminated, server shutting down.", asctime_r(&result, stime));
    fflush(log_fp);
    fclose(log_fp);
    exit(0);
}

void perror_exit(char *s){
    perror(s);
    exit(-1);
}

void show_parameters(char* input_file, int port, char* log_file, int start_thread, int max_thread){
    time_t ltime;
    struct tm result;
    char stime[32];
    ltime = time(NULL);
    localtime_r(&ltime, &result);
    fprintf(log_fp, "%.24s Executing with parameters:\n", asctime_r(&result, stime));
    fprintf(log_fp, "%.24s -i %s\n", asctime_r(&result, stime), input_file);
    fprintf(log_fp, "%.24s -p %d\n", asctime_r(&result, stime), port);
    fprintf(log_fp, "%.24s -o %s\n", asctime_r(&result, stime), log_file);
    fprintf(log_fp, "%.24s -s %d\n",asctime_r(&result, stime) , start_thread);
    fprintf(log_fp, "%.24s -x %d\n", asctime_r(&result, stime), max_thread);
    fflush(log_fp);
}

void job_fun(int client_fd, int id){
    int source_node, dest_node, first;
    time_t ltime;
    struct tm result;
    char stime[32];
    //printf("\n!working thread! : cliend fd: %d\n", client_fd);
    FILE *client;
    if ((client = fdopen(client_fd, "r+")) == NULL) {
        perror("fdopen");
    }

    fread(&source_node, sizeof(int), 1, client);
    fread(&dest_node, sizeof(int), 1, client);
    ltime = time(NULL);
    localtime_r(&ltime, &result);
    fprintf(log_fp, "\n%.24s Thread #%d: searching database for a path from node %d to node %d", asctime_r(&result, stime), id, source_node, dest_node);
    fflush(log_fp);
    List path = getCachePath(cache, source_node, dest_node);
    if (path == NULL){
        ltime = time(NULL);
        localtime_r(&ltime, &result);
        fprintf(log_fp, "\n%.24s Thread #%d: no path in database, calculating %d->%d", asctime_r(&result, stime), id, source_node, dest_node);
        fflush(log_fp);
        path = BFS_path(graph, source_node, dest_node);
        first = front(path);
        if (first == -1){
            ltime = time(NULL);
            localtime_r(&ltime, &result);
            fprintf(log_fp, "\n%.24s Thread #%d: path not possible from node %d to %d", asctime_r(&result, stime), id, source_node, dest_node);
            fflush(log_fp);
        } else{
            ltime = time(NULL);
            localtime_r(&ltime, &result);
            fprintf(log_fp, "\n%.24s Thread #%d: path calculated: ", asctime_r(&result, stime), id);
            fflush(log_fp);
            printList(log_fp, path);
        }
        ltime = time(NULL);
        localtime_r(&ltime, &result);
        fprintf(log_fp, "\n%.24s Thread #%d: responding to client and adding path to database", asctime_r(&result, stime), id);
        fflush(log_fp);
        appendCache(cache, source_node, dest_node, path);
    } else{
        first = front(path);
        if (first == -1){
            ltime = time(NULL);
            localtime_r(&ltime, &result);
            fprintf(log_fp, "\n%.24s Thread #%d: path not possible from node %d to %d", asctime_r(&result, stime), id, source_node, dest_node);
            fflush(log_fp);
        } else{
            ltime = time(NULL);
            localtime_r(&ltime, &result);
            fprintf(log_fp, "\n%.24s Thread #%d: path found in database: ", asctime_r(&result, stime), id);
            fflush(log_fp);
            printList(log_fp, path);
        }
    }

    printList(client, path);
    fclose(client);
}