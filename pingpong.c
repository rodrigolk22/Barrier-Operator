// PingPongOS - PingPong Operating System
// Rodrigo Luiz Kovalski 828130
//
//
// Definicoes internas do sistema operacional

#include "pingpong.h"

#ifndef PINPONG_INCLUDED
#define PINPONG_INCLUDED



//Indice geral das funcoes
int index = 0;
int userTasks = 0;
int tempoTotal = 0;
int semId = 1;//Id do semaforo


// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void pingpong_init (){
    #ifdef DEBUG
    printf ("\npingpong_init: Iniciando o sistema\n") ;
    #endif
    //Aponta a tarefa corrente pra Main
    Atual = &Main;
    //Cria o Main
    task_create(&Main, NULL, "Main");
    Main.op = 1;//Main esta ativa
    userTasks++;//Main sera tratada como uma tarefa de usuário
    #ifdef DEBUG
    printf ("\n    pingpong_init: Criou a Main\n") ;
    #endif
    //Cria o despachante
    task_create(&Dispatcher, (void*)(*dispatcher_body), "Dispatcher");
    #ifdef DEBUG
    printf ("\n    pingpong_init: Criou o despachante\n") ;
    #endif
    task_yield();
    // define a ação para o sinal de timer
    action.sa_handler = tratador ;
    sigemptyset (&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction (SIGALRM, &action, 0) < 0){
       perror ("Erro em signal: ") ;
       exit (1);
    }
    //ajusta valores do temporizador
    timer.it_value.tv_usec = 1000;      // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0;      // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0;   // disparos subsequentes, em segundos
    //arma o temporizador ITIMER_REAL
    if (setitimer (ITIMER_REAL, &timer, 0) < 0){
       perror ("Erro em setitimer: ");
       exit (1);
    }
    #ifdef DEBUG
    printf ("\n    pingpong_init: Criou o temporizador\n") ;
    #endif
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);

    #ifdef DEBUG
    printf ("\npingpong_init: sistema inicializado\n") ;
    #endif
}

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task, void (*start_func)(void *), void *arg){
    #ifdef DEBUG
    printf ("\ntask_create\n") ;
    #endif
    if(task == NULL){
        printf("\nErro!! tarefa nula!\n");
        return -1;
    }
    if((void*)(*start_func) == NULL && arg != "Main"){
        printf("\nAviso!! Parametro da funcao esta faltando!\n");

    }
    //A tarefa Main nao muda de contexto
    if(task != &Main){
        //Cria ponteiro para o inicializar contexto atual
        char *stack;
        //Pega e cria o contexto (extraido do arquivo ucontexts.c)
        getcontext (&task->context);
        stack = malloc (STACKSIZE);
        if (stack){
            task->context.uc_stack.ss_sp = stack ;
            task->context.uc_stack.ss_size = STACKSIZE;
            task->context.uc_stack.ss_flags = 0;
            task->context.uc_link = 0;
        }else{
            perror ("Erro na criacao da pilha: ");
            exit (1);
        }
        makecontext (&task->context, (void*)(*start_func), 1, (char*)arg);
    }
    //Adiciona o id e a operacao
    task->tid = index;
    task->op = 0;//0 - pronta
    //Aumenta o indice geral
    index++;
    //Coloca a prioridade como default
    task->pe = DEFAULT_PRIO;
    task->pd = DEFAULT_PRIO;
    //Iniciando valores
    task->cpu_time = 0;
    task->chamadas = 0;
    //Colocando o quantum
    task->quantum = QUANTUM;
    task->ini_time = systime();
    //Codigo de suspensao (nulo)
    task->wait_code = -1;
    //Colocando o tarefa na fila
    queue_append((queue_t**)&Queue, (queue_t*)task);
    //Adicionando contador de tarefas a fazer
    if(task->tid != 1){//Task Dispatcher nao conta como tarefa de usuario
        userTasks ++;
    }
    #ifdef DEBUG
    printf ("\n   task_create: criou tarefa %d\n", task->tid) ;
    #endif
    //Retorna o id a tarefa criada
    return task->tid;
}

// Transfere o processador para outra tarefa
int task_switch (task_t *task){
    //Guarda o valor do retorno da swapcontext
    int retorno;
    //Guarda op ponteiro da antiga
    task_t *antiga = Atual;
    //Atualiza a operacao das tarefas
    if(antiga->op == 1){
        antiga->op = 0;//pronta
    }
    task->op = 1;
    //Aponta a tarefa recebida como atual
    Atual = task;
    //Contabiliza cada vez que é chamada
    task->chamadas++;
    #ifdef DEBUG
    printf ("\ntask_switch: trocando contexto %d -> %d \n", antiga->tid, task->tid) ;
    #endif
    //Troca de contexto
    retorno = swapcontext (&antiga->context, &task->context);
    //valor negativo se houver erro, ou zero
    return retorno;
}

// Termina a tarefa corrente
void task_exit (int exitCode){
    #ifdef DEBUG
    printf ("\ntask_exit: tarefa %d sendo encerrada \n", Atual->tid) ;
    #endif
    Atual->op = 3;//Terminada
    //Se tiver tarefas a fazer, vai para o despachante
    if(userTasks > 0){
        //Atualiza o contador de tarefas a fazer
        userTasks --;
        Atual->exit_code = exitCode;
        //Remove a tarefa terminada da fila
        queue_remove((queue_t**)&Queue, (queue_t*)Atual);
        printf("Task %d exit: running time %d ms, processor time %d ms, %d activations\n", Atual->tid, (systime() - Atual->ini_time), Atual->cpu_time, Atual->chamadas);
        //Acorda as tarefas suspensas
        task_resume(Atual);
        //Encaminha ao despachante
        task_yield();
    }
    else{
        printf("Task %d exit: running time %d ms, processor time %d ms, %d activations\n", Dispatcher.tid, (systime() - Dispatcher.ini_time), Dispatcher.cpu_time, Dispatcher.chamadas);
        #ifdef DEBUG
        printf("task_exit: Fim das tarefas escalonador encerrado\n");
        #endif
    }
}

// Informa o identificador da tarefa corrente
int task_id (){
    #ifdef DEBUG
    printf ("\ntask_id: ID da tarefa atual %d \n", Atual->tid) ;
    #endif
    return Atual->tid;
}
//Devolve o processador ao dispatcher
void task_yield () {
    #ifdef DEBUG
    printf ("\nTask yeld\n") ;
    #endif
    //Guarda a ultima tarefa a ser executada
    Ultima = Atual;
    task_switch(&Dispatcher);
}
//Funcao do despachante
void dispatcher_body (){
    #ifdef DEBUG
    printf("\n\nDispatcher\n");
    #endif
    //Guarda a proxima tarefa
    task_t *next;
    //Se houver tarefas do usuario gerencia elas
    while(userTasks > 1){
        #ifdef DEBUG
        printf("\n   dispatcher: solicitando o escalonador\n");
        #endif
        //Solicita a proxima tarefa ao escalonador
        next = scheduler();
        if(next != NULL){
            //Ajusta a posicao na fila
            queue_remove((queue_t**)&Queue, (queue_t*)next);
            queue_append((queue_t**)&Queue, (queue_t*)next);
            //So manda para a tarefa se estiver pronta
            if(next->op == 0){
                task_switch (next);
            }
            //Verifica tarefas prontas pra "acordar"
            task_awake();
        }
    }
    #ifdef DEBUG
    printf("\n   dispatcher: acabou as tarefas de usuario\n");
    #endif
    //Funcao ao fim apenas quando nao ha userTasks
    task_exit(0); // encerra a tarefa dispatcher
}
/*
//Escalonador simples
task_t* scheduler(){
    return Dispatcher.next;
}
*/

//Escalonador Prio com Aging a = -1
task_t* scheduler(){
    #ifdef DEBUG
    printf ("\nscheduler\n");
    printf ("\n    scheduler: Saindo da tarefa %d prio %d\n\n", Ultima->tid, Ultima->pd);
    #endif
    #ifdef DEBUG
    printf ("\nID - Prio\n\n");
    #endif
    //Procura a menor prioridade
    task_t *aux = Dispatcher.next;
    task_t *Proxima = Dispatcher.next;
    while(aux != &Dispatcher){
        if(aux->pd < Proxima->pd && aux->tid > 1){
            Proxima = aux;
        }
        #ifdef DEBUG
        printf ("%d - %d\n", aux->tid, aux->pd);
        #endif
        //Percorre todas as tarefas
        aux = aux->next;
    }
    //Envelhece todas menos as tarefas de usuario
    aux = Dispatcher.next;
    while(aux != &Dispatcher){
        if(aux->pd > MIN_PRIO && aux != Proxima && aux->tid > 1){
            aux->pd += AGING;
        }
        aux = aux->next;
    }
    //Arruma a prio dinamica para prio estatica da proxima tarefa
    Proxima->pd = Proxima->pe;
    #ifdef DEBUG
    printf ("\n  scheduler: Proxima tarefa %d prio %d\n\n", Proxima->tid, Proxima->pd);
    #endif
    return Proxima;

}

// suspende uma tarefa, retirando-a de sua fila atual, adicionando-a à fila
// queue e mudando seu estado para "suspensa"
void task_suspend (task_t *task, task_t **queue){
    #ifdef DEBUG2
    printf ("\n    task suspend id %d wait_code %d\n", task->tid, task->wait_code);
    #endif
    task_t *aux = task;
    //Se a task for o escalonador a funcao nao suspende
    if(task == &Dispatcher){
        task_yield();
    }
    //Se a task for NULL usa a tarefa atual
    if(&task == NULL){
        aux = Atual;
    }
    //Arruma a ordem da fila
    queue_remove((queue_t**)&Queue, (queue_t*)aux);
    queue_append(queue, (queue_t*)aux);
    //Marca como suspensa
    task->op = 2;
    //Guarda um ponteiro para as tarefas supensas
    Suspensa = task;
    //Vai para o escalonador;
    task_yield();
}

// acorda uma tarefa, retirando-a de sua fila atual, adicionando-a à fila de
// tarefas prontas ("ready queue") e mudando seu estado para "pronta"
void task_resume (task_t *task){
    #ifdef DEBUG
    printf ("\n\n     task resume: task id %d exit code %d\n", task->tid, task->exit_code);
    #endif
    if(Suspensa == NULL){
        #ifdef DEBUG
        printf ("\n         task resume: fila vazia - task id %d exit code %d\n", task->tid, task->exit_code);
        #endif
    }
    else{
        if(Suspensa == Suspensa->next && Suspensa->wait_code == task->tid){//Fila de um unico elemento
            //Muda para pronta
            Suspensa->op = 0;
            //Arruma a ordem da fila
            queue_remove((queue_t**)&Queue_suspend, (queue_t*)Suspensa);
            queue_append((queue_t**)&Queue, (queue_t*)Suspensa);
            #ifdef DEBUG
            printf ("\n         task resume: acordando id %d\n", Suspensa->tid);
            #endif
            Suspensa = NULL;

        }
        else{
        //Percorre todas as tarefas que estao suspensas
        task_t *aux = Suspensa->next;
        task_t *Proxima;
            do{
                //Guarda a posicao do auxilar
                Proxima = aux;
                //verifica se o codigo de espera bate com o id da tarefa encerrada
                if(aux->wait_code == task->tid){
                    Proxima = aux->next;
                    #ifdef DEBUG
                    printf ("\n         task resume: acordando id %d\n", aux->tid);
                    #endif
                    //Muda para pronta
                    aux->op = 0;
                    //Arruma a ordem da fila
                    queue_remove((queue_t**)&Queue_suspend, (queue_t*)aux);
                    queue_append((queue_t**)&Queue, (queue_t*)aux);
                    aux = Proxima;
                }
                else{//Percorre todas as tarefas
                    aux = aux->next;
                }
            }while(aux != Suspensa);
        }

    }
}
// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio){
    #ifdef DEBUG
    printf ("\n     task_getprio: Tarefa Atual %d prio %d\n", Atual->tid, Atual->pe) ;
    #endif
    if(prio >= MAX_PRIO || prio <= MIN_PRIO){
        perror("Prioridade invalida");
    }
    else if(task == NULL){
        Atual->pe = prio;
        Atual->pd = prio;
    }
    else{
        task->pe = prio;
        task->pd = prio;
    }
}

// retorna a prioridade dinamica da tarefa (ou a tarefa atual)
int task_getprio (task_t *task){
    #ifdef DEBUG
    printf ("\n     task_getprio: Tarefa Atual %d prio %d\n", Atual->tid, Atual->pe) ;
    #endif
    if(task == NULL){
        return Atual->pe;
    }
    return task->pe;
}

//tratador de sinais
void tratador(){
    #ifdef DEBUG
    printf ("\n\n   tratador: tarefa id %d\n", Atual->tid) ;
    #endif
    // incrementa o contador de tempo global
    tempoTotal++;
    Atual->cpu_time++;
    // decrementa o quantum da tarefa de usuario
    if(Atual->tid != 1){
        Atual->quantum--;
    }
    //Verifica o fim do quantum
    if(Atual->quantum <= 0 && Atual->tid != 1){
        Atual->quantum = QUANTUM;
        #ifdef DEBUG
        printf ("\n\n   tratador: yeld\n\n") ;
        #endif
        task_yield();
    }
    #ifdef DEBUG
    printf ("\n   tratador: saindo\n") ;
    #endif
}
//Retorna o tempo total de execução do programa em ms
unsigned int systime(){
    return tempoTotal;
}
//Executa a task requisitada e bloqueia a atual até ela terminar
int task_join (task_t *task){
    #ifdef DEBUG
    printf ("\n\n       task join: atual id %d task id %d\n\n", Atual->tid, task->tid) ;
    #endif
    //Caso a tarefa b não exista ou já tenha encerrado, esta chamada deve retornar imediatamente, sem suspender a tarefa corrente
    if(task == NULL || task->op == 3){
        task_yield();
        return -1;
    }
    Atual->wait_code = task->tid;
    //a tarefa atual (corrente) sera suspensa até a conclusão da tarefa task
    task_suspend(Atual, &Queue_suspend);

    return task->exit_code;

}
void task_sleep(int t){
    #ifdef DEBUG
    printf ("\n    task sleep: %d ms tempo id %d tempo %d s \n", systime(), Atual->tid, t);
    #endif
    if(t > 0){
        //Marca o tempo de espera;
        Atual->sleep_time = t;
        //Suspende a tarefa atual
        Adormecida = Atual;
        task_suspend_sleep(Atual);
    }
}
// suspende uma tarefa, retirando-a de sua fila atual, adicionando-a à fila
// queue e mudando seu estado para "suspensa"
void task_suspend_sleep (task_t *task){
    #ifdef DEBUG
    printf ("\n    task suspend sleep: %d ms tempo id %d tempo %d \n", systime(), task->tid, task->sleep_time);
    #endif
    task_t *aux = task;
    //Se a task for o escalonador a funcao nao suspende
    if(task == &Dispatcher){
        task_yield();
    }
    //Se a task for NULL usa a tarefa atual
    if(&task == NULL){
        aux = Atual;
    }
    //Arruma n ordem da fila
    queue_remove((queue_t**)&Queue, (queue_t*)aux);
    queue_append((queue_t**)&Queue_sleep ,(queue_t*)aux);
    //Marca como suspensa
    task->op = 2;
    //Guarda um ponteiro para as tarefas supensas
    Adormecida = task;
    //Vai para o escalonador;
    task_yield();
}
// acorda uma tarefa, retirando-a de sua fila atual, adicionando-a à fila de
// tarefas prontas ("ready queue") e mudando seu estado para "pronta"
void task_awake(){
    #ifdef DEBUG
    printf ("\n    task awake: checando tarefas adormecidas tempo %d\n\n", systime());
    #endif
    if(Adormecida == NULL){
        #ifdef DEBUG
        printf ("\n         task awake: fila vazia \n\n");
        #endif
    }
    else{
        if(Adormecida == Adormecida->next && Adormecida->sleep_time <= systime()){//Fila de um unico elemento
            //Muda para pronta
            Adormecida->op = 0;
            //Arruma a ordem da fila
            queue_remove((queue_t**)&Queue_sleep, (queue_t*)Adormecida);
            queue_append((queue_t**)&Queue, (queue_t*)Adormecida);
            #ifdef DEBUG
            printf ("\n         task awake: acordando id %d, alarme %d tempo sistema %d\n\n", Adormecida->tid, Adormecida->sleep_time, systime());
            #endif
            Adormecida = NULL;

        }
        else{
        //Percorre todas as tarefas que estao suspensas
        task_t *aux = Adormecida->next;
        task_t *Proxima;
            do{
                //Guarda a posicao do auxilar
                Proxima = aux;
                //verifica se o codigo de espera bate com o id da tarefa encerrada
                if(aux->sleep_time <= systime()){
                    Proxima = aux->next;
                    #ifdef DEBUG
                    printf ("\n         task awake: acordando id %d\n\n", aux->tid);
                    #endif
                    //Muda para pronta
                    aux->op = 0;
                    //Arruma a ordem da fila
                    queue_remove((queue_t**)&Queue_sleep, (queue_t*)aux);
                    queue_append((queue_t**)&Queue, (queue_t*)aux);
                    aux = Proxima;
                }
                else{//Percorre todas as tarefas
                    aux = aux->next;
                }
            }while(aux != Adormecida);
        }

    }
}
//cria um semáforo com valor inicial "value"
int sem_create (semaphore_t *s, int value){
    #ifdef DEBUG
    printf ("\n    sem_create id %d: valor %d\n", semId, value);
    #endif
    //Id
    s->sid = semId;
    semId++;
    //contador
    s->value = value;
    //Cria fila
    queue_append((queue_t**)&s->Queue, (queue_t*)&s->Queue);
    //Verifica existencia
    if(&s != NULL){
        return 0;
    }
    else{
        return -1;
    }
}

//requisita o semáforo
int sem_down (semaphore_t *s){
    if(!s){
        return -1;
    }
    else{
        task_t *aux = Atual;
        queue_t *fila = &s->Queue;
        if(s->value > 0){
            #ifdef DEBUG2
            printf ("\n    sem_down: tarefa id %d continua sem interrupcoes sem %d (%d)\n\n", Atual->tid, s->sid, s->value);
            #endif
            s->value--;
        }else{
            #ifdef DEBUG2
            printf ("\n    sem_down: tarefa id %d aguarda sem %d (%d)\n\n", Atual->tid,  s->sid, s->value);
            #endif
            //a tarefa corrente é suspensa
            queue_remove((queue_t**)&Queue, (queue_t*)aux);
            //Fila vazia
            if(&s->Queue == s->Queue.next){
                queue_append((queue_t**)&s->Queue, (queue_t*)aux);
                #ifdef DEBUG2
                printf("1 - Semaforo s (%d->) (%d->) %d (->%d) (->%d)\n", s->Queue.prev->prev->tid, s->Queue.prev->tid, s->sid, s->Queue.next->tid, s->Queue.next->next->tid);
                #endif
            }
            else {
                //Fila com membros
                fila = fila->next;
                aux->next = fila;
                aux->prev = fila->prev;
                fila->prev->next = aux;
                fila->prev = aux;
                #ifdef DEBUG2
                printf("2 - Semaforo s (%d->) (%d->) %d (->%d) (->%d)\n", s->Queue.prev->prev->tid, s->Queue.prev->tid, s->sid, s->Queue.next->tid, s->Queue.next->next->tid);
                #endif
            }
            //Volta ao Despachante
            task_yield();
        }
        return 0;
    }
}

// libera o semáforo
int sem_up (semaphore_t *s){
    //Ponteiro auxiliar
    task_t *aux;
    aux = s->Queue.next;
    if(!s){
        return -1;
    }
    else{
        #ifdef DEBUG2
        printf ("\n    sem_up: tarefa id %d libera um acesso ao sem %d (%d)\n\n", Atual->tid, s->sid, s->value + 1);
        #endif
        //Libera a tarefa suspensa
        if(aux != &s->Queue){
            //Tira o elemento mais antigo
            while(aux->next != &s->Queue){
                aux = aux->next;
            }
            #ifdef DEBUG2
            printf("\n    sem_up: tarefa %d vai ganhar o acesso\n", aux->tid);
            #endif
            queue_remove((queue_t**)&s->Queue, (queue_t*)aux);
            queue_append((queue_t**)&Queue, (queue_t*)aux);
            return 0;
        }else{
            s->value++;
        }

        return 0;
    }

}

// destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy (semaphore_t *s){
    #ifdef DEBUG
    printf ("\n    sem_destroy id %d\n\n", s->sid);
    #endif
    if(!s){
        return -1;
    }
    else {
        //Ponteiro auxiliar
        task_t *proximo;
        proximo = s->Queue.next;
        //esvazia o semaforo
        while(proximo != &s->Queue){
            //tira da fila do semaforo
            queue_remove((queue_t**)&s->Queue, (queue_t*)proximo);
            //Adiciona a fila de prontas
            queue_append((queue_t**)&Queue, (queue_t*)proximo);
            //Le a proxima tarefa
            proximo = s->Queue.next;
        }
        return 0;
    }

}
// Inicializa uma barreira
int barrier_create (barrier_t *b, int N){
    //Id
    b->bid = index;
    index++;
    //Tarefas inicial
    b->tasks = 0;
    //Tarefas limite
    b->limite = N;
    //Inicia fila
    queue_append((queue_t**)&b->Queue, (queue_t*)&b->Queue);
    //Verifica existencia
    if(&b != NULL){
        return 0;
    }
    else{
        return -1;
    }

}

// Chega a uma barreira
int barrier_join (barrier_t *b){
    //Verifica a existencia
    if(!b){
        return -1;
    }
    else{
        //Ponteiros auxiliares
        task_t *aux = Atual;
        queue_t *fila = &b->Queue;
        //Chegou uma tarefa
        b->tasks++;
        //Verifica se tem N tarefas na barreira
        if(b->tasks >= b->limite){
            #ifdef DEBUG2
            printf ("\n    barrier_join: tarefa id %d chegou, o limite da barreira %d foi alcançado (%d/%d)\n\n", Atual->tid, b->bid, b->tasks, b->limite);
            printf("A Barrerira sera reiniciada\n");
            #endif
            //Reinica a barreira
            aux = &b->Queue.next;
            while(aux != &b->Queue){
                //tira da fila da barreira
                queue_remove((queue_t**)&b->Queue, (queue_t*)aux);
                //Adiciona a fila de prontas
                queue_append((queue_t**)&Queue, (queue_t*)aux);
                //Le a proxima tarefa
                aux = b->Queue.next;
            }
            //Muda pra 0 o contador
            b->tasks = 0;

        }else{
            #ifdef DEBUG2
            printf ("\n    barrier_join: tarefa id %d chegou, barreira %d foi foi incrementada (%d/%d)\n\n", Atual->tid, b->bid, b->tasks, b->limite);
            #endif
            //a tarefa corrente é suspensa
            queue_remove((queue_t**)&Queue, (queue_t*)aux);
            //Fila vazia
            if(&b->Queue == b->Queue.next){
                queue_append((queue_t**)&b->Queue, (queue_t*)aux);
                #ifdef DEBUG2
                printf("1 - Barreira (%d->) (%d->) %d (->%d) (->%d)\n", b->Queue.prev->prev->tid, b->Queue.prev->tid, b->bid, b->Queue.next->tid, b->Queue.next->next->tid);
                #endif
            }
            else {
                //Fila com membros
                fila = fila->next;
                aux->next = fila;
                aux->prev = fila->prev;
                fila->prev->next = aux;
                fila->prev = aux;
                #ifdef DEBUG2
                printf("2 - Barreira (%d->) (%d->) %d (->%d) (->%d)\n", b->Queue.prev->prev->tid, b->Queue.prev->tid, b->bid, b->Queue.next->tid, b->Queue.next->next->tid);
                #endif
            }
            //Volta ao Despachante
            task_yield();
        }
        return 0;

    return 0;
    }
}

// Destrói uma barreira
int barrier_destroy (barrier_t *b){
    #ifdef DEBUG
    printf ("\n    barrier_destroy id %d\n\n", b->bid);
    #endif
    if(!b){
        return -1;
    }
    else {
        //Ponteiro auxiliar
        task_t *proximo;
        proximo = b->Queue.next;
        //Esvazia a barreira
        while(proximo != &b->Queue){
            //tira da fila da barreira
            queue_remove((queue_t**)&b->Queue, (queue_t*)proximo);
            //Adiciona a fila de prontas
            queue_append((queue_t**)&Queue, (queue_t*)proximo);
            //Le a proxima tarefa
            proximo = b->Queue.next;
        }

        return 0;
    }

}

#endif
