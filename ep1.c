/******************************************************************
***************************** INCLUDES ***************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

/******************************************************************
***************************** DEFINES ****************************/
#define MAX_LINE_SIZE 128
#define min(a,b) a < b ? a : b

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


int compare_by_t0(const void *a, const void *b) {
    const Process *process_a = *(const Process **)a;
    const Process *process_b = *(const Process **)b;
    
    if (process_a->t0 < process_b->t0) return -1;
    if (process_a->t0 > process_b->t0) return 1;
    return 0;
}
/******************************************************************
************************ PROCESSOS/THREADS ***********************/
char* active_name;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void do_work(double seconds) {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    do {
        gettimeofday(&end, NULL);
    } while ((end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6 < seconds);
}

void* execute(void* arg) {
    Process* p = (Process*)arg;

    double time = 0;
    struct timeval start, end;

    while (time < p->dt) {
        pthread_mutex_lock(&lock);
        
        // Wait until this thread is active
        while (strcmp(active_name, p->name)) {
            pthread_cond_wait(&cond, &lock);
        }
        pthread_mutex_unlock(&lock);

        // Measure only execution time
        gettimeofday(&start, NULL);
        
        do_work(0.001);

        gettimeofday(&end, NULL);
        
        // Update elapsed time
        time += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    }

    return NULL;
}

/******************************************************************
************************** ESCALONADORES *************************/

void fcfs(FILE *file, Process **processes, int n) {
    double time = 0;
    struct timeval start, end;

    int on_time; double tr;

    for (int i = 0; i < n; i++) {
        if (time < processes[i]->t0) {
            sleep(processes[i]->t0 - time);
            time = processes[i]->t0;
        }

        pthread_t thread;
        active_name = processes[i]->name;

        Process* process_copy = malloc(sizeof(Process));
        *process_copy = *processes[i];

        gettimeofday(&start, NULL);

        pthread_create(&thread, NULL, execute, process_copy); 
        pthread_join(thread, NULL);

        pthread_cond_broadcast(&cond);

        gettimeofday(&end, NULL);
        time += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

        tr = time - processes[i]->t0;
        on_time = time <= processes[i]->deadline;

        fprintf(file, "%s %.4f %.4f %d\n", processes[i]->name, tr, time, on_time);
    }

    fprintf(file, "0\n");

    return;
}

/******************************************************************
*************************** FUNÇÃO MAIN **************************/
int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("Uso correto: ep1 <numero do escalonador> <trace entrada> <trace saida>\n");
        return 1;
    }

    FILE *file_in = fopen(argv[2], "r");
    int line_amount = count_lines(file_in);
    fseek(file_in, 0, SEEK_SET);

    Process **processes = malloc(line_amount * sizeof(Process*));
    char *line = malloc(MAX_LINE_SIZE * sizeof(char));

    int i = 0;
    while (fgets(line, MAX_LINE_SIZE, file_in)) {
        processes[i++] = line_to_process(line);
    }

    fclose(file_in);
    qsort(processes, line_amount, sizeof(Process*), compare_by_t0);

    FILE *file_out = fopen(argv[3], "a");

    int scheduler_mode = atoi(argv[1]);
    switch (scheduler_mode) {
        case 1:
            fcfs(file_out, processes, line_amount);
            break;
        default:
            printf("O primeiro argumento <numero do escalonador> deve ser 1, 2 ou 3\n");
            return 1;
    }

    fclose(file_out);

    return 0;
}
