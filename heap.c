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

/******************************************************************
***************************** DEFINES ****************************/
#define MAX_LINE_SIZE 128
#define min(a,b) a < b ? a : b

/******************************************************************
*********************** IMPLEMENTAÇÃO HEAP ***********************/
typedef int (*Comparator)(const void*, const void*);

typedef struct {
    void **pq;
    int n;
    int capacity;
    Comparator cmp;
} MinPQ;

void swim(MinPQ* pq, int k);
void sink(MinPQ* pq, int k);
void swap(MinPQ* pq, int i, int j);
void resize(MinPQ* pq, int capacity);

MinPQ* new_pq(int init_capacity, Comparator cmp) {
    MinPQ *pq = (MinPQ*)malloc(sizeof(MinPQ));
    pq->n = 0;
    pq->pq = (void**)malloc((init_capacity + 1) * sizeof(void*));
    pq->capacity = init_capacity;
    pq->cmp = cmp;
    return pq;
}

int is_pq_empty(MinPQ *pq) {
    return pq->n <= 0;
}

void pq_push(MinPQ* pq, void *data) {
    if (pq->n == pq->capacity) resize(pq, 2 * pq->capacity);
    pq->pq[++pq->n] = data;
    swim(pq, pq->n);
}

void* pq_pop(MinPQ* pq) {
    if (is_pq_empty(pq)) return NULL;
    void* min = pq->pq[1];
    swap(pq, 1, pq->n--);
    sink(pq, 1);
    pq->pq[pq->n + 1] = NULL;
    if ((pq->n > 0) && (pq->n <= (pq->capacity - 1) / 4)) resize(pq, pq->capacity / 2);
    return min;
}

void swim(MinPQ* pq, int k) {
    while (k > 1 && pq->cmp(pq->pq[k], pq->pq[k/2]) < 0) {
        swap(pq, k/2, k);
        k /= 2;
    }
}

void sink(MinPQ* pq, int k) {
    while (2 * k <= pq->n) {
        int j = 2 * k;
        if (j < pq->n && pq->cmp(pq->pq[j], pq->pq[j+1]) > 0) j++;
        if (pq->cmp(pq->pq[k], pq->pq[j]) <= 0) break;
        swap(pq, k, j);
        k = j;
    }
}

void* top(MinPQ *pq) {
    if (is_pq_empty(pq)) return NULL;
    return pq->pq[1];
}

void swap(MinPQ* pq, int i, int j) {
    void* temp = pq->pq[i];
    pq->pq[i] = pq->pq[j];
    pq->pq[j] = temp;
}

void resize(MinPQ* pq, int capacity) {
    void** temp = (void**)malloc((capacity + 1) * sizeof(void*));
    if (temp) {
        for (int i = 1; i <= pq->n; i++) {
            temp[i] = pq->pq[i];
        }
        free(pq->pq);
        pq->pq = temp;
        pq->capacity = capacity;
    }
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

int compare_by_dt(const void *a, const void *b) {
    const Process *process_a = (const Process *)a;
    const Process *process_b = (const Process *)b;
    
    if (process_a->dt < process_b->dt) return -1;
    if (process_a->dt > process_b->dt) return 1;
    return 0;
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
    
    MinPQ* pq = new_pq(4, compare_by_dt);
    for (int i = 0; i < line_amount; i++) {
        pq_push(pq, processes[i]);
    }

    while (!is_pq_empty(pq)) {
        Process* p = (Process*)pq_pop(pq);
        printf("Name: %s | t0: %d | dt: %d | deadline: %d\n", p->name, p->t0, p->dt, p->deadline);
    }

    return 0;
}
