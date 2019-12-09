#include <iostream>
#include <exception>
#include <pthread.h>
#include <string>
#include <fstream>

#include "include/cxxopts.hpp"
#include "include/skippy.h"

static pthread_t* threads;
static int NUM_THREADS = 0;
static struct timespec start, end;

// Threads initialize and cleanup
void global_init() { threads = static_cast<pthread_t*>(malloc(NUM_THREADS * sizeof(pthread_t))); }
void global_cleanup() { free(threads); }

std::vector<int> test;


// Small structure for passing information around in threads
typedef struct {
    Skippy *skip;
    int key;
    int low = -1;
    int high = -1;
} ThreadCommunication;

Skippy skippy(3, 0.5);
ThreadCommunication tc;

// Forwarder to call class function without doing the reference/dereference void * bullshit
// as it was cleaner than every other implementation aside from ripping up all my code
// and restructuring the entire class. Which wasn't even guaranteed to work
static void * pinsert_helper(void * arg){
    int * key = (int*)arg;
    int keyval = *key;
//    test.push_back(keyval);
    tc.skip->pinsert(keyval);
    pthread_exit(nullptr);
}

static void * pget_range_helper(void * arg) {
    ThreadCommunication *tc = (ThreadCommunication *)arg;
    std::vector<int> * range_vec = tc->skip->pget_range(tc->low, tc->high);
    pthread_exit(range_vec);
}

int main(int argc, char* argv[]) {
    cxxopts::Options options(argv[0], "Concurrent Skip List final project implemented with"
                                      " fine-grained hand-over-hand locking");

    std::string input, output;

    options.add_options()(
            "n, name", "My name (for grading purposes)", cxxopts::value<bool>())(
            "i, input", "input file name", cxxopts::value<std::string>(), "FILE")(
            "o, output", "Output file name", cxxopts::value<std::string>(), "FILE")(
            "t, threads", "Number of threads to be used", cxxopts::value<int>(), "NUM_THREADS")(
            "h, help", "Display help options");

    std::string ofile = "";
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
//         int NUM_THREADS;
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

    tc.skip = &skippy;
    int ret = -1;

    ThreadCommunication tc0;
    tc0.skip = &skippy;
    tc0.key = 0;
    ThreadCommunication tc1;
    tc1.skip = &skippy;
    tc1.key = 1;
    ThreadCommunication tc2;
    tc2.skip = &skippy;
    tc2.key = 2;
    ThreadCommunication tc3;
    tc3.skip = &skippy;
    tc3.key = 3;
    ThreadCommunication tc4;
    tc4.skip = &skippy;
    tc4.key = 4;
    ThreadCommunication tc5;
    tc5.skip = &skippy;
    tc5.key = 5;
    ThreadCommunication tc6;
    tc6.skip = &skippy;
    tc6.key = 6;
    ThreadCommunication tc7;
    tc7.skip = &skippy;
    tc7.key = 7;
    ThreadCommunication tc8;
    tc8.skip = &skippy;
    tc8.key = 8;
    ThreadCommunication tc9;
    tc9.skip = &skippy;
    tc9.key = 9;

    ret = pthread_create(&threads[0], nullptr, &pinsert_helper, &tc0.key);
    ret = pthread_create(&threads[1], nullptr, &pinsert_helper, &tc1.key);
    ret = pthread_create(&threads[2], nullptr, &pinsert_helper, &tc2.key);
    ret = pthread_create(&threads[3], nullptr, &pinsert_helper, &tc3.key);
    ret = pthread_create(&threads[4], nullptr, &pinsert_helper, &tc4.key);
    ret = pthread_create(&threads[5], nullptr, &pinsert_helper, &tc5.key);
    ret = pthread_create(&threads[6], nullptr, &pinsert_helper, &tc6.key);
    ret = pthread_create(&threads[7], nullptr, &pinsert_helper, &tc7.key);
    ret = pthread_create(&threads[8], nullptr, &pinsert_helper, &tc8.key);
    ret = pthread_create(&threads[9], nullptr, &pinsert_helper, &tc9.key);

    for (int i = 0; i < 10; i++) {
        ret = pthread_join(threads[i], nullptr);
        if (ret != 0){
            std::cout << "Error on thread " << i << " join.\n";
        }
    }

    global_cleanup();

    // Print. Or not.
    skippy.display();

    std::cout << "Test vec:\n";
    for (auto i = test.begin(); i< test.end(); i++){
        std::cout << *i << " ";
    }
    std::cout << "\n";
//    std::vector<int> * r = skippy.get_range(2, 12);
//
//    std::cout << "In range {2,12}: ";
//    for (auto i = r->begin(); i < r->end(); i++){
//        std::cout << *i << " ";
//    }
//    std::cout << std::endl;

//    tc.low = 6;
//    tc.high = 28;
//    auto t1 = pthread_create(&threads[0], nullptr, &pget_range_helper, &tc);
//    std::vector<int> * r2;
//    auto temp = pthread_join(threads[0], (void **)&r2);
//    std::cout << "In range {6,28}: ";
//    for (auto i = r2->begin(); i < r2->end(); i++){
//        std::cout << *i << " ";
//    }

    return 0;
}
