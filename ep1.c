/******************************************************************
***************************** INCLUDES ***************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************
***************************** DEFINES ****************************/
#define MAX_LINE_SIZE 128

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
************************** UTILITÁRIOS ***************************/
typedef struct Process {
    char *name;
    int t0;
    int dt;
    int deadline;
} Process;

int count_lines(FILE *file) {
    int count = 0;
    char ch;

    ch = fgetc(file);
    while (ch != EOF) {
        if (ch == '\n') {
            count++;
        }
        ch = fgetc(file);
    }

    return count;
}

Process *line_to_process(char *line) {
    char **items;
    char *token;
    
    // Reserva memória de acordo com a quantia de itens por linha
    items = malloc(4 * sizeof(char*));
    
    // Preenche o vetor com as partes separadas por espaço da linha
    int i = 0;
    token = strtok(line, " ");
    while (token != NULL) {
        items[i++] = token;
        token = strtok(NULL, " ");
    }
    
    // Cria um processo com os dados obtidos
    Process *p = malloc(sizeof(Process));

    p->name = malloc(strlen(items[0]) + 1);
    strcpy(p->name, items[0]);
    p->t0 = atoi(items[1]);
    p->dt = atoi(items[2]);
    p->deadline = atoi(items[3]);

    return p;
}

/******************************************************************
*************************** FUNÇÃO MAIN **************************/
int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("Uso correto: ep1 <numero do escalonador> <trace entrada> <trace saida>\n");
        return 1;
    }

    FILE *file = fopen(argv[2], "r");
    int line_amount = count_lines(file);
    fseek(file, 0, SEEK_SET);

    Process **processes = malloc(line_amount * sizeof(Process*));
    char *line = malloc(MAX_LINE_SIZE * sizeof(char));

    int i = 0;
    while (fgets(line, MAX_LINE_SIZE, file)) {
        processes[i++] = line_to_process(line);
    }
    
    fclose(file);

    for (int i = 0; i < line_amount; i++) {
        printf("%s %d %d %d\n", processes[i]->name, processes[i]->t0, processes[i]->dt, processes[i]->deadline);
    }


    // Queue *q = new_queue(sizeof(int));

    // int x = 10, y = 4;
    // enqueue(q, &x);
    // enqueue(q, &y);

    // while (!is_queue_empty(q)) {
    //     int num = *(int*) front(q);
    //     printf("Front: %d\n", num);
    //     dequeue(q);
    // }

    // free(q);

    return 0;
}
