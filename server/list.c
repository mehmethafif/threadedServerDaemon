//
// Created by mehmet on 20.06.2020.
//


#include <stdio.h>
#include <stdlib.h>
#include "list.h"

typedef struct NodeObj{
    int data;
    int data2;
    struct NodeObj* next;
    struct NodeObj* prev;
} NodeObj;

typedef NodeObj* Node;

typedef struct ListObj{
    Node front;
    Node back;
    Node cursor;
    int index;
    int length;
} ListObj;


Node newNode(int data){ // Returns reference to new Node object. Initializes next and data fields.
    Node N = malloc(sizeof(NodeObj));
    N->data = data;
    N->next = NULL;
    N->prev = NULL;
    return(N);
}

List newList(void) { // Returns reference to new empty List object.
    List L;
    L = malloc(sizeof(ListObj));
    L->front = L->back = L->cursor = NULL;
    L->length = 0;
    L->index = -1;
    return (L);
}


void freeNode(Node* pN){ // Frees heap memory pointed to by *pN, sets *pN to NULL.
    if( pN!=NULL && *pN!=NULL ){
        free(*pN);
        *pN = NULL;
    }
}

void freeList(List* pL) { // Frees all heap memory associated with List *pL, and sets *pL to NULL.
    if(pL != NULL && *pL != NULL) {
        Node temp = (*pL)->front;
        while(temp != NULL) {
            Node N = temp;
            temp = temp->next;
            free(N);
        }
        free(*pL);
        *pL = NULL;
    }
}


int length(List L) {
    // Returns the number of elements in this List.
    if(L == NULL) {
        printf("List Error: calling \"length()\" on NULL List reference\n");
        exit(1);
    }
    return(L->length);
}

int front(List L) {
    // Returns front element.
    if(L == NULL) {
        printf("List Error: calling \"front()\" on NULL List reference\n");
        exit(1);
    }
    if(L->length < 1) {
        printf("List Error: calling \"front()\" on empty List reference\n");
        exit(1);
    }
    return(L->front->data);
}

void clear(List L) {
    // Resets this List to its original empty state.
    if(L != NULL) {
        Node temp = L->front;
        while(temp != NULL) {
            Node N = temp;
            temp = temp->next;
            free(N);
        }
        L->front = L->back = L->cursor = NULL;
        L->length = 0;
        L->index = -1;
    }
}


void append(List L, int data) {
    // Insert new element into this List. If List is non-empty,
    // insertion takes place after back element.
    if(L == NULL) {
        printf("List Error: calling \"append()\" on NULL List reference\n");
        exit(1);
    }
    Node temp = newNode(data);
    if(L->length < 1) { // If empty List
        L->front = L->back = temp;
    }
    else { // If non empty List
        L->back->next = temp;
        temp->prev = L->back;
        L->back = L->back->next;
    }
    L->length++;
}

void append_data2(List L, int data, int data2){
    if(L == NULL) {
        printf("List Error: calling \"append()\" on NULL List reference\n");
        exit(1);
    }
    Node temp = newNode(data);
    temp->data2 = data2;
    if(L->length < 1) { // If empty List
        L->front = L->back = temp;
    }
    else { // If non empty List
        L->back->next = temp;
        temp->prev = L->back;
        L->back = L->back->next;
    }
    L->length++;
}

int get_data2(List L, int data){
    Node node = L->front;
    while (node != NULL){
        if(node->data == data){
            return node->data2;
        }
        node = node->next;
    }
    return 0;
}

void set_data2(List L, int data, int data2){
    Node node = L->front;
    while (node != NULL){
        if(node->data == data){
            node->data2 = data2;
        }
        node = node->next;
    }
}


void deleteFront(List L) {
    // Deletes the front element.
    if(L == NULL) {
        printf("List Error: calling \"deleteFront()\" on NULL List reference\n");
        exit(1);
    }
    if(L->length < 1) {
        printf("List Error: calling \"deleteFront()\" on empty List reference\n");
        exit(1);
    }
    if(L->length == 1) { // If length 1, head and tail are same
        clear(L);
    }
    else {
        Node temp = L->front;
        L->front = L->front->next;
        L->front->prev = NULL;
        freeNode(&temp);
        L->length--;

        if(L->index == 0) { // If cursor is at front, becomes undefined
            L->cursor = NULL;
            L->index = -1;
        }
        else if(L->index != -1) { // If cursor is defined, index is decremented
            L->index--;
        }
    }
}

void printList(FILE* out, List L) {
    if(length(L) != 0) { // Prints list if non-empty

        Node N = L->front;
        for( ; N != L->back; N = N->next){
            fprintf(out, "%d->", N->data);
        }
        if(N == L->back) {
            fprintf(out, "%d", N->data);
        }
    }
}

