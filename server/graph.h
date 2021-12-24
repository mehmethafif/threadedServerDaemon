//
// Created by mehmet on 20.06.2020.
//

#ifndef SERVER_GRAPH_H
#define SERVER_GRAPH_H

#include <stdbool.h>
#include <bits/types/FILE.h>
#include "list.h"

typedef int Tag;

typedef int edgeWeight;

typedef struct Graph *graphptr;

typedef struct Node{
    Tag name;
    int visited;  //I use this variable when searching for cycles
    struct Edge *edges; //the list of edges spawning from this Node
    struct Node *next;
} Node;

typedef struct Edge{
    Node *dest;
    int visited;
    struct Edge *next;
    edgeWeight weight;
} Edge;

typedef struct Graph{
    int node_count;
    int edge_count;
    Node *nodes;  //a list with all the nodes in the graph
} Graph;

graphptr createGraph();
void insertNode(Tag, graphptr);
void insertEdge(graphptr, Tag, Tag, edgeWeight);
Node* getNode(graphptr mainGraph, Tag n);
void printGraph(graphptr);
void exitProgram(graphptr);
List BFS_path(graphptr mainGraph, Tag source, Tag target);
void create_path(List list, int source, int target, List parent);

#endif //SERVER_GRAPH_H
