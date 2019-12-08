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

// Small structure for passing information around in threads
typedef struct {
    Skippy *skip;
    int key;
    int low = -1;
    int high = -1;
} ThreadCommunication;

// Forwarder to call class function without doing the reference/dereference void * bullshit
// as it was cleaner than every other implementation aside from ripping up all my code
// and restructuring the entire class. Which wasn't even guaranteed to work
static void * pinsert_helper(void * arg){
    ThreadCommunication *tc = (ThreadCommunication *)arg;
    tc->skip->pinsert(tc->key);
    pthread_exit(nullptr);
}

static void * pget_range_helper(void * arg) {
    ThreadCommunication *tc = (ThreadCommunication *)arg;
    std::vector<int> * range_vec = tc->skip->pget_range(tc->low, tc->high);
    pthread_exit(nullptr);
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

    Skippy skippy(3, 0.5);
    ThreadCommunication tc;
    tc.skip = &skippy;

    // Sequential insert only
//    for (auto i = file_contents.begin(); i < file_contents.end(); i++) {
//        skippy.insert(*i);
//    }


    int ret = -1;
    for (int i = 0; i < file_contents.size(); i++) {
        tc.key = file_contents[i];
        ret = pthread_create(&threads[i], nullptr, &pinsert_helper, &tc);
    }
    for (int i = 0; i < file_contents.size(); i++) {
        ret = pthread_join(threads[i], nullptr);
    }

    global_cleanup();

    // Print. Or not.
    skippy.display();

    std::vector<int> * r = skippy.get_range(2, 12);
    std::cout << "In range {2,12}: ";
    for (auto i = r->begin(); i < r->end(); i++){
        std::cout << *i << " ";
    }


    return 0;
}
