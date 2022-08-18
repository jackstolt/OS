#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

#include "array.h"
#include "util.h"

#define ARRAY_SIZE 8
#define MAX_INPUT_FILES 100
#define MAX_REQUESTER_THREADS 10
#define MAX_RESOLVER_THREADS 10
#define MAX_NAME_LENGTH 256
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

typedef struct req_data{
    array *buffer;
    array *filenames;
} request_data;

typedef struct res_data{
    array *buffer;
    FILE *res_log;
    FILE *req_log;
    int *req_done;
    pthread_mutex_t *m;
    pthread_mutex_t *j;
} resolve_data;

void *request(void* data);
void *resolve(void* data);
int custom_dnslookup(void);

#endif