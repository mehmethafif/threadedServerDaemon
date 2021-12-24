//
// Created by mehmet on 22.06.2020.
//

#include "cache.h"
#include <stdlib.h>
#include <stdio.h>

Cache newCache(void) { // Returns reference to new empty List object.
    Cache C = malloc(sizeof(CacheObj));
    C->head = NULL;
    C->AR = 0;
    C->AW = 0;
    C->WR = 0;
    C->WW = 0;
    pthread_mutex_init(&C->mutex, NULL);
    pthread_cond_init(&C->okToWrite, NULL);
    pthread_cond_init(&C->okToRead, NULL);
    return (C);
}

CacheNode newCacheNode(int data, int data2, List path){ // Returns reference to new Node object. Initializes next and data fields.
    CacheNode N = malloc(sizeof(CacheNodeObj));
    N->data = data;
    N->data2 = data2;
    N->path = path;
    return(N);
}

void freeCacheNode(CacheNode* pN){ // Frees heap memory pointed to by *pN, sets *pN to NULL.
    if( pN!=NULL && *pN!=NULL ){
        free(*pN);
        *pN = NULL;
    }
}

void freeCache(Cache* pC) { // Frees all heap memory associated with List *pL, and sets *pL to NULL.
    if(pC != NULL && *pC != NULL) {
        CacheNode temp = (*pC)->head;
        while(temp != NULL) {
            CacheNode N = temp;
            temp = temp->next;
            freeList(&N->path);
            free(N);
        }
        free(*pC);
        *pC = NULL;
    }
}

void appendCache(Cache C, int data, int data2, List path){
    if(C == NULL) {
        exit(1);
    }
    pthread_mutex_lock(&C->mutex);
    while((C->AW + C->AR) > 0){
        C->WW += 1;
        pthread_cond_wait(&C->okToWrite, &C->mutex);
        C->WW -= 1;
    }
    C->AW += 1;
    pthread_mutex_unlock(&C->mutex);
    //add to cache head
    CacheNode temp = newCacheNode(data, data2, path);
    temp->next = C->head;
    C->head = temp;
    //adding finished
    pthread_mutex_lock(&C->mutex);
    C->AW -= 1;
    if(C->WW > 0){
        pthread_cond_signal(&C->okToWrite);
    } else if (C->WR > 0){
        pthread_cond_broadcast(&C->okToWrite);
    }
    pthread_mutex_unlock(&C->mutex);

}

List getCachePath(Cache C, int data, int data2){
    pthread_mutex_lock(&C->mutex);
    while((C->AW + C->WW) > 0){
        C->WR += 1;
        pthread_cond_wait(&C->okToRead, &C->mutex);
        C->WR -= 1;
    }
    C->AR += 1;
    pthread_mutex_unlock(&C->mutex);
    CacheNode node = C->head;
    List path = NULL;
    while (node != NULL){
        if(node->data == data && node->data2 == data2){
            path = node->path;
            break;
        }
        node = node->next;
    }
    pthread_mutex_lock(&C->mutex);
    C->AR -= 1;
    if (C->AR == 0 && C->WW > 0){
        pthread_cond_signal(&C->okToWrite);
    }
    pthread_mutex_unlock(&C->mutex);
    return path;
}
