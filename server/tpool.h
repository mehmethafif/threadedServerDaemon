//
// Created by mehmet on 19.06.2020.
//

#ifndef SERVER_TPOOL_H
#define SERVER_TPOOL_H

#include <pthread.h>
#include "cache.h"

typedef struct Job {
    void (*function)(int, int); // The work function
    int args;
    //struct Job *next; // Link to next Job
} Job;

typedef struct ThreadList {
    pthread_t thread;
    struct ThreadList *next;
} ThreadList;


typedef struct ThreadPool {
    ThreadList * threads;
    ThreadList * rearThreads;
    int numThreads;
    int maxThreads;
    int removeThreads;
    volatile int waitingThreads;
    volatile int isInitialized;
    pthread_t resizer;
    pthread_mutex_t jobmutex;
    pthread_mutex_t condmutex;
    pthread_mutex_t addmutex;
    pthread_cond_t conditional;
    pthread_cond_t readycond;
    pthread_cond_t addcond;
    pthread_mutex_t readymutex;
    pthread_cond_t resizecond;
    pthread_mutex_t resizemutex;
    int run;
    int threadID;
    Job *job;
    pthread_mutex_t endmutex;
    pthread_cond_t endconditional;
    int suspend;
    int jobCount;
    FILE* log_fp;
} ThreadPool;

typedef int ThreadPoolStatus;

ThreadPool * createPool(int numThreads, int maxThreads, FILE* log_fp);

void waitToComplete(ThreadPool *);
void destroyPool(ThreadPool *);
ThreadPoolStatus addJobToPool(ThreadPool *, void (*func)(int, int), int);
ThreadPoolStatus addThreadsToPool(ThreadPool *, int);

void suspendPool(ThreadPool *);
void resumePool(ThreadPool *);
void removeThreadFromPool(ThreadPool *);
int getJobCount(ThreadPool *pool);
int getThreadCount(ThreadPool *);


#endif //SERVER_TPOOL_H
