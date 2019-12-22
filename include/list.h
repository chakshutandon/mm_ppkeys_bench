#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include <pthread.h>

typedef struct node {
    pthread_t thread_id;
    struct node* next;
} Node;

Node *insert_node(Node *head, pthread_t thread_id);
Node *remove_node(Node *head, pthread_t thread_id);
void print_list(Node *head);
