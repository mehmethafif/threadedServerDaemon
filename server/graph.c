//
// Created by mehmet on 20.06.2020.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "graph.h"
#include "list.h"

graphptr createGraph(){
    graphptr newGraph;
    newGraph = (Graph*)malloc(sizeof(Graph));
    if (newGraph == NULL)
    {
        fprintf(stderr, "Unable to allocate space\n");
        exit(-1);
    }

    newGraph->nodes = NULL;
    newGraph->node_count=0;
    newGraph->edge_count=0;

    return newGraph;
}

void insertNode(Tag nodeTag, graphptr mainGraph){
    //checking if the node already exists in the graph
    Node *tmp = mainGraph->nodes;
    while (tmp != NULL)
    {
        if (tmp->name == nodeTag)
        {
            printf("Node %d Exists\n", nodeTag);
            return;
        }
        tmp = tmp->next;
    }

    Node *newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL)
    {
        printf("Unable to allocate space\n");
        exit(-1);
    }

    newNode->name = nodeTag;
    newNode->visited = 0;
    newNode->edges = NULL;
    newNode->next = NULL;

    //checking if the graph is empty
    if (mainGraph->nodes == NULL)
    {
        mainGraph->nodes = newNode;
        mainGraph->node_count += 1;
    }
    else
    {
        //placing the new node at the end
        tmp = mainGraph->nodes;
        while ((tmp->next) != NULL)
        {
            tmp = tmp->next;
        }
        tmp->next = newNode;
        mainGraph->node_count += 1;
    }
}

void insertEdge(graphptr mainGraph, Tag ni, Tag nj, edgeWeight w){
    //check if the nodes exist or not and add them to the graph if not
    Node *tmp = mainGraph->nodes;
    while ((tmp != NULL) && (tmp->name != ni))
    {
        tmp = tmp->next;
    }
    if (tmp == NULL)
    {
        insertNode(ni, mainGraph);
    }
    tmp = mainGraph->nodes;
    while ((tmp != NULL) && (tmp->name != nj))
    {
        tmp = tmp->next;
    }
    if (tmp == NULL)
    {
        insertNode(nj, mainGraph);
    }

    //finding the nodes in the graph
    Node *niNode = mainGraph->nodes;
    while ((niNode != NULL) && (niNode->name != ni))
    {
        niNode = niNode->next;
    }
    Node *njNode = mainGraph->nodes;
    while ((njNode != NULL) && (njNode->name != nj))
    {
        njNode = njNode->next;
    }

    //adding an edge from ni to nj
    Edge *newEdge= (Edge*)malloc(sizeof(Edge));
    if (newEdge == NULL)
    {
        printf("Unable to allocate space\n");
        exit(-1);
    }

    newEdge->next = NULL;
    newEdge->visited = 0;
    newEdge->weight = w;
    newEdge->dest = njNode;

    //connecting the edge to nj
    Edge *tmpEdge = niNode->edges;
    if (niNode->edges == NULL)
    {
        niNode->edges = newEdge;
        mainGraph->edge_count += 1;
    }
    else
    {
        //placing the new node at the end
        tmpEdge = niNode->edges;
        while ((tmpEdge->next) != NULL)
        {
            tmpEdge = tmpEdge->next;
        }
        tmpEdge->next = newEdge;
        mainGraph->edge_count += 1;
    }

}

Node* getNode(graphptr mainGraph, Tag n){
    Node *node = mainGraph->nodes;
    while ((node != NULL) && (node->name != n))
    {
        node = node->next;
    }
    if (node == NULL)
    {
        printf("|%d| does not exist - abort-l\n", n);
        return NULL;
    } else{
        return node;
    }
}


void printGraph(graphptr mainGraph){
    Edge *currentEdge;
    Node *tmp;
    Node *current = mainGraph->nodes;
    while (current != NULL)
    {
        printf("%d", current->name);
        currentEdge = current->edges;
        while (currentEdge != NULL)
        {
            tmp = currentEdge->dest;
            printf(" --> %d", tmp->name);
            currentEdge = currentEdge->next;
        }
        current = current->next;
        printf("\n");
    }
}

void exitProgram(graphptr mainGraph){
    Edge *currentEdge;
    Edge *tmpEdge;
    Node *tmpNode;
    Node *currentNode;

    currentNode = mainGraph->nodes;
    while (currentNode != NULL)
    {
        //I start with freeing the edges of every node
        currentEdge = currentNode->edges;
        while (currentEdge != NULL)
        {
            tmpEdge = currentEdge;
            currentEdge = currentEdge->next;
            free(tmpEdge);
        }
        currentNode->edges = NULL;

        tmpNode = currentNode;
        currentNode = currentNode->next;
        free(tmpNode);
    }

    //finally I'm freeing the graph
    free(mainGraph);

}

List BFS_path(graphptr mainGraph, Tag source, Tag target){
    //white:0 gray:1 black:2
    List color = newList();
    List parent = newList();
    List distance = newList();
    Node* node = mainGraph->nodes ;
    while(node != NULL) {
        append_data2(color, node->name, 0);
        append_data2(parent, node->name, -1);
        append_data2(distance, node->name, INT_MAX);
        node = node->next;
    }
    set_data2(distance, source, 0);
    set_data2(color, source, 0);
    List queue = newList();
    append(queue, source);

    while(length(queue) != 0) {
        int curr_n = front(queue);
        deleteFront(queue);
        Node *current = getNode(mainGraph, curr_n);
        Edge *edge = current->edges;

        while (edge != NULL){
            if (get_data2(color, edge->dest->name) == 0){
                set_data2(color, edge->dest->name, 1);
                set_data2(parent, edge->dest->name, curr_n);
                set_data2(distance, edge->dest->name, get_data2(distance, curr_n)+1);
                append(queue, edge->dest->name);
            }
            edge = edge->next;
        }
        set_data2(color, curr_n, 2);
    }
    freeList(&queue);
    freeList(&color);
    freeList(&distance);

    List path = newList();
    create_path(path, source,target, parent);
    freeList(&parent);
    return path;

}

void create_path(List list, int source, int target, List parent){
    if (source == target){
        append(list, target);
    } else if(get_data2(parent, target) == -1){
        append(list, -1);
    } else{
        create_path(list, source, get_data2(parent, target), parent);
        append(list, target);
    }
}