#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX = 100; //isso aqui é pra fila

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

typedef struct {

} Fila;


int main(){

    return 0;
}