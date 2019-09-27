#include <iostream>
#include <pthread.h>


#define NUM_THREADS 10

static pthread_t* threads;
static int* args;
static pthread_barrier_t bar;

//struct timespec start, end;

void global_init(){
    args = static_cast<int*>(malloc(NUM_THREADS*sizeof(size_t)));

    threads = static_cast<pthread_t*>(malloc(NUM_THREADS*sizeof(pthread_t)));

    pthread_barrier_init(&bar, nullptr, NUM_THREADS);

}

void global_cleanup(){
    free(threads);
    free(args);
    pthread_barrier_destroy(&bar);
}

//void *recursivefn(void* tid){
//    int *thread_id;
//    thread_id = (int*)tid;
//    std::cout << "In thread: " << thread_id << std::endl;

//    pthread_exit(nullptr);
//}

// Should not work
void* test_plaunchfn(void* args){
    int *t = (int*)args;
    std::cout << "In plaunchfn. t: " << *t << std::endl;
    if (*t > 1){
        *t--;
        int temp = *t;
        test_plaunchfn(&temp);
    }
    pthread_exit(nullptr);
}

void test_launchfn(int t){
    std::cout << "testlaunch fn.\n";

    test_plaunchfn((void*)t);
}


int main()
{
    printf("Main begin\n");

    global_init();
    for (int i = 0; i < NUM_THREADS; i++){
        std::cout << "Creating thread: " << i << std::endl;
        pthread_create(&threads[i], nullptr, test_plaunchfn, (void*)i);
    }
    for (int i = 0; i < NUM_THREADS; i++){
        std::cout << "Joining thread: " << i << std::endl;
        pthread_join(threads[i], nullptr);
    }

    global_cleanup();
    return 0;
}
