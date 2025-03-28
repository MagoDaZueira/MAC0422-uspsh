/******************************************************************
***************************** INCLUDES ***************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sched.h>

/******************************************************************
***************************** DEFINES ****************************/
#define MAX_LINE_SIZE 128
#define MAX_CORES 3
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
char* active_threads[MAX_CORES];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
Queue* finished_threads;

int is_active_thread(char *name) {
    for (int i = 0; i < MAX_CORES; i++) {
        if (active_threads[i] && !strcmp(active_threads[i], name))
            return 1;
    }
    return 0;
}

void set_active_thread(char *name) {
    for (int i = 0; i < MAX_CORES; i++) {
        if (active_threads[i] == NULL) {
            active_threads[i] = name;
            return;
        }
    }
}

void remove_active_thread(char *name) {
    for (int i = 0; i < MAX_CORES; i++) {
        if (active_threads[i] && !strcmp(active_threads[i], name)) {
            active_threads[i] = NULL;
            return;
        }
    }
}

void do_work(double seconds) {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    do {
        gettimeofday(&end, NULL);
    } while ((end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6 < seconds);
}

int running = 0;

void* execute(void* arg) {
    Process* p = (Process*)arg;
    int time = 0;

    // double time_lim = p->dt - 0.5;
    int time_lim = p->dt;

    while (time < time_lim) {
        // pthread_mutex_lock(&lock);
        while (!is_active_thread(p->name)) {
            pthread_cond_wait(&cond, &lock);
        }
        // pthread_mutex_unlock(&lock);
        
        do_work(1);
        time++;
    }

    pthread_mutex_lock(&lock);
    enqueue(finished_threads, p);
    remove_active_thread(p->name);
    running--;
    pthread_mutex_unlock(&lock);
    return NULL;
}

/******************************************************************
************************** ESCALONADORES *************************/
cpu_set_t cores[MAX_CORES];

void fcfs(FILE *file, Process **processes, int n) {
    Queue *q = new_queue(sizeof(Process));
    finished_threads = new_queue(sizeof(Process));

    int time = 0;
    int on_time, tr;

    int core_i = 0;
    int i = 0;

    while (i < n || !is_queue_empty(q) || running > 0) {
        if (i < n && time >= processes[i]->t0) {
            enqueue(q, processes[i]);
            i++;
        }

        pthread_mutex_lock(&lock);
        while (running < MAX_CORES && !is_queue_empty(q)) {
            pthread_t thread;
            // pthread_mutex_lock(&lock);

            Process* process_copy = malloc(sizeof(Process));
            *process_copy = *(Process*)front(q);
            set_active_thread(process_copy->name);
            pthread_create(&thread, NULL, execute, process_copy); 
            
            pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cores[core_i]);
            core_i = (core_i + 1) % MAX_CORES;

            dequeue(q);
            running++;

            // pthread_mutex_unlock(&lock);
            pthread_cond_broadcast(&cond);
        }
        pthread_mutex_unlock(&lock);

        sleep(1);
        time++;

        pthread_mutex_lock(&lock);
        while (!is_queue_empty(finished_threads)) {
            Process* p = front(finished_threads);
            tr = time - p->t0;
            on_time = time <= p->deadline;
            fprintf(file, "%s %d %d %d\n", p->name, tr, time, on_time);
            dequeue(finished_threads);
        }
        pthread_mutex_unlock(&lock);
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

    for (int i = 0; i < MAX_CORES; i++) active_threads[i] = NULL;

    for (int i = 0; i < MAX_CORES; i++) {
        CPU_ZERO(&cores[i]);
        CPU_SET(i, &cores[i]);
    }

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
