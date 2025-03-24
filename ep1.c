/******************************************************************
***************************** INCLUDES ***************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************
*********************** IMPLEMENTAÇÃO QUEUE ***********************/
typedef struct Node {
    void *data;
    struct Node *next;
} Node;

typedef struct {
    Node *front;
    Node *back;
    size_t data_size;
} Queue;

Queue* new_queue(size_t data_size) {
    Queue *q = (Queue*)malloc(sizeof(Queue));
    q->front = q->back = NULL;
    q->data_size = data_size;
    return q;
}

int is_queue_empty(Queue *q) {
    return q->front == NULL;
}

void enqueue(Queue *q, void *data) {
    Node *new_node = (Node*)malloc(sizeof(Node));
    new_node->data = malloc(q->data_size);
    memcpy(new_node->data, data, q->data_size);
    new_node->next = NULL;

    if (is_queue_empty(q)) {
        q->front = new_node;
    }
    else {
        q->back->next = new_node;
    }
    q->back = new_node;
}

void dequeue(Queue *q) {
    if (is_queue_empty(q)) return;
    
    Node* temp = q->front;
    q->front = q->front->next;

    if (q->front == NULL) {
        q->back = NULL;
    }

    free(temp->data);
    free(temp);
}

void* front(Queue *q) {
    return q->front ? q->front->data : NULL;
}

/******************************************************************
*************************** FUNÇÃO MAIN **************************/
int main() {
    Queue *q = new_queue(sizeof(int));

    int x = 10, y = 4;
    enqueue(q, &x);
    enqueue(q, &y);

    while (!is_queue_empty(q)) {
        int num = *(int*) front(q);
        printf("Front: %d\n", num);
        dequeue(q);
    }

    free(q);

    return 0;
}
