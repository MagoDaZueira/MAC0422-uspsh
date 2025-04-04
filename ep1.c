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
// #define MAX_CORES 3
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
    int size;
} Queue;

Queue* new_queue(size_t data_size) {
    Queue *q = (Queue*)malloc(sizeof(Queue));
    q->front = q->back = NULL;
    q->data_size = data_size;
    q->size = 0;
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
    q->size++;
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
    q->size--;
}

void* front(Queue *q) {
    return q->front ? q->front->data : NULL;
}

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
    int remaining;
    int deadline;
    int core_id;
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
 ************************ PROCESSOS/THREADS ***********************/
 int MAX_CORES;
 char** active_names;
 Process** active_threads;
 pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
 pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
 pthread_cond_t thread_cond = PTHREAD_COND_INITIALIZER;
 pthread_cond_t scheduler_cond = PTHREAD_COND_INITIALIZER;
 Queue* finished_threads;
 int* core_dt;

int running = 0;
int threads_ready = 0;

int is_active_thread(char *name) {
    for (int i = 0; i < MAX_CORES; i++) {
        if (active_names[i] && !strcmp(active_names[i], name))
            return 1;
    }
    return 0;
}

void set_active_thread(char *name) {
    for (int i = 0; i < MAX_CORES; i++) {
        if (active_names[i] == NULL) {
            active_names[i] = name;
            return;
        }
    }
}

void remove_active_thread(char *name) {
    for (int i = 0; i < MAX_CORES; i++) {
        if (active_names[i] && !strcmp(active_names[i], name)) {
            active_names[i] = NULL;
            return;
        }
    }
}

int laziest_core() {
    int smallest_dt = 10000;
    int core_id = -1;
    for (int i = 0; i < MAX_CORES; i++) {
        if (core_dt[i] < smallest_dt) {
            smallest_dt = core_dt[i];
            core_id = i;
        }
    }
    return core_id;
}

void do_work(double seconds) {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    do {
        gettimeofday(&end, NULL);
    } while ((end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6 < seconds);
}

void* execute(void* arg) {
    Process* p = (Process*)arg;
    int time = 0;
    int time_lim = p->dt;

    while (time < time_lim) {
        pthread_mutex_lock(&lock);
        while (!is_active_thread(p->name)) {
            pthread_cond_wait(&thread_cond, &lock);
        }
        pthread_mutex_unlock(&lock);

        do_work(0.5);

        pthread_mutex_lock(&lock);
        core_dt[p->core_id]--;
        p->remaining--;
        time++;
        if (time < time_lim) threads_ready++;

        if (threads_ready >= running) {
            pthread_cond_signal(&scheduler_cond);
        }

        while (threads_ready > 0 && time < time_lim) {
            pthread_cond_wait(&thread_cond, &lock);
        }
        pthread_mutex_unlock(&lock);
    }

    // A thread acabou
    pthread_mutex_lock(&lock);
    enqueue(finished_threads, p);
    // remove_active_thread(p->name);
    active_names[p->core_id] = NULL;
    active_threads[p->core_id] = NULL;
    running--;
    // core_dt[p->core_id] -= p->dt;
    pthread_cond_signal(&scheduler_cond);
    pthread_mutex_unlock(&lock);

    return NULL;
}


/******************************************************************
************************** ESCALONADORES *************************/
cpu_set_t* cores;

void fcfs(FILE *file, Process **processes, int n) {
    Queue *q = new_queue(sizeof(Process));
    finished_threads = new_queue(sizeof(Process));

    int time = 0;
    int core_i = 0;
    int i = 0;
    threads_ready = 0;

    while (i < n || !is_queue_empty(q) || running > 0 || !is_queue_empty(finished_threads)) {
        // Adiciona processos que chegaram agora a fila de prontos
        while (i < n && time >= processes[i]->t0) {
            enqueue(q, processes[i]);
            i++;
        }

        // Inicia processos da fila de prontos
        pthread_mutex_lock(&lock);
        while (running < MAX_CORES && !is_queue_empty(q)) {
            pthread_t thread;
            Process* process_copy = malloc(sizeof(Process));
            *process_copy = *(Process*)front(q);
            set_active_thread(process_copy->name);

            core_i = laziest_core();
            core_dt[core_i] += process_copy->dt;
            process_copy->core_id = core_i;
            process_copy->remaining = process_copy->dt;

            pthread_create(&thread, NULL, execute, process_copy);
            pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cores[core_i]);

            dequeue(q);
            running++;
        }
        pthread_mutex_unlock(&lock);
        
        // Espera todas as threads acabarem o ciclo
        pthread_mutex_lock(&lock);
        pthread_cond_broadcast(&thread_cond);
        while (threads_ready < running) {
            pthread_cond_wait(&scheduler_cond, &lock);
        }
        threads_ready = 0;
        pthread_mutex_unlock(&lock);
        
        pthread_mutex_lock(&lock);
        while (!is_queue_empty(finished_threads)) {
            Process* p = front(finished_threads);
            int tr = time - p->t0 + 1;
            int on_time = (time + 1 <= p->deadline);
            fprintf(file, "%s %d %d %d\n", p->name, tr, time + 1, on_time);
            dequeue(finished_threads);
        }
        pthread_mutex_unlock(&lock);

        time++;
    }

    fprintf(file, "0\n");
}

void srtn(FILE *file, Process **processes, int n) {
    MinPQ *pq_array[MAX_CORES];
    for (int i = 0; i < MAX_CORES; i++) pq_array[i] = new_pq(8, compare_by_dt);

    finished_threads = new_queue(sizeof(Process));

    int time = 0;
    int core_i = 0;
    int i = 0;
    threads_ready = 0;

    int preemptions = 0;
    int all_queues_empty = 0;

    while (i < n || running > 0 || !is_queue_empty(finished_threads) || !all_queues_empty) {
        // Adiciona processos que chegaram agora a fila de prontos
        pthread_mutex_lock(&lock);
        while (i < n && time >= processes[i]->t0) {
            pthread_t thread;
            core_i = laziest_core();
            core_dt[core_i] += processes[i]->dt;
            processes[i]->core_id = core_i;
            processes[i]->remaining = processes[i]->dt;

            pthread_create(&thread, NULL, execute, processes[i]);
            pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cores[core_i]);
            
            // Ao chegar um processo, decide se havera preempcao
            if (active_threads[core_i] != NULL && processes[i]->dt < active_threads[core_i]->remaining) {
                active_names[core_i] = processes[i]->name;
                pq_push((pq_array[core_i]), active_threads[core_i]);
                active_threads[core_i] = processes[i];
                preemptions++;
            }
            else{
                pq_push((pq_array[core_i]), processes[i]);
            }
            
            i++;
        }

        for (int i = 0; i < MAX_CORES; i++) {
            if (active_threads[i] == NULL && !is_pq_empty(pq_array[i])) {
                Process* candidate = top(pq_array[i]);
                pq_pop(pq_array[i]);
                active_names[i] = candidate->name;
                active_threads[i] = candidate;
                running++;
            }
        }

        pthread_mutex_unlock(&lock);
        
        // Espera todas as threads acabarem o ciclo
        pthread_mutex_lock(&lock);
        pthread_cond_broadcast(&thread_cond);
        while (threads_ready < running) {
            pthread_cond_wait(&scheduler_cond, &lock);
        }
        threads_ready = 0;
        pthread_mutex_unlock(&lock);
        
        pthread_mutex_lock(&lock);
        while (!is_queue_empty(finished_threads)) {
            Process* p = front(finished_threads);
            int tr = time - p->t0 + 1;
            int on_time = (time + 1 <= p->deadline);
            fprintf(file, "%s %d %d %d\n", p->name, tr, time + 1, on_time);
            dequeue(finished_threads);
        }
        pthread_mutex_unlock(&lock);

        time++;
        
        all_queues_empty = 1;
        for (int i = 0; i < MAX_CORES; i++) {
            if (!is_pq_empty(pq_array[i]) || active_threads[i] != NULL) {
                all_queues_empty = 0;
                break;
            }
        }
    }

    fprintf(file, "%d\n", preemptions);
}


/******************************************************************
*************************** FUNÇÃO MAIN **************************/
int main(int argc, char *argv[]) {

    MAX_CORES = sysconf(_SC_NPROCESSORS_ONLN);
    // MAX_CORES = 1;
    active_names = malloc(MAX_CORES * sizeof(char*));
    active_threads = malloc(MAX_CORES * sizeof(Process*));
    cores = malloc(MAX_CORES * sizeof(cpu_set_t));
    core_dt = malloc(MAX_CORES * sizeof(int));

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

    for (int i = 0; i < MAX_CORES; i++) active_names[i] = NULL;
    for (int i = 0; i < MAX_CORES; i++) active_threads[i] = NULL;

    for (int i = 0; i < MAX_CORES; i++) {
        active_names[i] = NULL;
        core_dt[i] = 0;
        CPU_ZERO(&cores[i]);
        CPU_SET(i, &cores[i]);
    }

    int scheduler_mode = atoi(argv[1]);
    switch (scheduler_mode) {
        case 1:
            fcfs(file_out, processes, line_amount);
            break;
        case 2:
            srtn(file_out, processes, line_amount);
            break;
        default:
            printf("O primeiro argumento <numero do escalonador> deve ser 1, 2 ou 3\n");
            return 1;
    }

    fclose(file_out);

    return 0;
}
