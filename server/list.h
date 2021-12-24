//
// Created by mehmet on 24.06.2020.
//

#ifndef SERVER_LIST_H
#define SERVER_LIST_H

#include <stdio.h>

typedef struct ListObj* List;

List newList(void);
void freeList(List* pL);
int length(List L);
int front(List L);
void clear(List L);
void append(List L, int data);
void append_data2(List L, int data, int data2);
int get_data2(List L, int data);
void set_data2(List L, int data, int data2);
void deleteFront(List L);
void printList(FILE* out, List L);


#endif //SERVER_LIST_H
