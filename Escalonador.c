#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

#define MAX 100 //isso aqui é pro max fila
#define QUANTUM 5 //Round robiinnn

typedef enum{
    PRONTO,
    EXECUTANDO,
    BLOQUEADO,
    FINALIZADO
} Estado;


typedef struct Processo{ 
    int pid;
    int prioridade;
    int burst_time;
    int io_time;
    int tempo_restante;
    int pode_executar;
    Estado estado;
    pthread_t thread;

} Processo;

typedef struct { //fila de processos
    Processo* itens[MAX];
    int tamanho;
} Fila;

//Aqui em cima ta as definições baisca de tudo
//=======Funcções básicas pra mexer na fila de processos=================

void init_fila(Fila *f){
    f->tamanho = 0;
}

void ins_processo(Fila* f, Processo* p){
    if (f->tamanho < MAX){
        f->itens[f->tamanho++] = p;
    }
}

Processo* remover_proc(Fila* f, int remov){
    Processo *p = f->itens[remov];

    for (int i = remov; i < f->tamanho-1; i++){
        f->itens[i] = f->itens[i+1]; //joga todos os itens pra "frente" depois de remover 1
    }
    f->tamanho--;
    return p;
}

int f_vazia(Fila* f){ //ve se a fila de execução ta vazia
    return f->tamanho == 0;
}

int maior_prioridade(Fila* f){  //procura e da return no processo com maior prioridade
    int melhor = 0;

    for (int i=1; i < f->tamanho; i++){
        if (f->itens[i]->prioridade < f->itens[melhor]->prioridade){
            melhor = i;
        }
    }
    return melhor;
}

Processo* escolher_processo(Fila* f){ //envia o processo pra CPU e remove ele da fila
    if (f_vazia(f)){
        return NULL;
    } else {
        int index_melhor = maior_prioridade(f);
        return remover_proc(f, index_melhor);
    }
}



void print_fila(Fila* f){ //função de printagem da fila
    if (!(f_vazia(f))){
        for (int i = 0; i < f->tamanho; i++){
            printf("P%d: prioridade = %d\n", f->itens[i]->pid, f->itens[i]->prioridade);
        }
    }
}


void* run_processo(void* arg) {
    Processo* p = (Processo*) arg;

    while (p->tempo_restante > 0) {

        pthread_mutex_lock(&mutex);

        while (!p->pode_executar) {
            pthread_cond_wait(&cond, &mutex);
        }

        int tempo_exec = 0;

        while (tempo_exec < QUANTUM && p->tempo_restante > 0) {
            printf("P%d executando\n", p->pid);

            p->tempo_restante--;
            tempo_exec++;

            sleep(1);
        }

        p->pode_executar = 0;

        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    p->estado = FINALIZADO;
    printf("P%d finalizado\n", p->pid);

    return NULL;
}

Processo* cpu = NULL;

//MAIINNNNNNNNNNNNNNNNNNNN================================== pra nao se perder

int main() { //o main ta baiscamente funcionando como a CPU

    Fila fila;
    init_fila(&fila);

    // Criando processos
    Processo p1 = {1, 3, 5, 0, 5, 0, PRONTO};
    Processo p2 = {2, 1, 3, 0, 3, 0, PRONTO};
    Processo p3 = {3, 2, 4, 0, 4, 0, PRONTO};

    // Inserindo na fila
    ins_processo(&fila, &p1);
    ins_processo(&fila, &p2);
    ins_processo(&fila, &p3);

    printf("Fila inicial:\n");
    print_fila(&fila);

    // Criando threads (processos)
    pthread_create(&p1.thread, NULL, run_processo, &p1);
    pthread_create(&p2.thread, NULL, run_processo, &p2);
    pthread_create(&p3.thread, NULL, run_processo, &p3);


    // Scheduler
    while (!f_vazia(&fila) || cpu != NULL) {

        pthread_mutex_lock(&mutex);

        // Se CPU ta livre, pega o próximo da fila
        if (cpu == NULL && !f_vazia(&fila)) {
            cpu = remover_proc(&fila, 0);

            printf("\n[Scheduler] CPU pegou P%d\n", cpu->pid);

            cpu->estado = EXECUTANDO;
            cpu->pode_executar = 1;
        }

        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);

        //Deixa o processo rodar durante o quantum
        sleep(QUANTUM);

        pthread_mutex_lock(&mutex);

        if (cpu != NULL) {

            if (cpu->tempo_restante > 0) {
                cpu->estado = PRONTO;

                printf("[Scheduler] P%d voltou pra fila\n", cpu->pid);

                ins_processo(&fila, cpu);
            } else {
                cpu->estado = FINALIZADO;
                printf("[Scheduler] P%d finalizado\n", cpu->pid);
            }

            cpu = NULL; // libera CPU
        }

        pthread_mutex_unlock(&mutex);
    }

    // Espera todas as threads terminarem
    pthread_join(p1.thread, NULL);
    pthread_join(p2.thread, NULL);
    pthread_join(p3.thread, NULL);

    printf("\nTodos os processos finalizaram.\n");

    return 0;
}