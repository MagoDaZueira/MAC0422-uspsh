#ifndef EP1_H
#define EP1_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// ======================== DEFINES ========================
#define MAX_LINE_SIZE 128
#define TIME_WORKING 1
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

// ======================== STRUCTS ========================

// Fila (Queue)
typedef struct Node {
    void *data;
    struct Node *next;
} Node;

typedef struct {
    Node *front;
    Node *back;
    int size;
} Queue;

// Heap (MinPQ)
typedef int (*Comparator)(const void*, const void*);

typedef struct {
    void **pq;
    int n;
    int capacity;
    Comparator cmp;
} MinPQ;

// Processo
typedef struct Process {
    char *name;     // Nome do processo
    int t0;         // Tempo de chegada
    int dt;         // Tempo de execucao
    int remaining;  // Tempo restante (de um quantum ou da execucao total)
    int deadline;   // Tempo limite ideal
    int core_id;    // Nucleo ao qual a thread esta atribuida
    int quantum;    // Intervalo de tempo alocado no escalonamento por prioridade
} Process;

// ======================== QUEUE FUNÇÕES ========================
Queue* new_queue();
int is_queue_empty(Queue *q);
void enqueue(Queue *q, void *data);
void dequeue(Queue *q);
void* front(Queue *q);

// ======================== HEAP FUNÇÕES ========================
void swim(MinPQ* pq, int k);
void sink(MinPQ* pq, int k);
void swap(MinPQ* pq, int i, int j);
void resize(MinPQ* pq, int capacity);
MinPQ* new_pq(int init_capacity, Comparator cmp);
int is_pq_empty(MinPQ *pq);
void pq_push(MinPQ* pq, void *data);
void* pq_pop(MinPQ* pq);
void* top(MinPQ *pq);

// ======================== UTILITÁRIOS ========================
int count_lines(FILE *file);
Process* line_to_process(char *line);
int compare_by_t0(const void *a, const void *b);
int compare_by_dt(const void *a, const void *b);

// ======================== THREADS & ESCALONADOR ========================
extern int MAX_CORES;
extern Process** active_threads;
extern int* core_dt;
extern Queue* finished_threads;

extern pthread_mutex_t lock;
extern pthread_cond_t cond;
extern pthread_cond_t thread_cond;
extern pthread_cond_t scheduler_cond;

extern int running;
extern int threads_ready;

int laziest_core();
void do_work(double seconds);
void* execute(void* arg);

#endif // EP1_H
