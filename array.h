#ifndef STACK_H
#define STACK_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define ARRAY_SIZE 8
#define MAX_NAME_LENGTH 256

typedef struct {
	int top;
	int size;
	sem_t space;
	sem_t items;
	pthread_mutex_t mutex;
	char array[][MAX_NAME_LENGTH];
} array;

array *array_init(int size);
int array_push(array *s, char *element);
int array_pop(array *s, char **element);
int array_size(array *s);
void array_free(array *s);

#endif