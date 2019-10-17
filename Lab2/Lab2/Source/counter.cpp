// Derek Prince
// ECEN - 5033 Concurrent Programming
// Lab 2 - Various Locks and Barriers used on a counter microbench
// Due 10/16/19

#include <atomic>
#include <exception>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <set>
#include <string>

#include "include/barriers.hpp"
#include "include/cxxopts.hpp"
#include "include/locks.hpp"

#define COUNT_TYPE_UNSET 0
#define COUNT_BARRIER 1
#define COUNT_LOCK 2

static pthread_t* threads;
static int NUM_THREADS = 0;
static int iterations = 0;
static int cntr = 0;
static struct timespec start, end;

static std::atomic<int> threads_active;

static LockBox* lockBox = nullptr;
static LockType ltype = LOCK_TYPE_INVALID;

static BarrierBox* barrierBox = nullptr;
static BarrierType btype = BARRIER_TYPE_INVALID;

void global_init(){
    threads = static_cast<pthread_t*>(malloc(NUM_THREADS * sizeof(pthread_t)));
    threads_active.store(0);

}

void locks_init()
{
    // ltype MUST be set before calling.
    // Otherwise it will do nothing.
    lockBox = new LockBox(ltype);
}
void barrier_init(){
    barrierBox = new BarrierBox(btype, NUM_THREADS);
}

void locks_cleanup() { delete lockBox; }
void barrier_cleanup() { delete barrierBox; }
void global_cleanup(){ free(threads); }

void* lock_counter(void* thread_id){
    thread_local int tid = threads_active.fetch_add(1);
    thread_local int i = 0;

    for (i = 0; i < iterations; i++){
        if (i%NUM_THREADS == tid){
            lockBox->acquire();
            cntr++;
            lockBox->release();
        }
    }
    pthread_exit(nullptr);
}

void* barrier_counter(void* thread_id){
    thread_local int tid = threads_active.fetch_add(1);
    thread_local int i = 0;

    for (i = 0; i < iterations; i++){
        if (i%NUM_THREADS == tid){
            cntr++;
        }
        barrierBox->wait();
    }
    pthread_exit(nullptr);
}

int main(int argc, char* argv[]){
    cxxopts::Options options(argv[0], "Counter micro-benchmark for lab 2.");

    options.add_options()(
        "n, name", "My name (for grading purposes)", cxxopts::value<bool>())(
        "o, output", "Output file name", cxxopts::value<std::string>(), "FILE")(
        "t, threads", "Number of threads to be used", cxxopts::value<int>(), "NUM_THREADS")(
        "i, iter", "Number each thread iterates over", cxxopts::value<int>(), "Iterations desired")(
        "b, bar", "Selects the barrier type to use.", cxxopts::value<std::string>(), "<sense, pthread>")(
        "l, lock", "Selects the lock type to use.", cxxopts::value<std::string>(), " <tas, ttas, ticket, mcs, pthread, aflag>")(
        "h, help", "Display help options");

    std::string ofile = "";

    // --- Opt parse block ---
    int count_type = COUNT_TYPE_UNSET;
    try {
        options.parse_positional({ "input" });
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help({ "", "Group" }) << std::endl;
            return 0;
        }

        if (result.count("name")) {
            std::cout << "Derek Prince" << std::endl;
            return 0;
        }

        // Output file name
        if (result.count("o")) {
            ofile = result["o"].as<std::string>();
        } else { // default name if none given
            printf("No output file name given, default output.txt will be generated.\n");
            ofile = "output.txt";
        }

        // bar or lock
        if (result.count("bar") + result.count("lock") >= 2){
            printf("Only one option total between --bar and --lock allowed\n");
            return 0;
        }

        if (result.count("bar")) {
            count_type = COUNT_BARRIER;
            std::string bartype;
            bartype = result["bar"].as<std::string>();
            if (bartype == "sense"){
                btype = BARRIER_TYPE_SENSE;
            } else if (bartype == "pthread"){
                btype = BARRIER_TYPE_STD;
            } else {
                printf("No recognized or supported lock type found. Try lowercase or input supported lock type.\nRun with --help for a list of options\n");
                return 0;
            }
        }
        else if (result.count("lock")) {
            count_type = COUNT_LOCK;
            // I hate this format.
            std::string locktype;
            locktype = result["lock"].as<std::string>();
            if (locktype == "tas") {
                ltype = LOCK_TYPE_TAS;
            } else if (locktype == "ttas") {
                ltype = LOCK_TYPE_TTAS;
            } else if (locktype == "ticket") {
                ltype = LOCK_TYPE_TICKET;
            } else if (locktype == "mcs") {
                ltype = LOCK_TYPE_MCS;
            } else if (locktype == "pthread") {
                ltype = LOCK_TYPE_PTHREAD;
            } else if (locktype == "aflag") {
                ltype = LOCK_TYPE_AFLAG;
            } else {
                printf("No recognized or supported lock type found. Try lowercase or input supported lock type.\nRun with --help for a list of options\n");
                return 0;
            }
        } else {
            printf("Please supply a lock or barrier type to be used.\nLock values are: <tas, ttas, ticket, mcs, pthread, aflag>\nBarrier values are: <sense, pthread>\n");
            return 0;
        }

        if (result.count("iter")){
            iterations = result["iter"].as<int>();
        } else {
            printf("No iterations given.\nRun --help for input flags.\n");
        }

        // Extract thread number information
        if (result.count("threads")) {
            NUM_THREADS = result["t"].as<int>();
            if (NUM_THREADS > 15){//temp
                NUM_THREADS = 10;
            }
            if (NUM_THREADS > 150) {
                printf("Error: Too many threads\n");
                return (-1);
            }
        } else
            NUM_THREADS = 5;
    }
    // Catch unknown options and close nicely
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        return -1;
    }
    // -- END Opt parse block ---

    global_init();
    // --- Set up algorithm and launch threads ---
    switch (count_type){
    case COUNT_LOCK:{
        locks_init();
        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < NUM_THREADS; i++){
            pthread_create(&threads[i], nullptr, &lock_counter, &i);
        }
        for (int i = 0; i < NUM_THREADS; i++){
            pthread_join(threads[i], nullptr);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        locks_cleanup();
        break;
    }
    case COUNT_BARRIER:{
        barrier_init();
        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < NUM_THREADS; i++){
            pthread_create(&threads[i], nullptr, &barrier_counter, &i);
        }
        for (int i = 0; i < NUM_THREADS; i++){
            pthread_join(threads[i], nullptr);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        barrier_cleanup();
        break;
    }
    case COUNT_TYPE_UNSET:
    default:
        printf("No counter sync method selected.\nI\'m surprised the program got here.");
        return 0;
    }
    global_cleanup();


    // Print time
    unsigned long long elapsed_ns;
    elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
    printf("Elapsed (ns): %llu\n", elapsed_ns);
    double elapsed_s = static_cast<double>(elapsed_ns) / 1000000000.0;
    printf("Elapsed (s): %lf\n", elapsed_s);

    // --- Write data ---
    std::ofstream outfile(ofile);

    if (outfile.is_open()) {
        outfile << cntr << std::endl;
        std::cout << cntr << std::endl; // comment to remove/add terminal print
    }
    return 0;
}
