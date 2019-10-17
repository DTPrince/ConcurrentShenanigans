// Derek Prince
// ECEN - 5033 Concurrent Programming
// Lab 2 - Various Locks and Barriers used on Bucketsrot
// Due 10/16/19

// test.txt -o output --alg=bucket -t 100 --lock=aflag

#include <atomic>
#include <exception>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <set>
#include <string>

#include "include/cxxopts.hpp"
#include "include/locks.hpp"

#define FORK_JOIN       1
#define BUCKET          0
#define ALG_UNDEFINED   10
    // arbitrary number. No inclined to use negative numbers at this time to
    // avoid potential unsigned/signed cross-pollination

// Played with this number a bit but it'll depend on the machine so I'm not too
// woried about the +/-
#define LIST_SZ_LIMIT 12

// Manages how many entries in origninal array a thread is responsible for
#define MIN_ELEMENTS_PER_THREAD 10

static pthread_t* threads;
static int NUM_THREADS = 0;
static struct timespec start, end;

static LockBox* lockBox = nullptr;
static LockType ltype = LOCK_TYPE_INVALID;

static std::multiset<int>* landfill = nullptr;

typedef struct {
    std::vector<int>* vec;
    int lo, hi, tid;
} thread_vec;

typedef struct {
    std::vector<int>* vec;
    std::multiset<int>* buckets; // looks gross tbh but it should abstract cleanly
    int lo, hi, tid;
} thread_bvec;

void global_init() { threads = static_cast<pthread_t*>(malloc(NUM_THREADS * sizeof(pthread_t))); }
void global_bs_init() { landfill = new std::multiset<int>(); }
void locks_init() { lockBox = new LockBox(ltype); }

void locks_cleanup() { delete lockBox; }
void global_cleanup() { free(threads); }
void global_bs_cleanup() { delete landfill; }

// semi-intelligently find the partition index to divvy up the work load
int partition(std::vector<int>& my_vec, int lo, int hi)
{
    // Median of 3 might be better for large data sets but rand modulus sounds
    // expensive
    int pivot = my_vec[hi];

    // pre-emptive decrement
    int i = lo - 1;

    // iterate over sub-array and swap if needed
    for (int j = lo; j < hi; j++) {
        if (my_vec[j] < pivot) {
            i++;
            std::swap(my_vec[i], my_vec[j]);
        }
    }
    std::swap(my_vec[i + 1], my_vec[hi]);

    /*
   * This partition function 'weighs' the vector to see how uneven or pre-sorted
   * the array is. As i only increments when there is a swap, it is a
   * semi-direct way of labeling how many times the array was swapped with
   * respect to the highest location (which will be the highest number in the
   * array after quicksort finishes) In other words, the highest position is
   * supposed to be the highest number so if i is still -1 by the time it
   * iterates over all of it then we know the upper location is already sorted
   * and pi = 0
   */
    return (i + 1);
}

// my_qsort from lab0
void my_quicksort(std::vector<int>& my_vec, int lo, int hi)
{
    // exit condition
    if (lo < hi) {
        // split array by oartitioning into two arrays, returning index (pi) of
        // middle
        int pi = partition(my_vec, lo, hi);

        // now that split has been found, quicksort the two partitions
        my_quicksort(my_vec, lo, pi - 1);
        my_quicksort(my_vec, pi + 1, hi);
    }
}

// Standard insertion sort to take care of remainder of array sorting when
// creating threads has too much overhead.
void my_insertionsort(std::vector<int>& vec, int sz)
{
    int i, key, j;
    for (i = 0; i < sz; i++) {
        // grab key to check against
        key = vec[i];

        j = i - 1; // like j = 0 but starts at ~i
        // iterate backwards while index value is greater than key
        while ((j >= 0) && (vec[j] > key)) {
            vec[j + 1] = vec[j];
            j--;
        }
        vec[j + 1] = key;
    }
}

// only spawn one new thread. There is no reason to spawn 2 as the main thread
// will be idle anyhow
// Dont make new threads once the list becomes small enough. It has too much
// overhead
void* pqsort(void* arg)
{
    // Master/managing thread
    thread_vec* t_vec = static_cast<thread_vec*>(arg);
    if (t_vec->tid == 0) {
        clock_gettime(CLOCK_MONOTONIC, &start);
    }

    if (t_vec->lo < t_vec->hi) {
        if ((t_vec->hi - t_vec->lo) < LIST_SZ_LIMIT) {
            // sort with straight-forward method and smaller overhead
            my_insertionsort(*t_vec->vec, t_vec->vec->size());
        } else {
            if (t_vec->tid == NUM_THREADS) {
                // Sort in a single thread.
                // Possibility of checking for old, completed threads here but... Eh. So
                // much could go wrong and this isn't the design for it
                my_quicksort(*t_vec->vec, t_vec->lo, t_vec->hi);
            } else {
                // split array by oartitioning into two arrays, returning index (pi) of
                // middle
                int pi = partition(*t_vec->vec, t_vec->lo, t_vec->hi);

                // now that split has been found, quicksort the two partitions
                // Create thread for upper/right side
                thread_vec t_vec_nthread;
                t_vec_nthread.vec = t_vec->vec;
                t_vec_nthread.hi = t_vec->hi;
                t_vec_nthread.lo = pi + 1;
                t_vec_nthread.tid = t_vec->tid + 1;

                // Create upper in parallel
                pthread_create(&threads[t_vec_nthread.tid], nullptr, pqsort,
                    &t_vec_nthread);
                // Run next branch in same thread (as there is nothing to do anymore
                // here)
                my_quicksort(*t_vec->vec, pi + 1, t_vec->hi);

                // rejoin thread
                pthread_join(threads[t_vec_nthread.tid], nullptr);
            }
        }
    }
    if (t_vec->tid == 0) {
        clock_gettime(CLOCK_MONOTONIC, &end);
    }
    pthread_exit(nullptr);
}

void my_bucketsort(std::multiset<int>& buckets, std::vector<int>& vec)
{
    //    int m = *std::max_element(vec.begin(), vec.end());
    int k = vec.size(); // determined by the size of .reserve() in main() while
        // setting up master thread

    int i = 0;
    auto it = buckets.begin();
    while (i < k) {
        buckets.insert(it, vec[i]);
        it++;
        i++;
    }
}

void* pbucket_insert_task(void* args)
{
    thread_bvec* t_vec = static_cast<thread_bvec*>(args);

    // A stupid amount of work (mostly thinking) went into
    // making this call the same no matter how the program
    // is run.
    lockBox->acquire();

    std::vector<int>::iterator it = t_vec->vec->begin();
    t_vec->buckets->insert(it + t_vec->lo, it + t_vec->hi);

    lockBox->release();

    pthread_exit(nullptr);
}

// Idea here is to divide the array into n subdivisions
// where n is arr.size() / MIN_ELEMENTS_PER_THREAD
void* pbucketsort(void* args)
{
    thread_bvec* t_vec = static_cast<thread_bvec*>(args);
    /*
   * Now there are two scenarios going forward
   * #1 - we use all allocated threads and they have greater than or equal to
   * minimum MIN_ELEMENTS_PER_THREAD each
   * #2 - we use fewer threads than
   * allocated (because its faster) and each thread is responsible for
   * MIN_ELEMENTS_PER_THREAD After spinning off the threads this master thread
   * will be idle so it will always take the remainder elements. Thus each
   * thread should have approximately the same execution time and none should be
   * faster than Master. (though all dependent on lock contention)
  */

    int sz = t_vec->vec->size();

    int tid = 1;
    int elements_perthread;

    t_vec->hi = 0;

    // Timer start
    clock_gettime(CLOCK_MONOTONIC, &start);

    if ((sz / NUM_THREADS) >= MIN_ELEMENTS_PER_THREAD) {
        elements_perthread = sz / NUM_THREADS;
    } else {
        elements_perthread = MIN_ELEMENTS_PER_THREAD;
    }

    // ctrl was previously a more arcane symbol that was impossoble to follow.
    // so now think of him as 'i'
    int ctrl = 0;
    while (ctrl < (sz - elements_perthread)) {
        thread_bvec t_bvec;
        t_bvec.vec = t_vec->vec;
        t_bvec.buckets = t_vec->buckets;

        t_bvec.lo = ctrl;
        t_bvec.hi = ctrl + elements_perthread;

        t_bvec.tid = tid;

        pthread_create(&threads[tid], nullptr, pbucket_insert_task, &t_bvec);

        ctrl = ctrl + elements_perthread;

        tid++;
    }

    // Now do last bit in main thread
    t_vec->lo = ctrl;
    t_vec->hi = sz;

    lockBox->acquire();

    // Insert a range of memory. According to docs its faster (and I dont have to for loop it)
    std::vector<int>::iterator it = t_vec->vec->begin();
    t_vec->buckets->insert(it + t_vec->lo, it + t_vec->hi);

    lockBox->release();

    for (int i = 1; i < tid; i++) {
        pthread_join(threads[i], nullptr);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    pthread_exit(nullptr);
}

int main(int argc, char* argv[])
{
    cxxopts::Options options(argv[0], "Text parse and sort for lab 2.");

    std::string input, output;

    options.add_options()(
        "n, name", "My name (for grading purposes)", cxxopts::value<bool>())(
        "i,input", "input file name", cxxopts::value<std::string>(), "FILE")(
        "o, output", "Output file name", cxxopts::value<std::string>(), "FILE")(
        "t, threads", "Number of threads to be used", cxxopts::value<int>(), "NUM_THREADS")(
        "b, bar", "Barrier type to use.\nHowever no barriers were used in this so run counter to test.", cxxopts::value<std::string>(),"<sense, pthread>")(
        "l, lock", "Lock type to use.", cxxopts::value<std::string>(),"<tas, ttas, ticket, mcs, pthread, aflag>")(
        "alg", "Algorithm to sort with", cxxopts::value<std::string>(), "<fj, bucket>")(
        "h, help", "Display help options");

    std::string ofile = "";
    std::string ifile = "";
    std::string algorithm = "";
    int algType = ALG_UNDEFINED;

    // --- Opt parse block ---
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
            ofile = "output.txt";
        }

        // Input file name
        if (result.count("i")) {
            ifile = result["i"].as<std::string>();
        } else {
            std::cout << "No input file given" << std::endl;
            return (0);
        }

        // Extract algorithm type
        if (result.count("alg")) {
            algorithm = result["alg"].as<std::string>();

            if (algorithm == "fj")
                algType = FORK_JOIN;

            else if (algorithm == "bucket") {
                algType = BUCKET;
                if (result.count("bar")) {
                    printf("No barriers were used in the Bucketsort algorithm.\nRun Count to check barrier performance.\n");
                    return 0;
                }
                if (result.count("lock")) {
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
                    printf("Please supply a lock type to be used.\nValues are: <tas, ttas, ticket, mcs, pthread, aflag>\n");
                    return 0;
                }
            } else {
                printf("Improper algorithm argument supplied, Fork/Join assumed");
                algType = FORK_JOIN;
            }
        } else {
            printf("No algorithm selected, exiting\n");
            return 0;
        }

        // Extract thread number information
        if (result.count("threads")) {
            NUM_THREADS = result["t"].as<int>();
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

    // --- Gather file data ---
    std::ifstream infile(ifile);

    std::vector<int> file_contents;

    if (infile.is_open()) {
        std::string temp;
        // populate vector w/ contents
        while (infile >> temp) {
            // Attempt to convert str to int, catch errors
            try {
                file_contents.push_back(atoi(temp.c_str()));
            } catch (std::exception& e) {
                std::cout << e.what();
            }
        }
    } else {
        std::cout << "File " << ifile << " did not open.\n";
        return 0;
    }
    // This is just in case the output file is the same as the input
    infile.close();
    // --- END Gather file data ---

    std::vector<int> file_print;

    // --- Sort data ---
    switch (algType) {
    case FORK_JOIN: {
        int fj_ret;

        global_init();
        thread_vec t_vec;
        t_vec.vec = &file_contents;
        t_vec.hi = file_contents.size() - 1;
        t_vec.lo = 0;
        t_vec.tid = 0;
        if (NUM_THREADS == 1) { // Not parallel. Avoids checking in the parallel version every loop
            if (t_vec.vec->size() > LIST_SZ_LIMIT) {
                clock_gettime(CLOCK_MONOTONIC, &start);
                my_insertionsort(*t_vec.vec, t_vec.vec->size());
                clock_gettime(CLOCK_MONOTONIC, &end);
            } else {
                clock_gettime(CLOCK_MONOTONIC, &start);
                my_quicksort(*t_vec.vec, t_vec.lo, t_vec.hi);
                clock_gettime(CLOCK_MONOTONIC, &end);
            }
        } else {
            fj_ret = pthread_create(&threads[0], nullptr, &pqsort, &t_vec);
            if (fj_ret) {
                printf("Error: in creating master thread %d\n", fj_ret);
                return -1;
            }
            // Join master
            fj_ret = pthread_join(threads[0], nullptr);
            if (fj_ret) {
                printf("Error: in creating master thread %d\n", fj_ret);
                return -1;
            }
            global_cleanup();
        }
        file_print = *t_vec.vec;
        break;
    }

    case BUCKET: {
        // Start same as JORK_JOIN
        // Though notably the linter has no respect for break statements in case
        // statements
        int b_ret;

        global_init();
        global_bs_init();
        locks_init();
        thread_bvec t_vec_b;
        t_vec_b.vec = &file_contents;
        t_vec_b.hi = file_contents.size() - 1;
        t_vec_b.lo = 0;
        t_vec_b.tid = 0;

        std::multiset<int> buckets;
        t_vec_b.buckets = &buckets;

        // If there is a single thread or the problem is small enough to use a
        // single thread Skip overhead and compute in main() thread (from outside
        // main() perspective at any rate)
        if ((NUM_THREADS == 1) || (t_vec_b.hi < MIN_ELEMENTS_PER_THREAD)) {
            // Timing
            clock_gettime(CLOCK_MONOTONIC, &start);
            my_bucketsort(*t_vec_b.buckets, *t_vec_b.vec);
            clock_gettime(CLOCK_MONOTONIC, &end);
        } else {
            b_ret = pthread_create(&threads[0], nullptr, pbucketsort, &t_vec_b);
            if (b_ret) {
                printf("Error: in creating master thread %d\n", b_ret);
                return -1;
            }
            b_ret = pthread_join(threads[0], nullptr);
            if (b_ret) {
                printf("Error: in joining master thread %d\n", b_ret);
                return -1;
            }
        }

        global_bs_cleanup();
        global_cleanup();
        locks_cleanup();

        std::vector<int> temp(t_vec_b.buckets->begin(), t_vec_b.buckets->end());
        file_print = temp;
        break;
    }
    case ALG_UNDEFINED: {
        printf("ALG_UNDEFINED - This should have been set or returned before getting here\n");
        return -1;
    }
    default: {
        printf("How even\n");
        return -1;
    }
    }

    // Print time
    unsigned long long elapsed_ns;
    elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
    printf("Elapsed (ns): %llu\n", elapsed_ns);
    double elapsed_s = static_cast<double>(elapsed_ns) / 1000000000.0;
    printf("Elapsed (s): %lf\n", elapsed_s);

    // --- Write data ---
    std::ofstream outfile(ofile);

    if (outfile.is_open()) {
        try {
            for (std::vector<int>::const_iterator i = file_print.begin();
                 i != file_print.end(); i++) {
                outfile << *i << std::endl;
//                std::cout << *i << std::endl; // comment to remove/add terminal print
            }
        } catch (std::exception& e) {
            std::cout << e.what();
        }
    }
    return 0;
}
