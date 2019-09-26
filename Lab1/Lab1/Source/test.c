#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


pthread_t* threads;
size_t* args;
size_t NUM_THREADS;
pthread_barrier_t bar;

struct timespec start, end;

// Global to avoid passing thread references all over the place
void global_init(){
    threads = malloc(NUM_THREADS*sizeof(pthread_t));
    args = malloc(NUM_THREADS*sizeof(size_t));
    pthread_barrier_init(&bar, NULL, NUM_THREADS);
}

void global_cleanup(){
    free(threads);
    free(args);
    pthread_barrier_destroy(&bar);
}

void local_init(){}
void local_cleanup(){}

void* thread_main(void* args){
    size_t tid = *((size_t*)args);
    local_init();
    pthread_barrier_wait(&bar);
    if(tid==1){
        clock_gettime(CLOCK_MONOTONIC,&start);
    }
    pthread_barrier_wait(&bar);

    // do something
    printf("Thread %zu reporting for duty\n",tid);

    pthread_barrier_wait(&bar);
    if(tid==1){
        clock_gettime(CLOCK_MONOTONIC,&end);
    }
    local_cleanup();

    return 0;
}


// === MAIN ===
int main(int argc, const char* argv[]){

    // parse args
    if(argc==2){
        NUM_THREADS = atoi( argv[1] );
        if(NUM_THREADS > 150){
            printf("ERROR; too many threads\n");
            exit(-1);
        }
    }
    else{
        NUM_THREADS = 5;
    }


    global_init();

    // launch threads
    int ret; size_t i;
  for(i=1; i<NUM_THREADS; i++){
        args[i]=i+1;
        printf("creating thread %zu\n",args[i]);
        ret = pthread_create(&threads[i], NULL, &thread_main, &args[i]);
        if(ret){
            printf("ERROR; pthread_create: %d\n", ret);
            exit(-1);
        }
    }
    i = 1;
    thread_main(&i); // master also calls thread_main

    // join threads
    for(size_t i=1; i<NUM_THREADS; i++){
        ret = pthread_join(threads[i],NULL);
        if(ret){
            printf("ERROR; pthread_join: %d\n", ret);
            exit(-1);
        }
        printf("joined thread %zu\n",i+1);
    }

    global_cleanup();

    unsigned long long elapsed_ns;
    elapsed_ns = (end.tv_sec-start.tv_sec)*1000000000 + (end.tv_nsec-start.tv_nsec);
    printf("Elapsed (ns): %llu\n",elapsed_ns);
    double elapsed_s = ((double)elapsed_ns)/1000000000.0;
    printf("Elapsed (s): %lf\n",elapsed_s);
}
