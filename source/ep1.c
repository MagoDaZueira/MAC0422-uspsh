/*
NOME: Otávio Garcia Capobianco
NUSP: 15482671
EXERCÍCIO-PROGRAMA: EP1
*/

/******************************************************************
***************************** INCLUDES ***************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "ep1.h"

/******************************************************************
*********************** IMPLEMENTAÇÃO QUEUE ***********************/
Queue* new_queue() {
    Queue *q = (Queue*)malloc(sizeof(Queue));
    q->front = q->back = NULL;
    q->size = 0;
    return q;
}

int is_queue_empty(Queue *q) {
    return q->front == NULL;
}

void enqueue(Queue *q, void *data) {
    Node *new_node = (Node*)malloc(sizeof(Node));
    new_node->data = data;
    new_node->next = NULL;

    if (is_queue_empty(q)) {
        q->front = new_node;
    } else {
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

    free(temp);
    q->size--;
}

void* front(Queue *q) {
    return q->front ? q->front->data : NULL;
}

/******************************************************************
*********************** IMPLEMENTAÇÃO HEAP ***********************/
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
// Conta as linhas de um arquivo
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

// Transforma uma linha do arquivo em uma instancia de Process
// Supoe formatacao correta da linha
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

// Comparador de processos com base em t0
int compare_by_t0(const void *a, const void *b) {
    const Process *process_a = *(const Process **)a;
    const Process *process_b = *(const Process **)b;
    
    if (process_a->t0 < process_b->t0) return -1;
    if (process_a->t0 > process_b->t0) return 1;
    return 0;
}

// Comparador de processos com base em dt
int compare_by_dt(const void *a, const void *b) {
    const Process *process_a = (const Process *)a;
    const Process *process_b = (const Process *)b;
    
    if (process_a->dt < process_b->dt) return -1;
    if (process_a->dt > process_b->dt) return 1;
    return 0;
}

/******************************************************************
 ************************ PROCESSOS/THREADS ***********************/
int MAX_CORES;            // Numero de cores ativos da maquina
Process** active_threads; // Vetor que guarda quais threads estao rodando
int* core_dt;             // Vetor com a soma dos tempos restantes das threads de um core 
Queue* finished_threads;  // Threads que finalizaram sua execucao, a serem imprimidas

// Controladores de threads
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t thread_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t scheduler_cond = PTHREAD_COND_INITIALIZER;

// Variaveis para controle do escalonador e sincronizacao
int running = 0;       // Qtde de threads atualmente rodando
int threads_ready = 0; // Qtde de threads que finalizaram um ciclo de trabalho

// Identifica o nucleo com menor carga de trabalho
int laziest_core() {
    int smallest_dt = 10000; // Menor carga atual (inicializada arbitrariamente grande)
    int core_id = -1;        // Core com a tal menor carga
    for (int i = 0; i < MAX_CORES; i++) {
        if (core_dt[i] < smallest_dt) {
            smallest_dt = core_dt[i];
            core_id = i;
        }
    }
    return core_id;
}

// Realiza operacoes inuteis por um intervalo de tempo especificado
void do_work(double seconds) {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    do {
        gettimeofday(&end, NULL);
    } while ((end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6 < seconds);
}

// Funcao principal das threads
void* execute(void* arg) {
    // Obtem o processo correspondente pelo parametro
    Process* p = (Process*)arg;

    // Loop principal da funcao, dura p->dt segundos
    int time = 0; // Tempo local
    while (time < p->dt) {

        // Verifica se esta thread esta ativa
        pthread_mutex_lock(&lock);
        while (p != active_threads[p->core_id]) {
            pthread_cond_wait(&thread_cond, &lock); // Thread pausa enquanto nao estiver ativa
        }
        pthread_mutex_unlock(&lock);

        do_work(TIME_WORKING); // Trabalho inutil por 1 segundo

        // Apos trabalhar, altera as variaveis necessarias
        // e sincroniza com as demais threads 
        pthread_mutex_lock(&lock);
        core_dt[p->core_id]--;
        p->remaining--;
        p->quantum--;
        time++;
        if (time < p->dt) threads_ready++;
        
        // Todas as threads chegaram aqui, sinaliza o escalonador
        if (threads_ready >= running) {
            pthread_cond_signal(&scheduler_cond);
        }

        // Threads esperam ate o escalonador sinalizar
        while (threads_ready > 0 && time < p->dt) {
            pthread_cond_wait(&thread_cond, &lock);
        }
        pthread_mutex_unlock(&lock);
    }

    // A thread acabou
    pthread_mutex_lock(&lock);
    enqueue(finished_threads, p);
    active_threads[p->core_id] = NULL;
    running--;
    pthread_cond_signal(&scheduler_cond); // Avisa o escalonador (evita deadlock)
    pthread_mutex_unlock(&lock);

    return NULL;
}


/******************************************************************
************************** ESCALONADORES *************************/
cpu_set_t* cores; // Vetor com referencias aos nucleos da maquina

// Formula arbitraria para definir o quantum de um processo
int calculate_quantum(Process* p, int time) {
    if (time + p->remaining >= p->deadline) return 1;
    int urgency = (p->remaining * 10) / (p->deadline - time);
    return max(1, urgency);
}

// Escalonador 1: FCFS
void fcfs(FILE *file, Process **processes, int n) {
    // Fila de prontos compartilhada por todos os cores
    Queue *ready = new_queue();

    // Inicializa variaveis de controle do escalonador
    int time = 0;   // Tempo da simulacao
    int core_i = 0; // Indice relativo ao core ao qual um processo sera atribuido
    int i = 0;      // Indice atual do vetor de processos
    threads_ready = 0;

    // Loop principal do escalonador
    while (i < n || !is_queue_empty(ready) || running > 0 || !is_queue_empty(finished_threads)) {
        // Adiciona processos que chegaram agora a fila de prontos
        while (i < n && time >= processes[i]->t0) {
            enqueue(ready, processes[i]);
            i++; // Passa para o próximo processo do vetor inicial
        }

        // Inicia processos da fila de prontos
        pthread_mutex_lock(&lock);
        while (running < MAX_CORES && !is_queue_empty(ready)) {
            // Proximo processo na ordem de chegada
            Process *candidate = front(ready);
            
            // Seleciona o core com menos carga e atribui valores a variaveis relacionadas
            core_i = laziest_core();
            core_dt[core_i] += candidate->dt;
            candidate->core_id = core_i;
            candidate->remaining = candidate->dt;
            candidate->quantum = candidate->dt;
            active_threads[core_i] = candidate;
            
            // Criação da thread e atribuição ao core
            pthread_t thread;
            pthread_create(&thread, NULL, execute, candidate);
            pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cores[core_i]);

            // Retira o processo da fila de prontos e o considera como "rodando"
            dequeue(ready);
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
        
        time++; // Tempo que as threads gastaram acima

        // Caso algum processo tenha finalizado, o imprime na formatação correta
        pthread_mutex_lock(&lock);
        while (!is_queue_empty(finished_threads)) {
            Process* p = front(finished_threads);
            int tr = time - p->t0;
            int on_time = (time <= p->deadline);
            fprintf(file, "%s %d %d %d\n", p->name, tr, time, on_time);
            dequeue(finished_threads);
        }
        pthread_mutex_unlock(&lock);
    }

    // Sabemos que nao haverao preempcoes no FCFS, ultima linha contera 0
    fprintf(file, "0\n");
}

// Escalonador 2: SRTN
void srtn(FILE *file, Process **processes, int n) {
    // Vetor de filas de prioridade
    // Cada fila é a fila de prontos de um dado nucleo,
    // Onde o proximo processo é aquele de menor tempo de execução
    MinPQ *pq_array[MAX_CORES];
    for (int i = 0; i < MAX_CORES; i++) pq_array[i] = new_pq(8, compare_by_dt);

    // Inicializa variaveis de controle do escalonador
    int time = 0;             // Tempo da simulacao
    int core_i = 0;           // Indice relativo ao core ao qual um processo sera atribuido
    int i = 0;                // Indice atual do vetor de processos
    int preemptions = 0;      // Conta quantas preempcoes houveram
    int all_queues_empty = 0; // Indica se todas as filas de pronto estao vazias
    threads_ready = 0;

    // Loop principal do escalonador
    while (i < n || running > 0 || !is_queue_empty(finished_threads) || !all_queues_empty) {
        pthread_mutex_lock(&lock);
        // Gerencia processos que chegaram agora
        while (i < n && time >= processes[i]->t0) {
            // Seleciona o core com menos carga e atribui valores a variaveis relacionadas
            core_i = laziest_core();
            core_dt[core_i] += processes[i]->dt;
            processes[i]->core_id = core_i;
            processes[i]->remaining = processes[i]->dt;
            processes[i]->quantum = processes[i]->dt;
            
            // Criação da thread e atribuição ao core
            pthread_t thread;
            pthread_create(&thread, NULL, execute, processes[i]);
            pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cores[core_i]);
            
            // Decide se haverá preempção, baseado no tempo restante do processo rodando no core
            if (active_threads[core_i] != NULL && processes[i]->dt < active_threads[core_i]->remaining) {
                // Preempção, processo rodando volta pra fila e o que chegou roda
                pq_push((pq_array[core_i]), active_threads[core_i]);
                active_threads[core_i] = processes[i];
                preemptions++;
            }
            else {
                // Sem preempção, processo que chegou vai pra fila
                pq_push((pq_array[core_i]), processes[i]);
            }
            
            i++; // Passa para o próximo processo do vetor inicial
        }

        // Verifica se algum core está sem thread rodando, com fila de prontos não vazia
        for (int i = 0; i < MAX_CORES; i++) {
            if (active_threads[i] == NULL && !is_pq_empty(pq_array[i])) {
                // Põe o próximo da fila do núcleo para rodar
                Process* candidate = top(pq_array[i]);
                pq_pop(pq_array[i]);
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
        
        time++; // Tempo que as threads gastaram acima

        // Caso algum processo tenha finalizado, o imprime na formatação correta
        pthread_mutex_lock(&lock);
        while (!is_queue_empty(finished_threads)) {
            Process* p = front(finished_threads);
            int tr = time - p->t0;
            int on_time = (time <= p->deadline);
            fprintf(file, "%s %d %d %d\n", p->name, tr, time, on_time);
            dequeue(finished_threads);
        }
        pthread_mutex_unlock(&lock);
        
        // Verifica se alguma das filas de pronto não está vazia
        all_queues_empty = 1;
        for (int i = 0; i < MAX_CORES; i++) {
            if (!is_pq_empty(pq_array[i]) || active_threads[i] != NULL) {
                all_queues_empty = 0;
                break;
            }
        }
    }

    // Imprime a quantidade de preempções ocorridas
    fprintf(file, "%d\n", preemptions);
}

// Escalonador 3: Escalonamento por Prioridade
void priority(FILE *file, Process **processes, int n) {
    // Vetor de filas
    // Cada fila é a fila de prontos de um dado núcleo, sem ordenação específica
    Queue *queues[MAX_CORES];
    for (int i = 0; i < MAX_CORES; i++) queues[i] = new_queue();

    // Inicializa variaveis de controle do escalonador
    int time = 0;             // Tempo da simulacao
    int core_i = 0;           // Indice relativo ao core ao qual um processo sera atribuido
    int i = 0;                // Indice atual do vetor de processos
    int preemptions = 0;      // Conta quantas preempcoes houveram
    int all_queues_empty = 0; // Indica se todas as filas de pronto estao vazias
    threads_ready = 0;

    // Loop principal do escalonador
    while (i < n || running > 0 || !is_queue_empty(finished_threads) || !all_queues_empty) {
        pthread_mutex_lock(&lock);
        // Gerencia processos que chegaram agora
        while (i < n && time >= processes[i]->t0) {
            // Seleciona o core com menos carga e atribui valores a variaveis relacionadas
            core_i = laziest_core();
            core_dt[core_i] += processes[i]->dt;
            processes[i]->core_id = core_i;
            processes[i]->remaining = processes[i]->dt;
            
            // Criação da thread e atribuição ao core
            pthread_t thread;
            pthread_create(&thread, NULL, execute, processes[i]);
            pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cores[core_i]);

            // Adiciona o processo à fila do core correspondente
            enqueue(queues[core_i], processes[i]);
            i++; // Passa para o próximo processo do vetor inicial
        }

        // Verifica se algum core está sem thread rodando, com fila de prontos não vazia
        for (int i = 0; i < MAX_CORES; i++) {
            if (active_threads[i] == NULL && !is_queue_empty(queues[i])) {
                // Põe o próximo da fila do núcleo para rodar
                Process* candidate = front(queues[i]);
                candidate->quantum = calculate_quantum(candidate, time);
                active_threads[i] = candidate;
                
                dequeue(queues[i]);
                running++;
            }
        }

        // Verifica se o quantum de alguma das threads rodando acabou
        for (int i = 0; i < MAX_CORES; i++) {
            if (!is_queue_empty(queues[i]) && active_threads[i] != NULL && active_threads[i]->quantum <= 0) {
                // Fim do quantum: processo rodando volta para a fila e o próximo da fila roda
                // Ocorrência de preempção
                enqueue(queues[i], active_threads[i]);
                Process* candidate = front(queues[i]);
                active_threads[i] = candidate;
                candidate->quantum = calculate_quantum(candidate, time);
                dequeue(queues[i]);
                preemptions++;
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
        
        time++; // Tempo que as threads gastaram acima

        // Caso algum processo tenha finalizado, o imprime na formatação correta
        pthread_mutex_lock(&lock);
        while (!is_queue_empty(finished_threads)) {
            Process* p = front(finished_threads);
            int tr = time - p->t0;
            int on_time = (time <= p->deadline);
            fprintf(file, "%s %d %d %d\n", p->name, tr, time, on_time);
            dequeue(finished_threads);
        }
        pthread_mutex_unlock(&lock);
        // Verifica se alguma das filas de pronto não está vazia
        all_queues_empty = 1;
        for (int i = 0; i < MAX_CORES; i++) {
            if (!is_queue_empty(queues[i]) || active_threads[i] != NULL) {
                all_queues_empty = 0;
                break;
            }
        }
    }

    // Imprime a quantidade de preempções ocorridas
    fprintf(file, "%d\n", preemptions);
}

/******************************************************************
*************************** FUNÇÃO MAIN **************************/
int main(int argc, char *argv[]) {

    // Descobre o numero de cores, e inicializa vetores que dependem disso
    MAX_CORES = sysconf(_SC_NPROCESSORS_ONLN);
    active_threads = malloc(MAX_CORES * sizeof(Process*)); // Ponteiros para as threads ativas de cada core
    cores = malloc(MAX_CORES * sizeof(cpu_set_t));         // Referencias aos cores
    core_dt = malloc(MAX_CORES * sizeof(int));             // Soma dos tempos restantes das threads de cada core
    finished_threads = new_queue(sizeof(Process*));        // Guarda threads que acabaram para serem imprimidas
    
    // Preenche os vetores corretamente
    for (int i = 0; i < MAX_CORES; i++) {
        active_threads[i] = NULL;
        core_dt[i] = 0;
        CPU_ZERO(&cores[i]);
        CPU_SET(i, &cores[i]);
    }
    
    // Erro (quantidade de argumentos errada)
    if (argc != 4) {
        printf("Uso correto: ep1 <numero do escalonador> <trace entrada> <trace saida>\n");
        return 1;
    }

    // Conta quantas linhas o arquivo de entrada tem
    FILE *file_in = fopen(argv[2], "r"); // argv[2] == arquivo de entrada
    int line_amount = count_lines(file_in);
    fseek(file_in, 0, SEEK_SET);

    // Reserva espaco para vetor de processos
    Process **processes = malloc(line_amount * sizeof(Process*));
    char *line = malloc(MAX_LINE_SIZE * sizeof(char));

    // Preenche vetor de processos lendo do arquivo
    int i = 0;
    while (fgets(line, MAX_LINE_SIZE, file_in)) {
        processes[i++] = line_to_process(line);
    }

    fclose(file_in); // Fecha o arquivo de entrada

    // Ordena os processos pela ordem do tempo de chegada
    qsort(processes, line_amount, sizeof(Process*), compare_by_t0);

    FILE *file_out = fopen(argv[3], "a"); // argv[3] == arquivo de saida

    // De acordo com o tipo de escalonador selecionado, inicia a execucao
    int scheduler_mode = atoi(argv[1]);
    switch (scheduler_mode) {
        case 1:
            fcfs(file_out, processes, line_amount);
            break;
        case 2:
            srtn(file_out, processes, line_amount);
            break;
        case 3:
            priority(file_out, processes, line_amount);
            break;
        default:
            printf("O primeiro argumento <numero do escalonador> deve ser 1, 2 ou 3\n");
            return 1;
    }

    fclose(file_out); // Fecha o arquivo de saída

    for (int i = 0; i < line_amount; i++) free(processes[i]);
    free(processes);
    free(active_threads);
    free(finished_threads);
    free(cores);
    free(core_dt);

    return 0;
}
