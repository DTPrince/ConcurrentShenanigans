#include <iostream>
#include <exception>
#include <pthread.h>
#include <string>
#include <fstream>
#include <random>

#include <sys/types.h>

#include "include/cxxopts.hpp"
#include "include/skippy.h"

static pthread_t* threads;
static int NUM_THREADS = 0;
static struct timespec t_start, t_end;

// Threads initialize and cleanup
void global_init() { threads = static_cast<pthread_t*>(malloc(NUM_THREADS * sizeof(pthread_t))); }
void global_cleanup() { free(threads); }

// Small structure for passing information around in threads
typedef struct {
    int low = -1;
    int high = -1;
} ThreadCommunication;

Skippy skippy(3, 0.3);
ThreadCommunication tc;
int data_per_thread = 1;

// Forwarder to call class function as it was cleaner than every other
// implementation aside from ripping up all my code and restructuring
// the entire class. Which wasn't even guaranteed to work
static void * pinsert_helper(void * arg) {
    int * keys = static_cast<int *>(arg);
    for (int i = 0; i < data_per_thread; i++){
        skippy.pinsert(keys[i]);
    }
    pthread_exit(nullptr);
}

static void * pget_range_helper(void * arg) {
    ThreadCommunication *tc = (ThreadCommunication *)arg;
    std::vector<int> * range_vec = skippy.pget_range(tc->low, tc->high);
    pthread_exit(range_vec);
}

static void * pget_helper(void* arg) {
    SLNode * result;
    result = skippy.pget(*static_cast<int*>(arg));
    pthread_exit(result);
}

int main(int argc, char* argv[]) {
    cxxopts::Options options(argv[0], "Concurrent Skip List final project implemented with"
                                      " fine-grained hand-over-hand locking");

    std::string input, output;
    //./input/input.txt -t 100

    options.add_options()(
            "n, name", "My name (for grading purposes)", cxxopts::value<bool>())(
            "i, input", "input file name", cxxopts::value<std::string>(), "FILE")(
            "t, threads", "Number of threads to be used", cxxopts::value<int>(), "NUM_THREADS")(
            "h, help", "Display help options");

    std::string ifile = "";

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

        // Input file name
        if (result.count("i")) {
            ifile = result["i"].as<std::string>();
        } else {
            std::cout << "No input file given" << std::endl;
            return (0);
        }
//         int NUM_THREADS;
        // Extract thread number information
        if (result.count("threads")) {
            NUM_THREADS = result["t"].as<int>();
            if (NUM_THREADS > 150) {
                printf("Error: Too many threads\n");
                return (-1);
            }
        } else if (NUM_THREADS > 50) {
            NUM_THREADS = 50;
        } else
            NUM_THREADS = 5;
    }
    // Catch unknown options and close nicely
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        return -1;
    }
    // --- END Opt parse block ---

    // --- Gather file data ---
    std::ifstream infile(ifile);

    std::vector<int> file_contents;

    if (infile.is_open())
    {
        std::string temp;
        // populate vector w/ contents
        while (infile >> temp)
        {
            // Attempt to convert str to int, catch errors
            try
            {
                file_contents.push_back(atoi(temp.c_str()));
            }
            catch (std::exception& e)
            {
                std::cout << e.what();
            }
        }
    }
    else
        std::cout << "File " << ifile << " did not open.";

    // This is just in case the output file is the same as the input
    infile.close();

    // --- Insert items ---
    global_init();

    // Sequential insert only
//    for (auto i = file_contents.begin(); i < file_contents.end(); i++) {
//        skippy.insert(*i);
//    }

    data_per_thread = file_contents.size() / NUM_THREADS;
//    std::vector<int> insert[NUM_THREADS];
    // Super garbage way to handle arbitrary thread numbers
    // Alternative is to just modify ThreadCommunications struct
    // but that has its own downsides for something trivial
    int insert[NUM_THREADS][data_per_thread];

    // Prep vectors for threads
    // There are two cases here but I have condensed them into one for maximum bugs
    // (Just kidding. I am no longer using vectors is all)
    for (int i = 0; i < NUM_THREADS; i++){
        for (int j = 0; j < data_per_thread; j++){
            insert[i][j] = file_contents[i*data_per_thread + j];
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t_start);
    for (int i = 0; i < NUM_THREADS; i++) {
        static int tid = 0;
        int ret = pthread_create(&threads[tid], nullptr, &pinsert_helper, &insert[i]);
        tid++;
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        static int tid = 0;
        int ret = pthread_join(threads[tid], nullptr);
        tid++;
    }
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    unsigned long long t_insert = (t_end.tv_sec - t_start.tv_sec) * 1000000000 + (t_end.tv_nsec - t_start.tv_nsec);
    // Print. Or not.
    skippy.display();


    // Ranging tests:
//    std::vector<int> * r = skippy.get_range(2, 12);
//
//    std::cout << "In range {2,12}: ";
//    for (auto i = r->begin(); i < r->end(); i++){
//        std::cout << *i << " ";
//    }
//    std::cout << std::endl;

    tc.low = 6;
    tc.high = 45;

    clock_gettime(CLOCK_MONOTONIC, &t_start);
    auto ret = pthread_create(&threads[0], nullptr, &pget_range_helper, &tc);
    if (ret)
        std::cout << "Thread creation error.\n";
    std::vector<int> * r2;
    auto retval = pthread_join(threads[0], (void **)&r2);
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    if (retval)
        std::cout << "Error joining thread\n";

    unsigned long long t_range = (t_end.tv_sec - t_start.tv_sec) * 1000000000 + (t_end.tv_nsec - t_start.tv_nsec);

    std::cout << "In range {6,45}: ";
    for (auto i = r2->begin(); i < r2->end(); i++)
        std::cout << *i << " ";
    std::cout << std::endl;

    // Get tests:
    // choosing 36 here because I know its in my test data
//    SLNode * gottee = skippy.pget(36);
//    if (gottee->key == 36){
//        std::cout << "Get {36} Success.\n";
//    }
//    else
//        std::cout << "Key not present in list. Returned {" << gottee->key << "}\n";

    // This is decidedly not random. But thats fine, its not actually important
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0,file_contents.size());
    int rnum = distribution(generator);
    int search_val = file_contents[rnum];
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    auto gc_ret = pthread_create(&threads[0], nullptr, &pget_helper, &search_val);
    if (gc_ret)
        std::cout << "Thread creation error.\n";

    SLNode * get_node;
    auto gj_ret = pthread_join(threads[0], (void **)&get_node);
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    if (gj_ret)
        std::cout << "Thread creation error.\n";

    unsigned long long t_get = (t_end.tv_sec - t_start.tv_sec) * 1000000000 + (t_end.tv_nsec - t_start.tv_nsec);

    // check
    if (get_node->key == search_val){
        std::cout << "Get {" << search_val << "} Success but in a new thread.\n";
    }
    else
        std::cout << "Key not present in list. Returned {" << get_node->key << "}\n";

    // Timing prints
    std::cout << std::endl;
    std::cout << "Skip list inserted in approximately " << t_insert << " ns.\n";
    std::cout << "Skip list fetched range in approximately " << t_range << " ns.\n";
    std::cout << "Skip list returned {" << search_val << "} in approximately " << t_get << " ns.\n";

    // Benchmarks

    global_cleanup();

    return 0;
}
