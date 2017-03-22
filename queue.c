#include "queue.h"


//Rodrigo Luiz Kovalski 828130 2015

void queue_append (queue_t **queue, queue_t *elem){
    queue_t *aux;
    aux = *queue;
    if(elem == NULL){
        printf("\nERRO ELEMENTO NULO!!!\n");
    }
    else if(queue == NULL){
        printf("\nERRO FILA NAO EXISTE!!!\n");
    }
    else if(elem->next != NULL || elem->prev != NULL){
        //printf("\nERRO ELEMENTO PERTENCE A OUTRA FILA!!!\n");
    }
    //Adicionando o primeiro elemento
    else if(*queue == NULL){
        elem->next = elem;
        elem->prev = elem;
        *queue = elem;
    }//Adicionando mais elementos
    else {
        elem->next = aux;
        elem->prev = aux->prev;
        aux->prev->next = elem;
        aux->prev = elem;
        *queue = aux;
    }
}

queue_t *queue_remove (queue_t **queue, queue_t *elem){
    queue_t *ini = *queue;
    queue_t *aux = *queue;
    if(queue == NULL){
        printf("\nERRO FILA NAO EXISTE!!!\n");
        return queue;
    }
    else if (aux == NULL){
        printf("\nERRO FILA VAZIA!!!\n");
        return queue;
    }
    else if(elem == NULL){
        printf("\nERRO ELEMENTO NULO!!!\n");
        return queue;
    }
    else {//Se for um unico elemento
        if(queue_size(*queue) == 1 && *queue == elem){
            *queue = NULL;
            aux->next = NULL;
            aux->prev = NULL;
            return aux;
        }else{
            do{
                if(aux == elem){
                    aux->next->prev = aux->prev;
                    aux->prev->next = aux->next;
                    //Ajuste para remoção do primeiro elemento
                    if(aux == *queue){
                        *queue = aux->next;
                    }

                    aux->next = NULL;
                    aux->prev = NULL;
                    return aux;
                }
                aux = aux->next;
            }while(aux != ini);

        }
    }

    return queue;
}

int queue_size (queue_t *queue){
    int i = 1;
    queue_t *aux;
    aux = queue;
    if(queue == NULL){
        return 0;
    }//Unico elemento
    if((queue->next == queue) || (queue->prev == queue)){
        return 1;
    }
    while (queue->next != aux){
        if(queue->next == NULL){
            printf("\nERRO FILA MAL FORMADA!!!\n");
            return 0;
        }
        i++;
        queue = queue->next;
    }
    return i;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*)){

    if(queue == NULL || queue->next == NULL){
        printf("\nERRO FILA NAO EXISTE OU FILA MAL FORMADA!!!\n");
        return queue;
    }
    queue_t *ini = queue;
    queue_t *aux = queue;

    printf("%s: [", name);

    if(queue != NULL){
        do{
            print_elem(aux);
            printf(" ");
            aux = aux->next;
            if(queue->next == NULL){
            printf("\nERRO FILA MAL FORMADA!!!\n");
            break;
        }
        }while(aux->next != ini);
        print_elem(aux);
    }

    printf("]\n");
}

