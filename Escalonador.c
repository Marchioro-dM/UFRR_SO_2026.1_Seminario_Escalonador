#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX 100 //isso aqui é pro max fila

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

int f_vazia(Fila* f){
    return f->tamanho == 0;
}

int maior_prioridade(Fila* f){
    int melhor = 0;

    for (int i=1; i < f->tamanho; i++){
        if (f->itens[i]->prioridade < f->itens[melhor]->prioridade){
            melhor = i;
        }
    }
    return melhor;
}

Processo* escolher_processo(Fila* f){
    if (f_vazia(f)){
        return NULL;
    } else {
        int index_melhor = maior_prioridade(f);
        return remover_proc(f, index_melhor);
    }
}

void print_fila(Fila* f){ //função de checgem para testes
    if (!(f_vazia(f))){
        for (int i = 0; i < f->tamanho; i++){
            printf("P%d: prioridade = %d\n", f->itens[i]->pid, f->itens[i]->prioridade);
        }
    }
}


int main() {

    Fila fila;
    init_fila(&fila);

    Processo p1 = {1, 3, 5, 0, 5, 0, PRONTO};
    Processo p2 = {2, 1, 3, 0, 3, 0, PRONTO};
    Processo p3 = {3, 2, 4, 0, 4, 0, PRONTO};

    ins_processo(&fila, &p1);
    ins_processo(&fila, &p2);
    ins_processo(&fila, &p3);

    printf("Fila inicial:\n");
    print_fila(&fila);

    Processo* escolhido = escolher_processo(&fila);

    if (escolhido != NULL) {
        printf("\nProcesso escolhido: P%d (prioridade %d)\n",escolhido->pid, escolhido->prioridade);
    }

    printf("\nFila depois de remover o P2:\n");
    print_fila(&fila);

    return 0;
}