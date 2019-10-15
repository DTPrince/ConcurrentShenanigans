// Derek Prince
// ECEN - 5033 Concurrent Programming
// Lab 1 - Concurrent Text Sort with Quicksort and Bucketsort
// Due 9/25/19

#include <exception>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <set> // multiset support
#include <string>

#include "include/cxxopts.hpp"
#include "include/locks.hpp"

#define FORK_JOIN 1
#define BUCKET 0
#define ALG_UNDEFINED                                                          \
  10 // arbitrary number. No inclined to use negative numbers at this time to \
        // avoid potential unsigned/signed cross-pollination

#define THREAD_INACTIVE true
#define THREAD_ACTIVE false
// Played with this number a bit but it'll depend on the machine so I'm not too
// woried about the +/-
#define LIST_SZ_LIMIT 12

// Manages how many entries in origninal array a thread is responsible for
#define MIN_ELEMENTS_PER_THREAD 10

static pthread_t *threads;
static pthread_mutex_t *locks;
static int *no_threads;
static int NUM_THREADS = 0;
static struct timespec start, end;

static std::multiset<int> *landfill = nullptr;

// 10 ave -> 207.16
// 11 ave -> 197.6
// 12 ave -> 171

typedef struct {
    std::vector<int> *vec;
    int lo, hi, tid;
} thread_vec;

typedef struct {
    std::vector<int> *vec;
    std::multiset<int> *buckets; // looks gross tbh but it should abstract cleanly
    int lo, hi, tid;
} thread_bvec;

typedef struct {
    int *status;
    int no_active;
} thread_status;

void global_init() {
    threads = static_cast<pthread_t *>(malloc(NUM_THREADS * sizeof(pthread_t)));
    no_threads = static_cast<int *>(malloc(NUM_THREADS * sizeof(int)));
}

void global_bs_init() { landfill = new std::multiset<int>(); }

void locks_init() {
    locks = static_cast<pthread_mutex_t *>(
        malloc(NUM_THREADS * sizeof(pthread_mutex_t)));
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_mutex_init(&locks[i], nullptr);
    }
}

void locks_cleanup() { free(locks); }

void global_cleanup() {
    free(threads);
    free(no_threads);
}

void global_bs_cleanup() { delete landfill; }


int main(int argc, char *argv[]) {
    cxxopts::Options options(argv[0], "Text parser for lab 1.");

    std::string input, output;

    options.add_options()(
        "n, name", "My name (for grading purposes)", cxxopts::value<bool>())(
        "i,input", "input file name", cxxopts::value<std::string>(), "FILE")(
        "o, output", "Output file name", cxxopts::value<std::string>(), "FILE")(
        "t, threads", "Number of threads to be used", cxxopts::value<int>())(
        "b, bar", "Barrier type to use", cxxopts::value<std::string>())(
        "l, lock", "Lock type to use", cxxopts::value<std::string>())(
        "h, help", "Display help options");

    std::string ofile = "";
    std::string ifile = "";

    // --- Opt parse block ---
    try {
        options.parse_positional({"input"});
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help({"", "Group"}) << std::endl;
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
    catch (std::exception &e) {
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
            } catch (std::exception &e) {
                std::cout << e.what();
            }
        }
    } else
        std::cout << "File " << ifile << " did not open.";

    // This is just in case the output file is the same as the input
    infile.close();
    // --- END Gather file data ---


    std::vector<int> file_print;


    // Print time
    unsigned long long elapsed_ns;
    elapsed_ns =
        (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
    printf("Elapsed (ns): %llu\n", elapsed_ns);
    double elapsed_s = ((double)elapsed_ns) / 1000000000.0;
    printf("Elapsed (s): %lf\n", elapsed_s);

    // --- Write data ---
    std::ofstream outfile(ofile);

    if (outfile.is_open()) {
        try {
            for (std::vector<int>::const_iterator i = file_print.begin();
                 i != file_print.end(); i++) {
                outfile << *i << std::endl;
            }
        } catch (std::exception &e) {
            std::cout << e.what();
        }
    }
    return 0;
}
