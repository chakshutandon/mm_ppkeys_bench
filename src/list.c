#include "list.h"

Node *insert_node(Node *head, pthread_t thread_id) {
    Node *ptr = malloc(sizeof(Node));
    ptr->thread_id = thread_id;
    ptr->next = head;
    return ptr;
}

Node *remove_node(Node *head, pthread_t thread_id) {
    Node *ptr, *prev;
    if (!head) {
        return NULL;
    }
    if (head->thread_id == thread_id) {
        return head->next;
    }
    for (prev = head, ptr = head->next; ptr; ptr = ptr->next) {
        if (ptr->thread_id == thread_id) {
            prev->next = ptr->next;
            free(ptr);
        }
        prev = ptr;
    }
    return head;
}  

void print_list(Node *head) {
    Node *ptr;
    for (ptr = head; ptr; ptr = ptr->next)
        printf("%lu\n", ptr->thread_id);
}