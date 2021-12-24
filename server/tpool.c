//
// Created by mehmet on 19.06.2020.
//

#include "tpool.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


//function every thread executes
static void *threadExecutor(void *pl){
    time_t ltime;
    struct tm result;
    char stime[32];
    ThreadPool *pool = (ThreadPool *)pl;
    pthread_mutex_lock(&pool->jobmutex); // Lock the mutex
    ++pool->threadID; //Get id
    int id = pool->threadID;
    pthread_mutex_unlock(&pool->jobmutex); // Release the mutex

    //Start core loop
    while(pool->run){
        pthread_mutex_lock(&pool->jobmutex);
        Job *presentJob = pool->job; // Get job
        pool->job = NULL;
        pthread_cond_signal(&pool->addcond);
        if(presentJob==NULL || pool->suspend){
            pthread_mutex_unlock(&pool->jobmutex);
            pthread_mutex_lock(&pool->condmutex);
            pool->waitingThreads++;
            if(!pool->suspend && pool->waitingThreads==pool->numThreads){ // All threads idle
                if(pool->isInitialized){
                    pthread_mutex_lock(&pool->endmutex);
                    pthread_cond_signal(&pool->endconditional);
                    pthread_mutex_unlock(&pool->endmutex);
                }
                else
                    pool->isInitialized = 1;
            }
            ltime = time(NULL);
            localtime_r(&ltime, &result);
            fprintf(pool->log_fp, "\n%.24s Thread #%d: waiting for connection", asctime_r(&result, stime), id);
            fflush(pool->log_fp);
            pthread_cond_wait(&pool->conditional, &pool->condmutex);
            //wake up
            if(pool->waitingThreads>0) //not anymore waiting
                pool->waitingThreads--;
            if ((float)(pool->numThreads - pool->waitingThreads) > (float)(pool->numThreads/4)*3.0){
                //signal resizer
                pthread_cond_signal(&pool->resizecond);
            }
            //signal for main
            pthread_cond_signal(&pool->readycond);
            pthread_mutex_unlock(&pool->condmutex); //woke up
        }
        else { //job in pool
            ltime = time(NULL);
            localtime_r(&ltime, &result);
            fprintf(pool->log_fp, "\n%.24s A connection has been delegated to thread id #%d, system load %.1f%%",
                    asctime_r(&result, stime), id,
                    (float) ((float) (pool->numThreads - pool->waitingThreads) / (float) pool->numThreads) * 100);
            fflush(pool->log_fp);
            pthread_mutex_unlock(&pool->jobmutex);
            presentJob->function(presentJob->args, id); //run job
            free(presentJob);
        }
    }

    if(pool->run){ //force
        pool->removeThreads--;
        pthread_mutex_unlock(&pool->jobmutex);

    }
    //pthread_exit((void *)NULL);
    return NULL;
}

//resizer function
static void *threadResizer(void *pl){
    time_t ltime;
    struct tm result;
    char stime[32];
    ThreadPool *pool = (ThreadPool *)pl; // Get the pool
    while (pool->numThreads < pool->maxThreads && pool->run == 1){
        while ((float)(pool->numThreads - pool->waitingThreads) < (float)(pool->numThreads/4)*3){
            pthread_cond_wait(&pool->resizecond, &pool->resizemutex);
        }
        if (pool->run == 1 && pool->isInitialized == 1) {
            if (pool->numThreads + ((pool->numThreads + 3) / 4) <= pool->maxThreads) {
                ltime = time(NULL);
                localtime_r(&ltime, &result);
                fprintf(pool->log_fp, "\n%.24s System load %.1f%%, pool extended to %d threads", asctime_r(&result, stime),
                        (float) ((float) (pool->numThreads - pool->waitingThreads) / (float) pool->numThreads) * 100,
                        pool->numThreads + (pool->numThreads + 3) / 4);
                fflush(pool->log_fp);
                addThreadsToPool(pool, (pool->numThreads + 3) / 4);
            } else {
                addThreadsToPool(pool, (pool->maxThreads - pool->numThreads));
                ltime = time(NULL);
                localtime_r(&ltime, &result);
                fprintf(pool->log_fp, "\n%.24s System load %.1f%%, pool extended to %d threads", asctime_r(&result, stime),
                        (float) ((float) (pool->numThreads - pool->waitingThreads) / (float) pool->numThreads) * 100, pool->maxThreads);
                fflush(pool->log_fp);
            }
        }
    }
    //pthread_exit((void *)NULL);
    return NULL;
}

//add thread to the pool
ThreadPoolStatus addThreadsToPool(ThreadPool *pool, int threads){
    if(!pool->run){
        //pool stopped
        return -1;
    }
    if(threads < 1){
        //invalid no
        return -1;
    }
    int temp = 0;
    ThreadPoolStatus rc = 0;

    pthread_mutex_lock(&pool->condmutex);
    pool->numThreads += threads; // add to thread count
    pthread_mutex_unlock(&pool->condmutex);

    int i = 0;
    for(i = 0;i < threads;i++){

        ThreadList *newThread = (ThreadList *)malloc(sizeof(ThreadList));
        newThread->next = NULL;
        temp = pthread_create(&newThread->thread, NULL, threadExecutor, (void *)pool);
        if(temp){
            pthread_mutex_lock(&pool->condmutex);
            pool->numThreads--;
            pthread_mutex_unlock(&pool->condmutex);
            temp = 0;
            rc = -1;
        }
        else{
            if(pool->rearThreads==NULL)
                pool->threads = pool->rearThreads = newThread;
            else
                pool->rearThreads->next = newThread;
            pool->rearThreads = newThread;
        }
    }
    return rc;
}

//create new pool with given number of thread
ThreadPool * createPool(int numThreads, int maxThreads, FILE* log_fp){
    time_t ltime;
    struct tm result;
    char stime[32];
    ThreadPool * pool = (ThreadPool *)malloc(sizeof(ThreadPool)); // Allocate memory for the pool
    if(pool==NULL){
        //unable to allocate
        return NULL;
    }

    pool->log_fp = log_fp;
    pool->numThreads = 0;
    pool->maxThreads = maxThreads;
    pool->waitingThreads = 0;
    pool->isInitialized = 0;
    pool->removeThreads = 0;
    pool->suspend = 0;
    pool->rearThreads = NULL;
    pool->threads = NULL;
    pool->jobCount = 0;
    pool->threadID = 0;
    pool->job = NULL;

    pthread_mutex_init(&pool->jobmutex, NULL); // Initialize mutexes
    pthread_mutex_init(&pool->condmutex, NULL);
    pthread_mutex_init(&pool->endmutex, NULL);
    pthread_mutex_init(&pool->readymutex, NULL);
    pthread_mutex_init(&pool->resizemutex, NULL);
    pthread_mutex_init(&pool->addmutex, NULL);

    pthread_cond_init(&pool->endconditional, NULL); // Initialize conditionals
    pthread_cond_init(&pool->conditional, NULL);
    pthread_cond_init(&pool->readycond, NULL);
    pthread_cond_init(&pool->resizecond, NULL);
    pthread_cond_init(&pool->addcond, NULL);

    pool->run = 1; // Start the pool
    addThreadsToPool(pool, numThreads);
    ltime = time(NULL);
    localtime_r(&ltime, &result);
    fprintf(pool->log_fp, "\n%.24s A pool of %d threads has been created", asctime_r(&result, stime), pool->numThreads);

    //create resizer
    pthread_create(&pool->resizer, NULL, threadResizer, (void *)pool);
    return pool;
}

//add new job to the pool
ThreadPoolStatus addJobToPool(ThreadPool *pool, void (*func)(int args, int id), int args){
    if(pool==NULL || !pool->isInitialized){
        //not initialized
        return -1;
    }
    if(!pool->run){
        //stopped pool
        return -2;
    }
    if(pool->run==2){
        //waiting to complete
        return -3;
    }

    Job *newJob = (Job *)malloc(sizeof(Job));
    if(newJob==NULL){
        //unable to allocate
        return -1;
    }
    newJob->function = func;
    newJob->args = args;
    while(pool->job != NULL){
        pthread_cond_wait(&pool->addcond, &pool->addmutex);
    }
    pool->job = newJob;
    pthread_cond_signal(&pool->readycond);

    if(pool->waitingThreads>0){ // wake threads
        pthread_mutex_lock(&pool->condmutex);
        pthread_cond_signal(&pool->conditional); //signal to wake
        pthread_mutex_unlock(&pool->condmutex);
    }
    pthread_mutex_unlock(&pool->jobmutex);
    return 0;
}

//wait threads to finish their job
void waitToComplete(ThreadPool *pool){
    if(pool==NULL || !pool->isInitialized){
        //not initialized
        return;
    }
    if(!pool->run){
        //stopped
        return;
    }
    pool->run = 2;
    pthread_mutex_lock(&pool->condmutex);
    if(pool->numThreads==pool->waitingThreads){
        pthread_mutex_unlock(&pool->condmutex);
        pool->run = 1;
        return;
    }
    pthread_mutex_unlock(&pool->condmutex);
    pthread_mutex_lock(&pool->endmutex);
    pthread_cond_wait(&pool->endconditional, &pool->endmutex);
    pthread_mutex_unlock(&pool->endmutex);
    pool->run = 1;
}

//destroy and free the pool
void destroyPool(ThreadPool *pool){
    if(pool==NULL || !pool->isInitialized){
        //not initialized
        return;
    }

    pool->run = 0; //stop flag set
    pthread_mutex_lock(&pool->condmutex);
    pthread_cond_broadcast(&pool->conditional); //wake up all threads
    pthread_mutex_unlock(&pool->condmutex);
    ThreadList *list = pool->threads, *backup = NULL;
    int i = 0;
    while(list != NULL){
        pthread_join(list->thread, NULL); //join them
        backup = list;
        list = list->next;
        free(backup);
        i++;
    }
    pthread_join(pool->resizer, NULL);
    pthread_cond_destroy(&pool->conditional);
    pthread_cond_destroy(&pool->endconditional);
    pthread_cond_destroy(&pool->resizecond);
    pthread_cond_destroy(&pool->readycond);
    pthread_cond_destroy(&pool->addcond);

    pthread_mutex_destroy(&pool->jobmutex);
    pthread_mutex_destroy(&pool->condmutex);
    pthread_mutex_destroy(&pool->endmutex);
    pthread_mutex_destroy(&pool->resizemutex);
    pthread_mutex_destroy(&pool->readymutex);
    pthread_mutex_destroy(&pool->addmutex);

    free(pool);
}
