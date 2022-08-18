#include "array.h"
#include "multi-lookup.h"

int custom_dnslookup(void){
    usleep(rand() % 1000000);
    return 1;
}

void *request(void* data){
    request_data req_data = *(request_data*) data;
    int req_count = 0;
    while(1){
        if(array_size(req_data.filenames) > 0){ // get all files to be serviced
            char *filename;
            array_pop(req_data.filenames, &filename);
            
            FILE *file; 
            // skip file processing if file is invalid
            if((file = fopen(filename, "r")) == NULL || access(filename, F_OK | R_OK) == -1){
                fprintf(stderr, "Invalid file %s.\n", filename);
            }else{
                req_count++;

                char line[MAX_NAME_LENGTH];
                while(EOF != fscanf(file, "%s", line)){ // dissect hostnames from file
                    array_push(req_data.buffer, line);
                }
                fclose(file);
            }
            free(filename);
        }else{
            printf("thread %x serviced %d files\n", (int)pthread_self(), req_count);
            return NULL;
        }
    }
}

void *resolve(void* data){
    resolve_data res_data = *(resolve_data*) data;
    char *domain;
    char ip[MAX_IP_LENGTH];
    int res_count = 0;
    while(1){
        pthread_mutex_lock(res_data.j);
        if(!array_size(res_data.buffer)){
            pthread_mutex_unlock(res_data.j);

            pthread_mutex_lock(res_data.m);
            if(*res_data.req_done == 1){
                pthread_mutex_unlock(res_data.m);
                printf("thread %x resolved %d hostnames\n", (int)pthread_self(), res_count);
                return NULL;
            }
            pthread_mutex_unlock(res_data.m);
        }else{
            array_pop(res_data.buffer, &domain);
            pthread_mutex_unlock(res_data.j);
            // if(custom_dnslookup()==0){
            if(dnslookup(domain, ip, MAX_IP_LENGTH)==0){
                res_count++;
                fprintf(res_data.res_log, "%s, %s\n", domain, ip); // good ip
                fprintf(res_data.req_log, "%s\n", domain);
            }else{
                fprintf(res_data.res_log, "%s, NOT_RESOLVED\n", domain); // baad ip
            }
            free(domain);
        }
    }
}

int main(int argc, char *argv[]){
    struct timeval start, stop;
    gettimeofday(&start, NULL);

    pthread_mutex_t m; // conditional protection mutex
    if(pthread_mutex_init(&m, NULL)){
        printf("m mutex has failed\n");
        return -1;
    }

    pthread_mutex_t j; // conditional protection mutex
    if(pthread_mutex_init(&j, NULL)){
        printf("j mutex has failed\n");
        return -1;
    }
    
    // input checks
    if (argc <= 5 || argc > 6+MAX_INPUT_FILES){
        fprintf(stderr, "Incorrect format: <# requester> <# resolver> <requester log> <resolver log> [ <data file> ...]\n");
        exit(-1);
    }

    int n_req = atoi(argv[1]);
    if (n_req > MAX_REQUESTER_THREADS){
        fprintf(stderr, "Too many requester threads.\n");
        exit(-1);
    }else if (n_req < 1){
        fprintf(stderr, "Not enough requester threads.\n");
        exit(-1);
    }

    int n_res = atoi(argv[2]);
    if (n_res > MAX_RESOLVER_THREADS){
        fprintf(stderr, "Too many resolver threads.\n");
        exit(-1);
    }else if (n_res < 1){
        fprintf(stderr, "Not enough resolver threads.\n");
        exit(-1);
    }

    // open request and resolve log files
    FILE* req_log;
    FILE* res_log; 
    if((req_log = fopen(argv[3], "w+")) == NULL || access(argv[3], F_OK | W_OK) == -1){
        fprintf(stderr, "Invalid file %s.\n", argv[3]);
    }

    if((res_log = fopen(argv[4], "w+")) == NULL || access(argv[4], F_OK | W_OK) == -1){
        fprintf(stderr, "Invalid file %s.\n", argv[4]);
    }    

    // init files
    array *input_files = array_init(MAX_INPUT_FILES);
    for(int i=5; i<argc; i++){
        array_push(input_files, argv[i]);
    }
    // init shared_buffer
    array *shared_buffer = array_init(ARRAY_SIZE);

    // define thread arg structs
    int req_done = 0;
    request_data req_data;
    req_data.buffer = shared_buffer;
    req_data.filenames = input_files;

    resolve_data res_data;
    res_data.buffer = shared_buffer;
    res_data.res_log = res_log;
    res_data.req_log = req_log;
    res_data.req_done = &req_done;
    res_data.m = &m;
    res_data.j = &j;

    pthread_t req_thread[n_req];
    pthread_t res_thread[n_res];

    // create threads
    for(int i=0; i<n_req; i++){
        if(pthread_create(&req_thread[i], NULL, &request, &req_data) != 0){
            fprintf(stderr, "Error creating requester threads.\n");
            return -1;
        }
    }
    
    for(int i=0; i<n_res; i++){
        if(pthread_create(&res_thread[i], NULL, &resolve, &res_data) != 0){
            fprintf(stderr, "Error creating resolver threads.\n");
            return -1;
        }
    }

    // join requester threads
    for(int i=0; i<n_req; i++){
        if(pthread_join(req_thread[i], NULL) != 0){
            fprintf(stderr, "Error joining requester threads.\n");
            return -1;
        }
    }

    pthread_mutex_lock(&m); // protected boolean switch for when requesters finish
    req_done = 1;
    pthread_mutex_unlock(&m);

    // join resolver threads
    for(int i=0; i<n_res; i++){
        if(pthread_join(res_thread[i], NULL) != 0){
            fprintf(stderr, "Error joining resolver threads.\n");
            return -1;
        }
    }

    gettimeofday(&stop, NULL);
    double exec_time = (double)(stop.tv_usec-start.tv_usec)/1000000 + (double)(stop.tv_sec-start.tv_sec);
    printf("%s: total time is %f seconds\n", argv[0], exec_time);

    // close files
    fclose(req_log);
    fclose(res_log);
    // destroy and free
    pthread_mutex_destroy(&m);
    array_free(input_files);
    array_free(shared_buffer);

    return 0;
}