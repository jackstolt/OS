#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"

array *array_init(int size) {
    array *s = (array*) malloc(sizeof(array) + sizeof(char *[size*MAX_NAME_LENGTH]));
    s->top = -1;
    s->size = size;
    if(pthread_mutex_init(&s->mutex, NULL)){
        printf("mutex has failed\n");
        exit(-1);
    }
    sem_init(&s->space, 0, s->size);
    sem_init(&s->items, 0, 0);

    return s;
}

int array_push(array *s, char *element){ // double check semantics
    sem_wait(&s->space);
    pthread_mutex_lock(&s->mutex);
    
    s->top++;
    memcpy(s->array[s->top], element, MAX_NAME_LENGTH);
    s->array[s->top][MAX_NAME_LENGTH-1] = '\0'; // handle max name len hostnames

    pthread_mutex_unlock(&s->mutex);
    sem_post(&s->items);

    return 0;
}

int array_pop(array *s, char **element){
    sem_wait(&s->items);
    pthread_mutex_lock(&s->mutex);
    
    *element = (char*) malloc(MAX_NAME_LENGTH);
    memcpy(*element, s->array[s->top], MAX_NAME_LENGTH);
    s->top--;
    
    pthread_mutex_unlock(&s->mutex);
    sem_post(&s->space);
    
    return 0;
}

int array_size(array *s){
    pthread_mutex_lock(&s->mutex);
    int size = s->top+1;
    pthread_mutex_unlock(&s->mutex);
    return size;
}

void array_free(array *s){
    pthread_mutex_destroy(&s->mutex);
    sem_destroy(&s->space);
    sem_destroy(&s->items);
    free(s);
    s = NULL;
}