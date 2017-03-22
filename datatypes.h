// PingPongOS - PingPong Operating System
// Rodrigo Luiz Kovalski 828130
//
//
// Estruturas de dados internas do sistema operacional

#ifndef __DATATYPES__
#define __DATATYPES__

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>

#include "queue.h"

#define STACKSIZE 32768
#define DEFAULT_PRIO 0
#define MIN_PRIO -20
#define MAX_PRIO 20
#define AGING -1
#define QUANTUM 20

//#define DEBUG
#define DEBUG2

// Estrutura que define uma tarefa
typedef struct task_t{
    struct task_t *prev, *next;// para usar com a biblioteca de filas (cast)
    int tid;// ID da tarefa 0 reservado para a Main e 1 para o despachante
    int op;//ID da operacao da tarefa 0 = nova 1 = executando 2 = suspensa 3 = terminada
    ucontext_t context;//Contexto da tarefa
    int chamadas;//Conta quantas vezes foi chamada
    int pe;//Prio estatica
    int pd;//prio dinamica
    int quantum;//fatia de tempo de processador
    int ini_time;//tempo que foi criada
    int cpu_time;//Contabiliza tempo de execução
    int wait_code;//Codigo que libera a tarefa suspensa
    int exit_code;//Codigo de saida
    int sleep_time;//Tempo que vai ficar dormindo
} task_t;

/* Composicao do ucontext_t
ucontext_t *uc_link     ponteiro para o contexto que será retomado quando retorna neste contexto
sigset_t    uc_sigmask  o conjunto de sinais que são bloqueados quando este contexto é ativo
stack_t     uc_stack    a pilha usada por este contexto
mcontext_t  uc_mcontext uma representação específica do computador para o contexto salvo.
*/

//Tarefa principal do SO
task_t Main;
//Ponteiro para a tarefa atual
task_t *Atual;
//Dispachante
task_t Dispatcher;
//Filas prontas, suspensas e adormecidas
task_t Queue, Queue_suspend, Queue_sleep;
//Ultima tarefa a ser executada
task_t *Ultima;
//Guarda endereco da fila de tarefas suspensas
task_t *Suspensa;
//Guarda endereco da fila de tarefas adormecidas
task_t *Adormecida;
//Struct do temporizador
struct itimerval timer;
//Struct da ação do tratador
struct sigaction action;
// estrutura que define um semáforo
typedef struct
{
    int sid;//Id do semaforo
    int value;//valor inicial de itens disponiveis
    int tasks;//Quantas tarefas estão no semaforo
    task_t Queue;//Fila vazia
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  int bid;// Id da brreira
  int tasks;// Quantidade atual de tarefas que esta barrando
  int limite;// Limite determinado para liberar as tarefas
  task_t Queue;//Fila vazia
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

//Funcao do despachante
void dispatcher_body ();
//funcao escalonador
task_t* scheduler();

//Tratador de sinais
void tratador();
//Funcao para suspender uma tarefa por tempo
void task_suspend_sleep (task_t *task);
//Funcao auxiliar para o despachamente verificar e acordar tarefas suspensas por tempo
void task_awake();


#endif
