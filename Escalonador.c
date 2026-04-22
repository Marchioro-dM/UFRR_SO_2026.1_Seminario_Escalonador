#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

#define MAX 100 //isso aqui é pro max fila

#define QUANTUM 3 //Round robiinnn

int cpu_livre = 1;// Inicializa a CPU estando livre


typedef enum{
    PRONTO,
    EXECUTANDO,
    FINALIZADO
} Estado;


typedef struct Processo{ 
    int pid;
    int prioridade;
    int burst_time; //eles estavam aqui na tentativa de fazer um escalonador preemptivo mais robusto (deu errado)
    int io_time;    //não é usado também
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
        f->itens[i] = f->itens[i+1]; //joga todos os itens pra frente depois de mandar um pra CPU
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
        printf("FILA -");
        for (int i = 0; i < f->tamanho; i++){
            printf("[P%d, PR%d]", f->itens[i]->pid, f->itens[i]->prioridade);
        }
    }
    printf("\n");
}


void* run_processo(void* arg) {
    Processo* p = (Processo*) arg;

    while (1) { // Alterado para garantir que a thread não morra sem avisar o mutex

        pthread_mutex_lock(&mutex);

        // Se o tempo acabou, finaliza a thread corretamente liberando o mutex
        if (p->tempo_restante <= 0) {
            p->estado = FINALIZADO;
            pthread_mutex_unlock(&mutex);
            break; 
        }

        while (!p->pode_executar) {
            pthread_cond_wait(&cond, &mutex);
            // Re-checa se deve morrer enquanto esperava
            if (p->tempo_restante <= 0) {
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
        }

        pthread_mutex_unlock(&mutex);
        int tempo_exec = 0;

        // Loop de execução do Quantum
        while (tempo_exec < QUANTUM) { 
            pthread_mutex_lock(&mutex);
            if (p->tempo_restante <= 0) {
                pthread_mutex_unlock(&mutex);
                break;
            }
            printf("P%d executando| Pr:%d | Tempo restate:%d\n", p->pid,p->prioridade ,p->tempo_restante);
            p->tempo_restante--;
            pthread_mutex_unlock(&mutex);
            
            sleep(1);
            tempo_exec++;
        }

        pthread_mutex_lock(&mutex);
        p->pode_executar = 0;
        cpu_livre = 1;
        pthread_cond_broadcast(&cond); // Esse broadcast é pra acordar o escalonador denovo
        pthread_mutex_unlock(&mutex);
    }

    printf("[P%d] finalizado!\n", p->pid);
    return NULL;
}

void aplicar_aging(Fila *f) {
    for (int i = 0; i < f->tamanho; i++) {
        if(f->itens[i]->prioridade > 0){
            f->itens[i]->prioridade--; // Aumenta a prioridade de quem ta na lista de espera
        }
    }
}

Processo* cpu = NULL;

//MAIINNNNNNNNNNNNNNNNNNNN================================== pra nao se perder

int main() { //o main ta baiscamente funcionando como o Escalonador

    int quantum_count = 0; //bom ter de referencia, é usando pro aging

    Fila fila;
    init_fila(&fila);

    // Criando processos
    Processo p1 = {1, 4, 5, 0, 5, 0, PRONTO};
    Processo p2 = {2, 1, 3, 0, 7, 0, PRONTO};
    Processo p3 = {3, 6, 4, 0, 6, 0, PRONTO};
    Processo p4 = {4, 0, 3, 2, 10, 0, PRONTO};
    Processo p5 = {5, 3, 2, 1, 8, 0, PRONTO};

    // Inserindo na fila
    ins_processo(&fila, &p1);
    ins_processo(&fila, &p2);
    ins_processo(&fila, &p3);
    ins_processo(&fila, &p4);
    ins_processo(&fila, &p5);

    printf("Fila inicial:\n");
    print_fila(&fila);

    // Criando threads (processos)
    pthread_create(&p1.thread, NULL, run_processo, &p1);
    pthread_create(&p2.thread, NULL, run_processo, &p2);
    pthread_create(&p3.thread, NULL, run_processo, &p3);
    pthread_create(&p4.thread, NULL, run_processo, &p4);
    pthread_create(&p5.thread, NULL, run_processo, &p5);


    // Scheduler
    while (!f_vazia(&fila) || cpu != NULL) {

        pthread_mutex_lock(&mutex);

        // Se CPU ta livre, pega o próximo da fila
        if (cpu == NULL && !f_vazia(&fila)) {
            cpu = escolher_processo(&fila); //escolhe o processo com maior prioridade

            printf("\n[Scheduler] P%d esta usando a CPU\n", cpu->pid);

            cpu->estado = EXECUTANDO;
            cpu->pode_executar = 1;
            cpu_livre = 0;
            pthread_cond_broadcast(&cond); // Acorda a thread específica
        }
        
        // Espera o processo terminar seu turno ou finalizar
        while (cpu != NULL && cpu_livre == 0) {
            pthread_cond_wait(&cond, &mutex);
        }
        
        quantum_count++;
        if (quantum_count % 2 == 0){
            aplicar_aging(&fila); //a cada 2 quantums aumenta a prioridade dos itens na fila de espera
        }
        printf("\n%dº [QUANTUM finalizado]\n", quantum_count);

        if (cpu != NULL) {
            if (cpu->tempo_restante > 0) {
                cpu->estado = PRONTO;
                printf("[Scheduler] P%d voltou pra fila\n", cpu->pid);          
                ins_processo(&fila, cpu);
                print_fila(&fila);
            } else {
                cpu->estado = FINALIZADO;
                printf("[Scheduler] P%d finalizado\n", cpu->pid);
                pthread_cond_broadcast(&cond); // Acorda a thread para ela poder dar break
            }
            cpu = NULL; // libera CPU
            cpu_livre = 1;
        }
        
        pthread_mutex_unlock(&mutex);
    }

    // Espera todas as threads terminarem
    pthread_join(p1.thread, NULL);
    pthread_join(p2.thread, NULL);
    pthread_join(p3.thread, NULL);
    pthread_join(p4.thread, NULL);
    pthread_join(p5.thread, NULL);

    printf("\nTodos os processos finalizaram.\n");

    return 0;
}