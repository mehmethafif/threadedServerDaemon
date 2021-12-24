//
// Created by mehmet on 22.06.2020.
//

#ifndef SERVER_CACHE_H
#define SERVER_CACHE_H

#include <pthread.h>
#include "list.h"

typedef struct CacheNodeObj{
    int data;
    int data2;
    List path;
    struct CacheNodeObj* next;
} CacheNodeObj;

typedef struct CacheNodeObj* CacheNode;

typedef struct CacheObj{
    CacheNode head;
    pthread_mutex_t mutex;
    pthread_cond_t okToRead;
    pthread_cond_t okToWrite;
    int AR;
    int AW;
    int WR;
    int WW;
} CacheObj;

typedef struct CacheObj* Cache;

Cache newCache(void);
CacheNode newCacheNode(int data, int data2, List path);
void freeCacheNode(CacheNode* pN);
void freeCache(Cache* pC);
void appendCache(Cache C, int data, int data2, List path);
List getCachePath(Cache C, int data, int data2);

#endif //SERVER_CACHE_H
